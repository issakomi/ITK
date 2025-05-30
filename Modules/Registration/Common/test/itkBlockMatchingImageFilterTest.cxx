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

#include <iostream>
#include "itkIndex.h"
#include "itkImage.h"
#include "itkRGBPixel.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionIterator.h"
#include "itkImageDuplicator.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkLineIterator.h"
#include "itkMultiThreaderBase.h"
#include "itkRegionOfInterestImageFilter.h"
#include "itkMaskFeaturePointSelectionFilter.h"
#include "itkBlockMatchingImageFilter.h"
#include "itkScalarToRGBColormapImageFilter.h"
#include "itkTranslationTransform.h"
#include "itkResampleImageFilter.h"
#include "itkTestingMacros.h"


int
itkBlockMatchingImageFilterTest(int argc, char * argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << std::endl;
    std::cerr << itkNameOfTestExecutableMacro(argv) << " inputImageFile outputImageFile [Mask File]" << std::endl;
    return EXIT_FAILURE;
  }

  constexpr double selectFraction = 0.01;

  using InputPixelType = unsigned char;
  using OutputPixelType = itk::RGBPixel<InputPixelType>;
  static constexpr unsigned int Dimension = 3;

  using InputImageType = itk::Image<InputPixelType, Dimension>;
  using OutputImageType = itk::Image<OutputPixelType, Dimension>;

  // Parameters used for FS and BM
  using RadiusType = InputImageType::SizeType;
  auto blockRadius = itk::MakeFilled<RadiusType>(2);

  auto searchRadius = itk::MakeFilled<RadiusType>(7);

  using ReaderType = itk::ImageFileReader<InputImageType>;

  // Set up the reader
  auto reader = ReaderType::New();
  reader->SetFileName(argv[1]);

  ITK_TRY_EXPECT_NO_EXCEPTION(reader->Update());


  // Reduce region of interest by SEARCH_RADIUS
  using RegionOfInterestFilterType = itk::RegionOfInterestImageFilter<InputImageType, InputImageType>;

  auto regionOfInterestFilter = RegionOfInterestFilterType::New();

  regionOfInterestFilter->SetInput(reader->GetOutput());

  RegionOfInterestFilterType::RegionType regionOfInterest = reader->GetOutput()->GetLargestPossibleRegion();

  RegionOfInterestFilterType::RegionType::IndexType regionOfInterestIndex = regionOfInterest.GetIndex();
  regionOfInterestIndex += searchRadius;
  regionOfInterest.SetIndex(regionOfInterestIndex);

  RegionOfInterestFilterType::RegionType::SizeType regionOfInterestSize = regionOfInterest.GetSize();
  regionOfInterestSize -= searchRadius + searchRadius;
  regionOfInterest.SetSize(regionOfInterestSize);

  regionOfInterestFilter->SetRegionOfInterest(regionOfInterest);
  regionOfInterestFilter->Update();

  using FeatureSelectionFilterType = itk::MaskFeaturePointSelectionFilter<InputImageType>;
  using PointSetType = FeatureSelectionFilterType::FeaturePointsType;

  // Feature Selection
  auto featureSelectionFilter = FeatureSelectionFilterType::New();

  featureSelectionFilter->SetInput(regionOfInterestFilter->GetOutput());
  featureSelectionFilter->SetSelectFraction(selectFraction);
  featureSelectionFilter->SetBlockRadius(blockRadius);
  featureSelectionFilter->ComputeStructureTensorsOff();

  // Create transformed image from input to match with
  using TranslationTransformType = itk::TranslationTransform<double, Dimension>;
  auto                                       transform = TranslationTransformType::New();
  TranslationTransformType::OutputVectorType translation;
  // move each pixel in input image 5 pixels along first(0) dimension
  translation[0] = 20.0;
  translation[1] = 0.0;
  translation[2] = 0.0;
  transform->Translate(translation);

  using ResampleImageFilterType = itk::ResampleImageFilter<InputImageType, InputImageType>;
  auto resampleFilter = ResampleImageFilterType::New();
  resampleFilter->SetTransform(transform);
  resampleFilter->SetInput(reader->GetOutput());
  resampleFilter->SetReferenceImage(reader->GetOutput());
  resampleFilter->UseReferenceImageOn();

  using BlockMatchingFilterType = itk::BlockMatchingImageFilter<InputImageType>;
  auto blockMatchingFilter = BlockMatchingFilterType::New();

  ITK_EXERCISE_BASIC_OBJECT_METHODS(blockMatchingFilter, BlockMatchingImageFilter, MeshToMeshFilter);


  // inputs (all required)
  blockMatchingFilter->SetFixedImage(resampleFilter->GetOutput());
  blockMatchingFilter->SetMovingImage(reader->GetOutput());
  blockMatchingFilter->SetFeaturePoints(featureSelectionFilter->GetOutput());

  // parameters (all optional)
  blockMatchingFilter->SetBlockRadius(blockRadius);
  ITK_TEST_SET_GET_VALUE(blockRadius, blockMatchingFilter->GetBlockRadius());

  blockMatchingFilter->SetSearchRadius(searchRadius);
  ITK_TEST_SET_GET_VALUE(searchRadius, blockMatchingFilter->GetSearchRadius());

  std::cout << "Block matching: " << blockMatchingFilter << std::endl;

  ITK_TRY_EXPECT_NO_EXCEPTION(blockMatchingFilter->Update());


  // Exercise the following methods
  BlockMatchingFilterType::DisplacementsType * displacements = blockMatchingFilter->GetDisplacements();
  if (displacements == nullptr)
  {
    std::cerr << "GetDisplacements() failed." << std::endl;
    return EXIT_FAILURE;
  }
  BlockMatchingFilterType::SimilaritiesType * similarities = blockMatchingFilter->GetSimilarities();
  if (similarities == nullptr)
  {
    std::cerr << "GetSimilarities() failed." << std::endl;
    return EXIT_FAILURE;
  }

  // create RGB copy of input image
  using RGBFilterType = itk::ScalarToRGBColormapImageFilter<InputImageType, OutputImageType>;
  auto colormapImageFilter = RGBFilterType::New();

  colormapImageFilter->SetColormap(itk::ScalarToRGBColormapImageFilterEnums::RGBColormapFilter::Grey);
  colormapImageFilter->SetInput(reader->GetOutput());

  ITK_TRY_EXPECT_NO_EXCEPTION(colormapImageFilter->Update());


  const OutputImageType::Pointer outputImage = colormapImageFilter->GetOutput();

  // Highlight the feature points identified in the output image
  using PointIteratorType = PointSetType::PointsContainer::ConstIterator;
  using PointDataIteratorType = BlockMatchingFilterType::DisplacementsType::PointDataContainer::ConstIterator;

  PointIteratorType       pointItr = featureSelectionFilter->GetOutput()->GetPoints()->Begin();
  const PointIteratorType pointEnd = featureSelectionFilter->GetOutput()->GetPoints()->End();
  PointDataIteratorType   displItr = displacements->GetPointData()->Begin();

  // define colors
  OutputPixelType red;
  red.SetRed(255);
  red.SetGreen(0);
  red.SetBlue(0);

  OutputPixelType green;
  green.SetRed(0);
  green.SetGreen(255);
  green.SetBlue(0);

  OutputPixelType blue;
  blue.SetRed(0);
  blue.SetGreen(0);
  blue.SetBlue(255);

  OutputImageType::IndexType index;
  while (pointItr != pointEnd)
  {
    if (outputImage->TransformPhysicalPointToIndex(pointItr.Value(), index))
    {
      const OutputImageType::IndexType displ =
        outputImage->TransformPhysicalPointToIndex(pointItr.Value() + displItr.Value());

      // draw line between old and new location of a point in blue
      itk::LineIterator<OutputImageType> lineIter(outputImage, index, displ);
      for (lineIter.GoToBegin(); !lineIter.IsAtEnd(); ++lineIter)
      {
        lineIter.Set(blue);
      }

      // mark old location of a point in green
      outputImage->SetPixel(index, green);

      // mark new location of a point in red
      outputImage->SetPixel(displ, red);
    }
    pointItr++;
    displItr++;
  }

  // Set up the writer
  using WriterType = itk::ImageFileWriter<OutputImageType>;
  auto writer = WriterType::New();

  writer->SetFileName(argv[2]);
  writer->SetInput(outputImage);

  ITK_TRY_EXPECT_NO_EXCEPTION(writer->Update());


  return EXIT_SUCCESS;
}
