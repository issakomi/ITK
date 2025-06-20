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
#ifndef itkCentralDifferenceImageFunction_hxx
#define itkCentralDifferenceImageFunction_hxx

#include "itkLinearInterpolateImageFunction.h"

namespace itk
{

template <typename TInputImage, typename TCoordinate, typename TOutputType>
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::CentralDifferenceImageFunction()
  : m_Interpolator(LinearInterpolateImageFunction<TInputImage, TCoordinate>::New())
{}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
void
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::SetInputImage(const TInputImage * inputData)
{
  if (inputData != this->m_Image)
  {
    Superclass::SetInputImage(inputData);
    this->m_Interpolator->SetInputImage(inputData);

    // Verify the output vector is the right size.
    // OutputType of VariableLengthVector will have size 0 until allocated, so this
    // case can't be tested.
    if (inputData != nullptr)
    {
      const SizeValueType nComponents = OutputConvertType::GetNumberOfComponents();
      if (nComponents > 0)
      {
        if (nComponents != inputData->GetNumberOfComponentsPerPixel() * TInputImage::ImageDimension)
        {
          itkExceptionMacro("The OutputType is not the right size (" << nComponents << ") for the given pixel size ("
                                                                     << inputData->GetNumberOfComponentsPerPixel()
                                                                     << ") and image dimension ("
                                                                     << TInputImage::ImageDimension << ").");
        }
      }
    }
    this->Modified();
  }
}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
void
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::SetInterpolator(InterpolatorType * interpolator)
{
  if (interpolator != this->m_Interpolator)
  {
    this->m_Interpolator = interpolator;
    if (this->GetInputImage() != nullptr)
    {
      this->m_Interpolator->SetInputImage(this->GetInputImage());
    }
    this->Modified();
  }
}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
void
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::PrintSelf(std::ostream & os, Indent indent) const
{
  this->Superclass::PrintSelf(os, indent);

  itkPrintSelfBooleanMacro(UseImageDirection);

  itkPrintSelfObjectMacro(Interpolator);
}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
auto
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::EvaluateAtIndex(const IndexType & index) const
  -> OutputType
{
  OutputType derivative;

  // When ScalarDerivativeType is the same as OutputType, this calls
  // the version specialized for scalar pixels since in that case,
  // the two vector types are the same.
  EvaluateAtIndexSpecialized<ScalarDerivativeType>(
    index, derivative, OutputTypeSpecializationStructType<ScalarDerivativeType>());

  return derivative;
}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
template <typename Type>
void
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::EvaluateAtIndexSpecialized(
  const IndexType & index,
  OutputType &      orientedDerivative,
  OutputTypeSpecializationStructType<OutputType>) const
{
  OutputType derivative;

  IndexType neighIndex = index;

  const InputImageType * inputImage = this->GetInputImage();

  const typename InputImageType::RegionType & region = inputImage->GetBufferedRegion();

  const typename InputImageType::SizeType &  size = region.GetSize();
  const typename InputImageType::IndexType & start = region.GetIndex();

  const unsigned int MaxDims = Self::ImageDimension;
  for (unsigned int dim = 0; dim < MaxDims; ++dim)
  {
    // bounds checking
    // checks for index either on the boundary or out of bounds.
    // note that the documentation says this method assumes the index
    // is in-bounds, so we don't do anything else if the point is out of bounds.
    if (index[dim] < start[dim] + 1 || index[dim] > (start[dim] + static_cast<OffsetValueType>(size[dim]) - 2))
    {
      derivative[dim] = OutputValueType{};
      continue;
    }

    // compute derivative
    neighIndex[dim] += 1;
    derivative[dim] = inputImage->GetPixel(neighIndex);

    neighIndex[dim] -= 2;
    derivative[dim] -= inputImage->GetPixel(neighIndex);

    derivative[dim] *= static_cast<OutputValueType>(0.5) / inputImage->GetSpacing()[dim];
    neighIndex[dim] += 1;
  }

  if (this->m_UseImageDirection)
  {
    inputImage->TransformLocalVectorToPhysicalVector(derivative, orientedDerivative);
  }
  else
  {
    orientedDerivative = derivative;
  }
}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
template <typename Type>
void
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::EvaluateAtIndexSpecialized(
  const IndexType & index,
  OutputType &      derivative,
  OutputTypeSpecializationStructType<Type>) const
{
  const InputImageType * inputImage = this->GetInputImage();
  const unsigned int     numberComponents = this->GetInputImage()->GetNumberOfComponentsPerPixel();

  IndexType neighIndex = index;

  const typename InputImageType::RegionType & region = inputImage->GetBufferedRegion();
  const typename InputImageType::SizeType &   size = region.GetSize();
  const typename InputImageType::IndexType &  start = region.GetIndex();

  using PixelType = typename InputImageType::PixelType;
  const PixelType *  neighPixels[Self::ImageDimension][2];
  const PixelType    zeroPixel{};
  const unsigned int MaxDims = Self::ImageDimension;
  bool               dimOutOfBounds[Self::ImageDimension];

  for (unsigned int dim = 0; dim < MaxDims; ++dim)
  {
    // initialize to quiet compiler warnings
    neighPixels[dim][0] = &zeroPixel;
    neighPixels[dim][1] = &zeroPixel;

    // cached bounds checking
    dimOutOfBounds[dim] =
      ((index[dim] < (start[dim] + 1)) || index[dim] > (start[dim] + static_cast<OffsetValueType>(size[dim]) - 2));
  }

  for (unsigned int nc = 0; nc < numberComponents; ++nc)
  {
    ScalarDerivativeType componentDerivative;

    for (unsigned int dim = 0; dim < MaxDims; ++dim)
    {
      // bounds checking
      if (dimOutOfBounds[dim])
      {
        componentDerivative[dim] = OutputValueType{};
        continue;
      }

      // get pixels
      if (nc == 0)
      {
        neighIndex[dim] += 1;
        neighPixels[dim][0] = &(inputImage->GetPixel(neighIndex));
        neighIndex[dim] -= 2;
        neighPixels[dim][1] = &(inputImage->GetPixel(neighIndex));
        neighIndex[dim] += 1;
      }

      // compute derivative
      componentDerivative[dim] = InputPixelConvertType::GetNthComponent(nc, *neighPixels[dim][0]);
      componentDerivative[dim] -= InputPixelConvertType::GetNthComponent(nc, *neighPixels[dim][1]);
      componentDerivative[dim] *= 0.5 / inputImage->GetSpacing()[dim];
    }

    if (this->m_UseImageDirection)
    {
      const auto componentDerivativeOut = inputImage->TransformLocalVectorToPhysicalVector(componentDerivative);
      for (unsigned int dim = 0; dim < MaxDims; ++dim)
      {
        OutputConvertType::SetNthComponent(nc * MaxDims + dim, derivative, componentDerivativeOut[dim]);
      }
    }
    else
    {
      for (unsigned int dim = 0; dim < MaxDims; ++dim)
      {
        OutputConvertType::SetNthComponent(nc * MaxDims + dim, derivative, componentDerivative[dim]);
      }
    }
  }
}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
auto
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::Evaluate(const PointType & point) const
  -> OutputType
{
  OutputType derivative;

  // When ScalarDerivativeType is the same as OutputType, this calls
  // the version specialized for scalar pixels since in that case,
  // the two vector types are the same.
  EvaluateSpecialized<ScalarDerivativeType>(
    point, derivative, OutputTypeSpecializationStructType<ScalarDerivativeType>());

  return derivative;
}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
template <typename Type>
void
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::EvaluateSpecialized(
  const PointType & point,
  OutputType &      orientedDerivative,
  OutputTypeSpecializationStructType<OutputType>) const
{
  using PointValueType = typename PointType::ValueType;
  using DerivativeValueType = typename OutputType::ValueType;

  PointType neighPoint1 = point;
  PointType neighPoint2 = point;

  const InputImageType * inputImage = this->GetInputImage();

  const SpacingType & spacing = inputImage->GetSpacing();

  const unsigned int MaxDims = Self::ImageDimension;
  for (unsigned int dim = 0; dim < MaxDims; ++dim)
  {
    const PointValueType offset = static_cast<PointValueType>(0.5) * spacing[dim];
    // Check the bounds using the point because the image direction may swap dimensions,
    // making checks in index space inaccurate.
    // If on a boundary, we set the derivative to zero. This is done to match the behavior
    // of EvaluateAtIndex. Another approach is to calculate the 1-sided difference.
    neighPoint1[dim] = point[dim] - offset;
    if (!this->IsInsideBuffer(neighPoint1))
    {
      orientedDerivative[dim] = DerivativeValueType{};
      neighPoint1[dim] = point[dim];
      neighPoint2[dim] = point[dim];
      continue;
    }
    neighPoint2[dim] = point[dim] + offset;
    if (!this->IsInsideBuffer(neighPoint2))
    {
      orientedDerivative[dim] = DerivativeValueType{};
      neighPoint1[dim] = point[dim];
      neighPoint2[dim] = point[dim];
      continue;
    }

    const PointValueType delta = neighPoint2[dim] - neighPoint1[dim];
    if (delta > 10.0 * NumericTraits<PointValueType>::epsilon())
    {
      orientedDerivative[dim] =
        (this->m_Interpolator->Evaluate(neighPoint2) - this->m_Interpolator->Evaluate(neighPoint1)) / delta;
    }
    else
    {
      orientedDerivative[dim] = static_cast<DerivativeValueType>(0.0);
    }

    neighPoint1[dim] = point[dim];
    neighPoint2[dim] = point[dim];
  }

  // Since we've implicitly calculated the derivative with respect to image
  // direction, we need to reorient into index-space if the user desires.
  if (!this->m_UseImageDirection)
  {
    orientedDerivative = inputImage->TransformPhysicalVectorToLocalVector(orientedDerivative);
  }
}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
template <typename Type>
void
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::EvaluateSpecialized(
  const PointType & point,
  OutputType &      derivative,
  OutputTypeSpecializationStructType<Type>) const
{
  using PointValueType = typename PointType::ValueType;

  const InputImageType * inputImage = this->GetInputImage();
  const unsigned int     numberComponents = inputImage->GetNumberOfComponentsPerPixel();

  PointType neighPoint1 = point;
  PointType neighPoint2 = point;

  const SpacingType & spacing = inputImage->GetSpacing();

  using PixelType = typename InputImageType::PixelType;
  PixelType          neighPixels[Self::ImageDimension][2];
  bool               dimOutOfBounds[Self::ImageDimension];
  const unsigned int MaxDims = Self::ImageDimension;
  PointValueType     delta[Self::ImageDimension];
  const PixelType    zeroPixel{};

  ScalarDerivativeType componentDerivativeOut;
  ScalarDerivativeType componentDerivative{};

  for (unsigned int dim = 0; dim < Self::ImageDimension; ++dim)
  {
    // initialize to quiet compiler warnings
    neighPixels[dim][0] = zeroPixel;
    neighPixels[dim][1] = zeroPixel;
    delta[dim] = PointValueType{};
    dimOutOfBounds[dim] = true;
  }

  for (unsigned int nc = 0; nc < numberComponents; ++nc)
  {
    for (unsigned int dim = 0; dim < MaxDims; ++dim)
    {
      // Initialize values that only depend on dimension and not component number.
      if (nc == 0)
      {
        // Check the bounds using the point because the image direction may swap dimensions,
        // making checks in index space inaccurate.
        // If on a boundary, we set the derivative to zero. This is done to match the behavior
        // of EvaluateAtIndex. Another approach is to calculate the 1-sided difference.
        const PointValueType offset = static_cast<PointValueType>(0.5) * spacing[dim];
        neighPoint1[dim] = point[dim] - offset;
        neighPoint2[dim] = point[dim] + offset;
        dimOutOfBounds[dim] = (!this->IsInsideBuffer(neighPoint1) || !this->IsInsideBuffer(neighPoint2));

        if (dimOutOfBounds[dim])
        {
          componentDerivative[dim] = OutputValueType{};
          neighPoint1[dim] = point[dim];
          neighPoint2[dim] = point[dim];
          continue;
        }

        neighPixels[dim][0] = this->m_Interpolator->Evaluate(neighPoint2);
        neighPixels[dim][1] = this->m_Interpolator->Evaluate(neighPoint1);

        delta[dim] = neighPoint2[dim] - neighPoint1[dim];

        neighPoint1[dim] = point[dim];
        neighPoint2[dim] = point[dim];
      }
      else
      {
        if (dimOutOfBounds[dim])
        {
          continue;
        }
      }

      if (delta[dim] > 10.0 * NumericTraits<PointValueType>::epsilon())
      {
        const OutputValueType left = InputPixelConvertType::GetNthComponent(nc, neighPixels[dim][0]);
        const OutputValueType right = InputPixelConvertType::GetNthComponent(nc, neighPixels[dim][1]);
        componentDerivative[dim] = (left - right) / delta[dim];
      }
      else
      {
        componentDerivative[dim] = OutputValueType{};
      }
    }

    // Since we've implicitly calculated the derivative with respect to image
    // direction, we need to reorient into index-space if the user
    // desires.
    if (!this->m_UseImageDirection)
    {
      inputImage->TransformPhysicalVectorToLocalVector(componentDerivative, componentDerivativeOut);
      for (unsigned int dim = 0; dim < MaxDims; ++dim)
      {
        OutputConvertType::SetNthComponent(nc * MaxDims + dim, derivative, componentDerivativeOut[dim]);
      }
    }
    else
    {
      for (unsigned int dim = 0; dim < MaxDims; ++dim)
      {
        OutputConvertType::SetNthComponent(nc * MaxDims + dim, derivative, componentDerivative[dim]);
      }
    }
  }
}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
auto
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::EvaluateAtContinuousIndex(
  const ContinuousIndexType & cindex) const -> OutputType
{
  OutputType derivative;
  // When ScalarDerivativeType is the same as OutputType, this calls
  // the version specialized for scalar pixels since in that case,
  // the two vector types are the same.
  this->EvaluateAtContinuousIndexSpecialized<ScalarDerivativeType>(
    cindex, derivative, OutputTypeSpecializationStructType<ScalarDerivativeType>());
  return derivative;
}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
template <typename Type>
void
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::EvaluateAtContinuousIndexSpecialized(
  const ContinuousIndexType & cindex,
  OutputType &                orientedDerivative,
  OutputTypeSpecializationStructType<OutputType>) const
{
  using DerivativeValueType = typename OutputType::ValueType;
  using ContinuousIndexValueType = typename ContinuousIndexType::ValueType;

  OutputType derivative;

  ContinuousIndexType neighIndex = cindex;

  const InputImageType * inputImage = this->GetInputImage();

  const typename InputImageType::RegionType & region = inputImage->GetBufferedRegion();

  const typename InputImageType::SizeType &  size = region.GetSize();
  const typename InputImageType::IndexType & start = region.GetIndex();

  const unsigned int MaxDims = Self::ImageDimension;
  for (unsigned int dim = 0; dim < MaxDims; ++dim)
  {
    // bounds checking
    if (cindex[dim] < static_cast<ContinuousIndexValueType>(start[dim] + 1) ||
        cindex[dim] > static_cast<ContinuousIndexValueType>(start[dim] + static_cast<OffsetValueType>(size[dim]) - 2))
    {
      derivative[dim] = DerivativeValueType{};
      continue;
    }

    // compute derivative
    neighIndex[dim] += static_cast<ContinuousIndexValueType>(1.0);
    derivative[dim] = this->m_Interpolator->EvaluateAtContinuousIndex(neighIndex);

    neighIndex[dim] -= static_cast<ContinuousIndexValueType>(2.0);
    derivative[dim] -= this->m_Interpolator->EvaluateAtContinuousIndex(neighIndex);

    derivative[dim] *= static_cast<ContinuousIndexValueType>(0.5) / inputImage->GetSpacing()[dim];
    neighIndex[dim] += static_cast<ContinuousIndexValueType>(1.0);
  }

  if (this->m_UseImageDirection)
  {
    inputImage->TransformLocalVectorToPhysicalVector(derivative, orientedDerivative);
  }
  else
  {
    orientedDerivative = derivative;
  }
}

template <typename TInputImage, typename TCoordinate, typename TOutputType>
template <typename Type>
void
CentralDifferenceImageFunction<TInputImage, TCoordinate, TOutputType>::EvaluateAtContinuousIndexSpecialized(
  const ContinuousIndexType & cindex,
  OutputType &                derivative,
  OutputTypeSpecializationStructType<Type>) const
{
  using DerivativeValueType = typename OutputType::ValueType;
  using ContinuousIndexValueType = typename ContinuousIndexType::ValueType;

  const InputImageType * inputImage = this->GetInputImage();
  const unsigned int     numberComponents = inputImage->GetNumberOfComponentsPerPixel();

  ContinuousIndexType                         neighIndex = cindex;
  const typename InputImageType::RegionType & region = inputImage->GetBufferedRegion();

  const typename InputImageType::SizeType &  size = region.GetSize();
  const typename InputImageType::IndexType & start = region.GetIndex();

  using PixelType = typename InputImageType::PixelType;
  PixelType          neighPixels[Self::ImageDimension][2];
  bool               dimOutOfBounds[Self::ImageDimension];
  const unsigned int MaxDims = Self::ImageDimension;
  const PixelType    zeroPixel{};

  for (unsigned int dim = 0; dim < MaxDims; ++dim)
  {
    // initialize to quiet compiler warnings
    neighPixels[dim][0] = zeroPixel;
    neighPixels[dim][1] = zeroPixel;

    // bounds checking
    dimOutOfBounds[dim] =
      ((cindex[dim] < static_cast<ContinuousIndexValueType>(start[dim] + 1)) ||
       cindex[dim] > static_cast<ContinuousIndexValueType>(start[dim] + static_cast<OffsetValueType>(size[dim]) - 2));
  }

  for (unsigned int nc = 0; nc < numberComponents; ++nc)
  {
    ScalarDerivativeType componentDerivative;

    for (unsigned int dim = 0; dim < MaxDims; ++dim)
    {
      if (dimOutOfBounds[dim])
      {
        componentDerivative[dim] = DerivativeValueType{};
        continue;
      }

      // get pixels
      if (nc == 0)
      {
        neighIndex[dim] += static_cast<ContinuousIndexValueType>(1.0);
        neighPixels[dim][0] = this->m_Interpolator->EvaluateAtContinuousIndex(neighIndex);
        neighIndex[dim] -= static_cast<ContinuousIndexValueType>(2.0);
        neighPixels[dim][1] = this->m_Interpolator->EvaluateAtContinuousIndex(neighIndex);
        neighIndex[dim] += static_cast<ContinuousIndexValueType>(1.0);
      }

      // compute derivative
      componentDerivative[dim] = InputPixelConvertType::GetNthComponent(nc, neighPixels[dim][0]);
      componentDerivative[dim] -= InputPixelConvertType::GetNthComponent(nc, neighPixels[dim][1]);
      componentDerivative[dim] *= static_cast<ContinuousIndexValueType>(0.5) / inputImage->GetSpacing()[dim];
    }

    if (this->m_UseImageDirection)
    {
      const auto componentDerivativeOut = inputImage->TransformLocalVectorToPhysicalVector(componentDerivative);
      for (unsigned int dim = 0; dim < MaxDims; ++dim)
      {
        OutputConvertType::SetNthComponent(nc * MaxDims + dim, derivative, componentDerivativeOut[dim]);
      }
    }
    else
    {
      for (unsigned int dim = 0; dim < MaxDims; ++dim)
      {
        OutputConvertType::SetNthComponent(nc * MaxDims + dim, derivative, componentDerivative[dim]);
      }
    }
  }
}

} // end namespace itk

#endif
