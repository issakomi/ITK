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
#ifndef itkJointHistogramMutualInformationComputeJointPDFThreaderBase_hxx
#define itkJointHistogramMutualInformationComputeJointPDFThreaderBase_hxx


#include "itkImageRegionIterator.h"
#include "itkMakeUniqueForOverwrite.h"

namespace itk
{

template <typename TDomainPartitioner, typename TJointHistogramMetric>
JointHistogramMutualInformationComputeJointPDFThreaderBase<TDomainPartitioner, TJointHistogramMetric>::
  JointHistogramMutualInformationComputeJointPDFThreaderBase()
  : m_JointHistogramMIPerThreadVariables(nullptr)
{}

template <typename TDomainPartitioner, typename TJointHistogramMetric>
void
JointHistogramMutualInformationComputeJointPDFThreaderBase<TDomainPartitioner,
                                                           TJointHistogramMetric>::BeforeThreadedExecution()
{
  const ThreadIdType numWorkUnitsUsed = this->GetNumberOfWorkUnitsUsed();
  this->m_JointHistogramMIPerThreadVariables =
    make_unique_for_overwrite<AlignedJointHistogramMIPerThreadStruct[]>(numWorkUnitsUsed);
  for (ThreadIdType i = 0; i < numWorkUnitsUsed; ++i)
  {
    if (this->m_JointHistogramMIPerThreadVariables[i].JointHistogram.IsNull())
    {
      this->m_JointHistogramMIPerThreadVariables[i].JointHistogram = JointHistogramType::New();
    }
    this->m_JointHistogramMIPerThreadVariables[i].JointHistogram->CopyInformation(this->m_Associate->m_JointPDF);
    this->m_JointHistogramMIPerThreadVariables[i].JointHistogram->SetRegions(
      this->m_Associate->m_JointPDF->GetLargestPossibleRegion());
    this->m_JointHistogramMIPerThreadVariables[i].JointHistogram->AllocateInitialized();
    this->m_JointHistogramMIPerThreadVariables[i].JointHistogramCount = SizeValueType{};
  }
}

template <typename TDomainPartitioner, typename TJointHistogramMetric>
void
JointHistogramMutualInformationComputeJointPDFThreaderBase<TDomainPartitioner, TJointHistogramMetric>::ProcessPoint(
  const VirtualIndexType & itkNotUsed(virtualIndex),
  const VirtualPointType & virtualPoint,
  const ThreadIdType       threadId)
{
  typename AssociateType::Superclass::FixedImagePointType  mappedFixedPoint;
  typename AssociateType::Superclass::FixedImagePixelType  fixedImageValue;
  typename AssociateType::Superclass::MovingImagePointType mappedMovingPoint;
  typename AssociateType::Superclass::MovingImagePixelType movingImageValue;
  bool                                                     pointIsValid = false;

  try
  {
    pointIsValid = this->m_Associate->TransformAndEvaluateFixedPoint(virtualPoint, mappedFixedPoint, fixedImageValue);
    if (pointIsValid)
    {
      pointIsValid =
        this->m_Associate->TransformAndEvaluateMovingPoint(virtualPoint, mappedMovingPoint, movingImageValue);

      /** Add the paired intensity points to the joint histogram */
      if (pointIsValid)
      {
        JointPDFPointType jointPDFpoint;
        this->m_Associate->ComputeJointPDFPoint(fixedImageValue, movingImageValue, jointPDFpoint);
        const auto jointPDFIndex =
          m_JointHistogramMIPerThreadVariables[threadId].JointHistogram->TransformPhysicalPointToIndex(jointPDFpoint);
        if (this->m_JointHistogramMIPerThreadVariables[threadId].JointHistogram->GetBufferedRegion().IsInside(
              jointPDFIndex))
        {
          typename JointHistogramType::PixelType jointHistogramPixel =
            this->m_JointHistogramMIPerThreadVariables[threadId].JointHistogram->GetPixel(jointPDFIndex);
          ++jointHistogramPixel;
          this->m_JointHistogramMIPerThreadVariables[threadId].JointHistogram->SetPixel(jointPDFIndex,
                                                                                        jointHistogramPixel);
          this->m_JointHistogramMIPerThreadVariables[threadId].JointHistogramCount++;
        }
      }
    }
  }
  catch (const ExceptionObject & exc)
  {
    // NOTE: there must be a cleaner way to do this:
    std::string msg("Caught exception: \n");
    msg += exc.what();
    ExceptionObject err(__FILE__, __LINE__, msg);
    throw err;
  }
}

template <typename TDomainPartitioner, typename TJointHistogramMetric>
void
JointHistogramMutualInformationComputeJointPDFThreaderBase<TDomainPartitioner,
                                                           TJointHistogramMetric>::AfterThreadedExecution()
{
  const ThreadIdType numberOfWorkUnitsUsed = this->GetNumberOfWorkUnitsUsed();

  using JointHistogramPixelType = typename JointHistogramType::PixelType;
  this->m_Associate->m_JointHistogramTotalCount = SizeValueType{};
  for (ThreadIdType i = 0; i < numberOfWorkUnitsUsed; ++i)
  {
    this->m_Associate->m_JointHistogramTotalCount += this->m_JointHistogramMIPerThreadVariables[i].JointHistogramCount;
  }

  if (this->m_Associate->m_JointHistogramTotalCount == 0)
  {
    this->m_Associate->m_JointPDF->FillBuffer(SizeValueType{});
    return;
  }

  using JointPDFIteratorType = ImageRegionIterator<JointPDFType>;
  JointPDFIteratorType jointPDFIt(this->m_Associate->m_JointPDF, this->m_Associate->m_JointPDF->GetBufferedRegion());
  jointPDFIt.GoToBegin();
  using JointHistogramIteratorType = ImageRegionConstIterator<JointHistogramType>;
  std::vector<JointHistogramIteratorType> jointHistogramPerThreadIts;
  for (ThreadIdType i = 0; i < numberOfWorkUnitsUsed; ++i)
  {
    jointHistogramPerThreadIts.push_back(
      JointHistogramIteratorType(this->m_JointHistogramMIPerThreadVariables[i].JointHistogram,
                                 this->m_JointHistogramMIPerThreadVariables[i].JointHistogram->GetBufferedRegion()));
    jointHistogramPerThreadIts[i].GoToBegin();
  }

  JointHistogramPixelType jointHistogramPixel = 0;
  while (!jointPDFIt.IsAtEnd())
  {
    jointHistogramPixel = JointHistogramPixelType{};
    for (ThreadIdType i = 0; i < numberOfWorkUnitsUsed; ++i)
    {
      jointHistogramPixel += jointHistogramPerThreadIts[i].Get();
      ++jointHistogramPerThreadIts[i];
    }
    jointPDFIt.Set(static_cast<JointPDFValueType>(jointHistogramPixel) /
                   static_cast<JointPDFValueType>(this->m_Associate->m_JointHistogramTotalCount));
    ++jointPDFIt;
  }
}

} // end namespace itk

#endif
