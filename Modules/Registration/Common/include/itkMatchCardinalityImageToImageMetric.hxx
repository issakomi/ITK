/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkMatchCardinalityImageToImageMetric_hxx
#define itkMatchCardinalityImageToImageMetric_hxx

#include "itkMath.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkPrintHelper.h"

namespace itk
{

template <typename TFixedImage, typename TMovingImage>
MatchCardinalityImageToImageMetric<TFixedImage, TMovingImage>::MatchCardinalityImageToImageMetric()
{
  this->SetComputeGradient(false); // don't use the default gradients
}

template <typename TFixedImage, typename TMovingImage>
auto
MatchCardinalityImageToImageMetric<TFixedImage, TMovingImage>::GetValue(
  const TransformParametersType & parameters) const -> MeasureType
{
  return const_cast<Self *>(this)->GetNonconstValue(parameters);
}

template <typename TFixedImage, typename TMovingImage>
auto
MatchCardinalityImageToImageMetric<TFixedImage, TMovingImage>::GetNonconstValue(
  const TransformParametersType & parameters) -> MeasureType
{
  itkDebugMacro("GetValue( " << parameters << " ) ");

  const FixedImageConstPointer fixedImage = this->m_FixedImage;
  if (!fixedImage)
  {
    itkExceptionMacro("Fixed image has not been assigned");
  }

  // Initialize some variables before spawning threads
  //
  //
  MeasureType measure{};
  this->m_NumberOfPixelsCounted = 0;

  m_ThreadMatches.clear();
  m_ThreadCounts.clear();
  m_ThreadMatches.resize(this->GetNumberOfWorkUnits());
  m_ThreadCounts.resize(this->GetNumberOfWorkUnits());

  {
    auto mIt = m_ThreadMatches.begin();
    for (auto cIt = m_ThreadCounts.begin(); mIt != m_ThreadMatches.end(); ++mIt, ++cIt)
    {
      *mIt = MeasureType{};
      *cIt = 0;
    }
  }
  // store the parameters in the transform so all threads can access them
  this->SetTransformParameters(parameters);

  // Set up the multithreaded processing
  //
  //
  ThreadStruct str;
  str.Metric = this;

  this->GetMultiThreader()->SetNumberOfWorkUnits(this->GetNumberOfWorkUnits());
  this->GetMultiThreader()->SetSingleMethodAndExecute(this->ThreaderCallback, &str);

  // Collect the contribution to the metric for each thread
  //
  //
  {
    auto mIt = m_ThreadMatches.begin();
    for (auto cIt = m_ThreadCounts.begin(); mIt != m_ThreadMatches.end(); ++mIt, ++cIt)
    {
      measure += *mIt;
      this->m_NumberOfPixelsCounted += *cIt;
    }
  }
  if (!this->m_NumberOfPixelsCounted)
  {
    itkExceptionMacro("All the points mapped to outside of the moving image");
  }
  else
  {
    measure /= this->m_NumberOfPixelsCounted;
  }

  return measure;
}

template <typename TFixedImage, typename TMovingImage>
void
MatchCardinalityImageToImageMetric<TFixedImage, TMovingImage>::ThreadedGetValue(
  const FixedImageRegionType & regionForThread,
  ThreadIdType                 threadId)
{
  const FixedImageConstPointer fixedImage = this->GetFixedImage();

  if (!fixedImage)
  {
    itkExceptionMacro("Fixed image has not been assigned");
  }

  using FixedIteratorType = ImageRegionConstIteratorWithIndex<FixedImageType>;
  FixedIteratorType ti(fixedImage, regionForThread);

  MeasureType   threadMeasure{};
  SizeValueType threadNumberOfPixelsCounted = 0;
  while (!ti.IsAtEnd())
  {
    const typename FixedImageType::IndexType index = ti.GetIndex();

    typename Superclass::InputPointType inputPoint;
    fixedImage->TransformIndexToPhysicalPoint(index, inputPoint);

    if (this->GetFixedImageMask() && !this->GetFixedImageMask()->IsInsideInWorldSpace(inputPoint))
    {
      ++ti;
      continue;
    }

    const typename Superclass::OutputPointType transformedPoint = this->GetTransform()->TransformPoint(inputPoint);

    if (this->GetMovingImageMask() && !this->GetMovingImageMask()->IsInsideInWorldSpace(transformedPoint))
    {
      ++ti;
      continue;
    }

    if (this->GetInterpolator()->IsInsideBuffer(transformedPoint))
    {
      const RealType movingValue = this->GetInterpolator()->Evaluate(transformedPoint);
      const RealType fixedValue = ti.Get();
      ++threadNumberOfPixelsCounted;

      RealType diff;
      if (m_MeasureMatches)
      {
        diff = Math::AlmostEquals(movingValue, fixedValue); // count matches
      }
      else
      {
        diff = Math::NotAlmostEquals(movingValue, fixedValue); // count mismatches
      }
      threadMeasure += diff;
    }

    ++ti;
  }

  m_ThreadMatches[threadId] = threadMeasure;
  m_ThreadCounts[threadId] = threadNumberOfPixelsCounted;
}

template <typename TFixedImage, typename TMovingImage>
ThreadIdType
MatchCardinalityImageToImageMetric<TFixedImage, TMovingImage>::SplitFixedRegion(ThreadIdType           i,
                                                                                int                    num,
                                                                                FixedImageRegionType & splitRegion)
{
  // Get the output pointer
  const typename FixedImageRegionType::SizeType & fixedRegionSize = this->GetFixedImageRegion().GetSize();

  // Initialize the splitRegion to the output requested region
  splitRegion = this->GetFixedImageRegion();
  typename FixedImageRegionType::IndexType splitIndex = splitRegion.GetIndex();
  typename FixedImageRegionType::SizeType  splitSize = splitRegion.GetSize();

  // split on the outermost dimension available
  int splitAxis = this->GetFixedImage()->GetImageDimension() - 1;
  while (fixedRegionSize[splitAxis] == 1)
  {
    --splitAxis;
    if (splitAxis < 0)
    { // cannot split
      itkDebugMacro("  Cannot Split");
      return 1;
    }
  }

  // determine the actual number of pieces that will be generated
  const typename FixedImageRegionType::SizeType::SizeValueType range = fixedRegionSize[splitAxis];
  auto               valuesPerThread = Math::Ceil<int>(range / static_cast<double>(num));
  const ThreadIdType maxThreadIdUsed = Math::Ceil<int>(range / static_cast<double>(valuesPerThread)) - 1;

  // Split the region
  if (i < maxThreadIdUsed)
  {
    splitIndex[splitAxis] += i * valuesPerThread;
    splitSize[splitAxis] = valuesPerThread;
  }
  if (i == maxThreadIdUsed)
  {
    splitIndex[splitAxis] += i * valuesPerThread;
    // last thread needs to process the "rest" dimension being split
    splitSize[splitAxis] = splitSize[splitAxis] - i * valuesPerThread;
  }

  // set the split region ivars
  splitRegion.SetIndex(splitIndex);
  splitRegion.SetSize(splitSize);

  itkDebugMacro("  Split Piece: " << splitRegion);

  return maxThreadIdUsed + 1;
}

template <typename TFixedImage, typename TMovingImage>
ITK_THREAD_RETURN_FUNCTION_CALL_CONVENTION
MatchCardinalityImageToImageMetric<TFixedImage, TMovingImage>::ThreaderCallback(void * arg)
{
  const ThreadIdType workUnitID = (static_cast<MultiThreaderBase::WorkUnitInfo *>(arg))->WorkUnitID;
  const ThreadIdType workUnitCount = (static_cast<MultiThreaderBase::WorkUnitInfo *>(arg))->NumberOfWorkUnits;

  auto * str = (ThreadStruct *)((static_cast<MultiThreaderBase::WorkUnitInfo *>(arg))->UserData);

  // execute the actual method with appropriate computation region
  // first find out how many pieces extent can be split into.
  FixedImageRegionType splitRegion;
  const ThreadIdType   total = str->Metric->SplitFixedRegion(workUnitID, workUnitCount, splitRegion);

  if (workUnitID < total)
  {
    str->Metric->ThreadedGetValue(splitRegion, workUnitID);
  }
  // else
  //   {
  //   otherwise don't use this thread. Sometimes the threads don't
  //   break up very well and it is just as efficient to leave a
  //   few threads idle.
  //   }

  return ITK_THREAD_RETURN_DEFAULT_VALUE;
}

template <typename TFixedImage, typename TMovingImage>
void
MatchCardinalityImageToImageMetric<TFixedImage, TMovingImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  using namespace print_helper;

  Superclass::PrintSelf(os, indent);

  itkPrintSelfBooleanMacro(MeasureMatches);

  os << indent << "ThreadMatches: " << m_ThreadMatches << std::endl;
  os << indent << "ThreadCounts: " << m_ThreadCounts << std::endl;

  itkPrintSelfObjectMacro(Threader);
}
} // end namespace itk

#endif
