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

#include "itkMaximumImageFilter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkTestingMacros.h"


int
itkMaximumImageFilterTest(int, char *[])
{

  // Define the dimension of the images
  constexpr unsigned int Dimension = 3;

  using PixelType = unsigned char;

  // Declare the types of the images
  using ImageType = itk::Image<PixelType, Dimension>;

  // Declare the type of the index to access images
  using IndexType = itk::Index<Dimension>;

  // Declare the type of the size
  using SizeType = itk::Size<Dimension>;

  // Declare the type of the region
  using RegionType = itk::ImageRegion<Dimension>;

  // Declare the type for the filter
  using MaximumImageFilterType = itk::MaximumImageFilter<ImageType, ImageType, ImageType>;

  // Create two images
  auto inputImageA = ImageType::New();
  auto inputImageB = ImageType::New();

  // Define their size, and start index
  SizeType size;
  size[0] = 2;
  size[1] = 2;
  size[2] = 2;

  IndexType start;
  start[0] = 0;
  start[1] = 0;
  start[2] = 0;

  RegionType region;
  region.SetIndex(start);
  region.SetSize(size);

  // Initialize Image A
  inputImageA->SetRegions(region);
  inputImageA->Allocate();

  // Initialize Image B
  inputImageB->SetRegions(region);
  inputImageB->Allocate();

  // Define the pixel values for each image
  constexpr PixelType largePixelValue = 3;
  constexpr PixelType smallPixelValue = 2;

  // Declare Iterator types apropriated for each image
  using IteratorType = itk::ImageRegionIteratorWithIndex<ImageType>;

  // Create one iterator for Image A (this is a light object)
  IteratorType it1(inputImageA, inputImageA->GetBufferedRegion());

  // Initialize the content of Image A
  while (!it1.IsAtEnd())
  {
    it1.Set(smallPixelValue);
    ++it1;
  }

  // Create one iterator for Image B (this is a light object)
  IteratorType it2(inputImageB, inputImageB->GetBufferedRegion());

  // Initialize the content of Image B
  while (!it2.IsAtEnd())
  {
    it2.Set(largePixelValue);
    ++it2;
  }

  // Create the filter
  auto maximumImageFilter = MaximumImageFilterType::New();

  ITK_EXERCISE_BASIC_OBJECT_METHODS(maximumImageFilter, MaximumImageFilter, BinaryGeneratorImageFilter);

  // Connect the input images
  maximumImageFilter->SetInput1(inputImageA);
  maximumImageFilter->SetInput2(inputImageB);

  // Get the Smart Pointer to the filter output
  const ImageType::Pointer outputImage = maximumImageFilter->GetOutput();


  // Execute the filter
  maximumImageFilter->Update();

  // Test some pixels in the result image
  // Note that we are not comparing the entirety of the filter output in order
  // to keep compile time as small as possible

  constexpr ImageType::IndexType pixelIndex = { { 0, 1, 1 } };

  ITK_TEST_EXPECT_EQUAL(outputImage->GetPixel(start), largePixelValue);
  ITK_TEST_EXPECT_EQUAL(outputImage->GetPixel(pixelIndex), largePixelValue);

  // All objects should be automatically destroyed at this point
  return EXIT_SUCCESS;
}
