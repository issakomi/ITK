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
#ifndef itkVanHerkGilWermanErodeDilateImageFilter_hxx
#define itkVanHerkGilWermanErodeDilateImageFilter_hxx

#include "itkImageRegionIterator.h"

#include "itkVanHerkGilWermanUtilities.h"

namespace itk
{
template <typename TImage, typename TKernel, typename TFunction1>
VanHerkGilWermanErodeDilateImageFilter<TImage, TKernel, TFunction1>::VanHerkGilWermanErodeDilateImageFilter()
  : m_Boundary(InputImagePixelType{})
{
  this->DynamicMultiThreadingOn();
  this->ThreaderUpdateProgressOff();
}

template <typename TImage, typename TKernel, typename TFunction1>
void
VanHerkGilWermanErodeDilateImageFilter<TImage, TKernel, TFunction1>::DynamicThreadedGenerateData(
  const InputImageRegionType & outputRegionForThread)
{
  // check that we are using a decomposable kernel
  if (!this->GetKernel().GetDecomposable())
  {
    itkExceptionMacro("VanHerkGilWerman morphology only works with decomposable structuring elements");
  }

  // TFunction1 will be < for erosions

  // the initial version will adopt the methodology of loading a line
  // at a time into a buffer vector, carrying out the opening or
  // closing, and then copy the result to the output. Hopefully this
  // will improve cache performance when working along non raster
  // directions.

  InputImageConstPointer input = this->GetInput();

  const SizeValueType totalPixels =
    this->GetKernel().GetLines().size() * this->GetOutput()->GetRequestedRegion().GetNumberOfPixels();
  TotalProgressReporter progress(this, totalPixels);

  InputImageRegionType IReg = outputRegionForThread;
  IReg.PadByRadius(this->GetKernel().GetRadius());
  // IReg.PadByRadius( this->GetKernel().GetRadius() );
  IReg.Crop(this->GetInput()->GetRequestedRegion());

  // allocate an internal buffer
  auto internalbuffer = InputImageType::New();
  internalbuffer->SetRegions(IReg);
  internalbuffer->Allocate();
  const InputImagePointer output = internalbuffer;

  // get the region size
  const InputImageRegionType OReg = outputRegionForThread;
  // maximum buffer length is sum of dimensions
  unsigned int bufflength = 0;
  for (unsigned int i = 0; i < TImage::ImageDimension; ++i)
  {
    bufflength += IReg.GetSize()[i];
  }

  // compat
  bufflength += 2;

  std::vector<InputImagePixelType> buffer(bufflength);
  std::vector<InputImagePixelType> forward(bufflength);
  std::vector<InputImagePixelType> reverse(bufflength);
  // iterate over all the structuring elements
  typename KernelType::DecompType decomposition = this->GetKernel().GetLines();
  BresType                        BresLine;

  using KernelLType = typename KernelType::LType;

  for (unsigned int i = 0; i < decomposition.size(); ++i)
  {
    const typename KernelType::LType     ThisLine = decomposition[i];
    const typename BresType::OffsetArray TheseOffsets = BresLine.BuildLine(ThisLine, bufflength);
    unsigned int                         SELength = GetLinePixels<KernelLType>(ThisLine);
    // want lines to be odd
    if (!(SELength % 2))
    {
      ++SELength;
    }

    const InputImageRegionType BigFace = MakeEnlargedFace<InputImageType, KernelLType>(input, IReg, ThisLine);

    DoFace<TImage, BresType, TFunction1, KernelLType>(
      input, output, m_Boundary, ThisLine, TheseOffsets, SELength, buffer, forward, reverse, IReg, BigFace);

    // after the first pass the input will be taken from the output
    input = internalbuffer;
    progress.Completed(IReg.GetNumberOfPixels());
  }

  // copy internal buffer to output
  ImageAlgorithm::Copy(input.GetPointer(), this->GetOutput(), OReg, OReg);
}

template <typename TImage, typename TKernel, typename TFunction1>
void
VanHerkGilWermanErodeDilateImageFilter<TImage, TKernel, TFunction1>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Boundary: " << m_Boundary << std::endl;
}

} // end namespace itk

#endif
