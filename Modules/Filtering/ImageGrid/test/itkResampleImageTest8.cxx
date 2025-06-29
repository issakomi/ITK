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

#include "itkResampleImageFilter.h"
#include "itkImageFileWriter.h"
#include "itkMath.h"
#include "itkTestingMacros.h"
#include "itkIdentityTransform.h"

class ProjectTransform : public itk::Transform<double, 3, 2>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(ProjectTransform);

  using Self = ProjectTransform;
  using Superclass = itk::Transform<double, 3, 2>;
  using Pointer = itk::SmartPointer<Self>;
  using ConstPointer = itk::SmartPointer<const Self>;

  itkNewMacro(Self);

  /** Standard parameters container. */
  using typename Superclass::ParametersType;

  /** Fixed Parameter type */
  using typename Superclass::FixedParametersType;

  /** Standard vector type for this class. */
  using InputVectorType = itk::Vector<double, Self::InputSpaceDimension>;
  using OutputVectorType = itk::Vector<double, Self::OutputSpaceDimension>;

  /** Standard covariant vector type for this class. */
  using InputCovariantVectorType = itk::CovariantVector<double, Self::InputSpaceDimension>;
  using OutputCovariantVectorType = itk::CovariantVector<double, Self::OutputSpaceDimension>;

  /** Standard vnl_vector type for this class. */
  using InputVnlVectorType = vnl_vector_fixed<double, Self::InputSpaceDimension>;
  using OutputVnlVectorType = vnl_vector_fixed<double, Self::OutputSpaceDimension>;

  /** Standard coordinate point type for this class. */
  using InputPointType = itk::Point<double, Self::InputSpaceDimension>;
  using OutputPointType = itk::Point<double, Self::OutputSpaceDimension>;

  using Superclass::TransformCovariantVector;
  using Superclass::TransformVector;
  using OutputVectorPixelType = Superclass::OutputVectorPixelType;
  using InputVectorPixelType = Superclass::InputVectorPixelType;

  OutputVectorType
  TransformVector(const InputVectorType & itkNotUsed(vector)) const override
  {
    return {};
  }
  OutputVnlVectorType
  TransformVector(const InputVnlVectorType & itkNotUsed(vector)) const override
  {
    return {};
  }
  OutputVectorPixelType
  TransformVector(const InputVectorPixelType & itkNotUsed(inputPixel),
                  const InputPointType &       itkNotUsed(inputPoint)) const override
  {
    return {};
  }
  OutputCovariantVectorType
  TransformCovariantVector(const InputCovariantVectorType & itkNotUsed(vector)) const override
  {
    return {};
  }
  void
  ComputeJacobianWithRespectToParameters(const InputPointType &, JacobianType &) const override
  {}
  void
  SetParameters(const ParametersType &) override
  {}
  void
  SetFixedParameters(const FixedParametersType &) override
  {}


  OutputPointType
  TransformPoint(const InputPointType & inputPoint) const override
  {
    auto outputPoint =
      itk::MakeFilled<OutputPointType>(std::numeric_limits<typename OutputPointType::ValueType>::max());
    for (unsigned int d = 0; d < 2; ++d)
    {
      outputPoint[d] = inputPoint[d] * 0.5;
    }
    return outputPoint;
  }

protected:
  ProjectTransform() = default;
  ~ProjectTransform() override = default;

}; // class ProjectTransform


int
itkResampleImageTest8(int, char *[])
{

  constexpr unsigned int InputImageDimensions = 2;
  constexpr unsigned int OutputImageDimensions = 3;

  using PixelType = float;

  using InputImageType = itk::Image<PixelType, InputImageDimensions>;
  using OutputImageType = itk::Image<PixelType, OutputImageDimensions>;
  using InputImageIndexType = InputImageType::IndexType;
  using InputImagePointerType = InputImageType::Pointer;
  using InputImageRegionType = InputImageType::RegionType;
  using InputImageSizeType = InputImageType::SizeType;

  using OutputImageIndexType = OutputImageType::IndexType;
  using OutputImageSizeType = OutputImageType::SizeType;

  using CoordinateType = double;

  using TransformType = ProjectTransform;
  using InterpolatorType = itk::LinearInterpolateImageFunction<InputImageType, CoordinateType>;

  std::cout << "Input Image Type\n";

  // Create and configure an image
  const InputImagePointerType  inputImage = InputImageType::New();
  InputImageIndexType          inputIndex = { { 0, 0 } };
  constexpr InputImageSizeType inputSize = { { 18, 12 } };
  const InputImageRegionType   inputRegion{ inputIndex, inputSize };
  inputImage->SetLargestPossibleRegion(inputRegion);
  inputImage->SetBufferedRegion(inputRegion);
  inputImage->Allocate();

  // Fill input image with a ramp

  for (itk::ImageRegionIteratorWithIndex<InputImageType> iter(inputImage, inputRegion); !iter.IsAtEnd(); ++iter)
  {
    inputIndex = iter.GetIndex();
    PixelType value = inputIndex[0] + inputIndex[1];
    iter.Set(value);
  }

  // Create an Project transformation
  auto tform = TransformType::New();

  // OutputImagePointerType outputImage = OutputImageType::New();
  OutputImageIndexType          outputIndex = { { 0, 0, 0 } };
  constexpr OutputImageSizeType outputSize = { { 18, 12, 5 } };

  // Create a linear interpolation image function
  auto interp = InterpolatorType::New();
  interp->SetInputImage(inputImage);

  // Create and configure a resampling filter
  const itk::ResampleImageFilter<InputImageType, OutputImageType>::Pointer resample =
    itk::ResampleImageFilter<InputImageType, OutputImageType>::New();

  ITK_EXERCISE_BASIC_OBJECT_METHODS(resample, ResampleImageFilter, ImageToImageFilter);

  resample->SetInput(inputImage);
  ITK_TEST_SET_GET_VALUE(inputImage, resample->GetInput());


  resample->SetTransform(tform);
  ITK_TEST_SET_GET_VALUE(tform, resample->GetTransform());

  resample->SetInterpolator(interp);
  ITK_TEST_SET_GET_VALUE(interp, resample->GetInterpolator());

  resample->SetSize(outputSize);
  ITK_TEST_SET_GET_VALUE(outputSize, resample->GetSize());

  outputIndex.Fill(0);
  resample->SetOutputStartIndex(outputIndex);
  ITK_TEST_SET_GET_VALUE(outputIndex, resample->GetOutputStartIndex());

  // Run the resampling filter
  resample->Update();

  // Check if desired results were obtained
  bool                              passed = true;
  const OutputImageType::RegionType region2 = resample->GetOutput()->GetRequestedRegion();
  constexpr double                  tolerance = 1e-30;
  for (itk::ImageRegionIteratorWithIndex<OutputImageType> iter2(resample->GetOutput(), region2); !iter2.IsAtEnd();
       ++iter2)
  {
    outputIndex = iter2.GetIndex();
    PixelType       value = iter2.Get();
    const PixelType pixval = value;
    auto            expectedValue = static_cast<PixelType>((outputIndex[0] + outputIndex[1]) / 2.0);
    if (!itk::Math::FloatAlmostEqual(expectedValue, pixval, 10, tolerance))
    {
      std::cout << "Error in resampled image: Pixel " << outputIndex << "value    = " << value << "  "
                << "pixval   = " << pixval << "  "
                << "expected = " << expectedValue << std::endl;
      passed = false;
    }
  }

  // Test non-default values
  constexpr auto origin = itk::MakeFilled<OutputImageType::PointType>(1234.0);
  resample->SetOutputOrigin(origin);
  ITK_TEST_SET_GET_VALUE(origin, resample->GetOutputOrigin());

  auto spacing = itk::MakeFilled<OutputImageType::SpacingType>(9876.0);
  resample->SetOutputSpacing(spacing);
  ITK_TEST_SET_GET_VALUE(spacing, resample->GetOutputSpacing());


  // Report success or failure
  if (!passed)
  {
    std::cout << "Resampling test failed" << std::endl;
    return EXIT_FAILURE;
  }

  // Exercise error handling

  try
  {
    std::cout << "Setting interpolator to nullptr" << std::endl;
    passed = false;
    resample->SetInterpolator(nullptr);
    resample->Update();
  }
  catch (const itk::ExceptionObject & err)
  {
    std::cout << err << std::endl;
    passed = true;
    resample->ResetPipeline();
    resample->SetInterpolator(interp);
  }

  if (!passed)
  {
    std::cout << "Resampling test failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Test passed." << std::endl;
  return EXIT_SUCCESS;
}
