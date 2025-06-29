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
#ifndef itkMinMaxCurvatureFlowFunction_hxx
#define itkMinMaxCurvatureFlowFunction_hxx

#include "itkMath.h"
#include "itkNeighborhoodInnerProduct.h"

namespace itk
{

template <typename TImage>
MinMaxCurvatureFlowFunction<TImage>::MinMaxCurvatureFlowFunction()
  : m_StencilRadius(0)
{
  this->SetStencilRadius(2);
}

template <typename TImage>
void
MinMaxCurvatureFlowFunction<TImage>::SetStencilRadius(const RadiusValueType value)
{
  if (m_StencilRadius == value)
  {
    return;
  }

  m_StencilRadius = (value > 1) ? value : 1;
  RadiusType radius;

  for (unsigned int j = 0; j < ImageDimension; ++j)
  {
    radius[j] = m_StencilRadius;
  }

  this->SetRadius(radius);
  this->InitializeStencilOperator();
}

template <typename TImage>
void
MinMaxCurvatureFlowFunction<TImage>::InitializeStencilOperator()
{
  // Fill stencil operator with a sphere of radius m_StencilRadius.

  m_StencilOperator.SetRadius(m_StencilRadius);

  RadiusValueType counter[ImageDimension];

  const RadiusValueType span = 2 * m_StencilRadius + 1;
  const RadiusValueType sqrRadius = m_StencilRadius * m_StencilRadius;
  for (unsigned int j = 0; j < ImageDimension; ++j)
  {
    counter[j] = 0;
  }

  using Iterator = typename StencilOperatorType::Iterator;
  Iterator opIter;
  Iterator opEnd = m_StencilOperator.End();

  SizeValueType numPixelsInSphere = 0;

  for (opIter = m_StencilOperator.Begin(); opIter < opEnd; ++opIter)
  {
    *opIter = PixelType{};

    RadiusValueType length = 0;
    for (unsigned int j = 0; j < ImageDimension; ++j)
    {
      length += static_cast<RadiusValueType>(
        itk::Math::sqr(static_cast<IndexValueType>(counter[j]) - static_cast<IndexValueType>(m_StencilRadius)));
    }
    if (length <= sqrRadius)
    {
      *opIter = 1;
      ++numPixelsInSphere;
    }

    bool carryOver = true;
    for (unsigned int j = 0; carryOver && j < ImageDimension; ++j)
    {
      counter[j] += 1;
      carryOver = false;
      if (counter[j] == span)
      {
        counter[j] = 0;
        carryOver = true;
      }
    }
  }

  // normalize the operator so that it sums to one
  if (numPixelsInSphere != 0)
  {
    for (opIter = m_StencilOperator.Begin(); opIter < opEnd; ++opIter)
    {
      *opIter = static_cast<PixelType>((double)*opIter / static_cast<double>(numPixelsInSphere));
    }
  }
}

template <typename TImage>
auto
MinMaxCurvatureFlowFunction<TImage>::ComputeThreshold(const DispatchBase &, const NeighborhoodType & it) const
  -> PixelType
{
  PixelType threshold{};

  // Compute gradient
  PixelType     gradient[ImageDimension];
  SizeValueType center = it.Size() / 2;

  auto gradMagnitude = PixelType{};
  for (unsigned int j = 0; j < ImageDimension; ++j)
  {
    SizeValueType stride = it.GetStride(static_cast<SizeValueType>(j));
    gradient[j] = 0.5 * (it.GetPixel(center + stride) - it.GetPixel(center - stride));
    gradient[j] *= this->m_ScaleCoefficients[j];

    gradMagnitude += itk::Math::sqr(static_cast<double>(gradient[j]));
  }

  if (gradMagnitude == 0.0)
  {
    return threshold;
  }

  gradMagnitude = std::sqrt(static_cast<double>(gradMagnitude));

  // Search for all position in the neighborhood perpendicular to
  // the gradient and at a distance of StencilRadius from center.

  RadiusValueType       counter[ImageDimension];
  const RadiusValueType span = 2 * m_StencilRadius + 1;
  for (unsigned int j = 0; j < ImageDimension; ++j)
  {
    counter[j] = 0;
  }

  using Iterator = typename NeighborhoodType::ConstIterator;
  Iterator neighIter;
  Iterator neighEnd = it.End();

  SizeValueType i = 0;
  SizeValueType numPixels = 0;

  for (neighIter = it.Begin(); neighIter < neighEnd; ++neighIter, ++i)
  {
    PixelType dotProduct{};
    PixelType vectorMagnitude{};

    for (unsigned int j = 0; j < ImageDimension; ++j)
    {
      const IndexValueType diff =
        static_cast<IndexValueType>(counter[j]) - static_cast<IndexValueType>(m_StencilRadius);

      dotProduct += static_cast<PixelType>(diff) * gradient[j];
      vectorMagnitude += static_cast<PixelType>(itk::Math::sqr(diff));
    }

    vectorMagnitude = std::sqrt(static_cast<double>(vectorMagnitude));

    if (vectorMagnitude != 0.0)
    {
      dotProduct /= gradMagnitude * vectorMagnitude;
    }

    if (vectorMagnitude >= m_StencilRadius && itk::Math::abs(dotProduct) < 0.262)
    {
      threshold += it.GetPixel(i);
      ++numPixels;
    }

    bool carryOver = true;
    for (unsigned int j = 0; carryOver && j < ImageDimension; ++j)
    {
      counter[j] += 1;
      carryOver = false;
      if (counter[j] == span)
      {
        counter[j] = 0;
        carryOver = true;
      }
    }
  }

  if (numPixels > 0)
  {
    threshold /= static_cast<PixelType>(numPixels);
  }

  return threshold;
}

template <typename TImage>
auto
MinMaxCurvatureFlowFunction<TImage>::ComputeThreshold(const Dispatch<2> &, const NeighborhoodType & it) const
  -> PixelType
{
  constexpr unsigned int imageDimension = 2;

  if (m_StencilRadius == 0)
  {
    return it.GetCenterPixel();
  }

  PixelType threshold{};

  // Compute gradient
  double        gradient[imageDimension];
  SizeValueType position[imageDimension];

  SizeValueType center = it.Size() / 2;

  gradient[0] = 0.5 * (it.GetPixel(center + 1) - it.GetPixel(center - 1));
  unsigned int k = 0;
  gradient[k] *= this->m_ScaleCoefficients[k];
  double gradMagnitude = Math::sqr(gradient[k]);
  ++k;

  SizeValueType stride = it.GetStride(1);
  gradient[k] = 0.5 * (it.GetPixel(center + stride) - it.GetPixel(center - stride));
  gradient[k] *= this->m_ScaleCoefficients[k];
  gradMagnitude += Math::sqr(gradient[k]);

  if (gradMagnitude == 0.0)
  {
    return threshold;
  }

  gradMagnitude = std::sqrt(static_cast<double>(gradMagnitude)) / static_cast<PixelType>(m_StencilRadius);

  for (double & j : gradient)
  {
    j /= gradMagnitude;
  }

  // Compute first perpendicular point
  position[0] = Math::Round<SizeValueType>(static_cast<double>(m_StencilRadius - gradient[1]));
  position[1] = Math::Round<SizeValueType>(static_cast<double>(m_StencilRadius + gradient[0]));

  threshold = it.GetPixel(position[0] + stride * position[1]);

  // Compute second perpendicular point
  position[0] = Math::Round<SizeValueType>(static_cast<double>(m_StencilRadius + gradient[1]));
  position[1] = Math::Round<SizeValueType>(static_cast<double>(m_StencilRadius - gradient[0]));

  threshold += it.GetPixel(position[0] + stride * position[1]);
  threshold *= 0.5;

  return threshold;
}

template <typename TImage>
auto
MinMaxCurvatureFlowFunction<TImage>::ComputeThreshold(const Dispatch<3> &, const NeighborhoodType & it) const
  -> PixelType
{
  constexpr unsigned int imageDimension = 3;

  if (m_StencilRadius == 0)
  {
    return it.GetCenterPixel();
  }

  PixelType threshold{};

  // Compute gradient
  double        gradient[imageDimension];
  SizeValueType position[imageDimension];

  SizeValueType center = it.Size() / 2;
  SizeValueType strideY = it.GetStride(1);
  SizeValueType strideZ = it.GetStride(2);

  gradient[0] = 0.5 * (it.GetPixel(center + 1) - it.GetPixel(center - 1));
  unsigned int k = 0;
  gradient[k] *= this->m_ScaleCoefficients[k];
  double gradMagnitude = itk::Math::sqr(gradient[k]);
  ++k;

  gradient[k] = 0.5 * (it.GetPixel(center + strideY) - it.GetPixel(center - strideY));
  gradient[k] *= this->m_ScaleCoefficients[k];
  gradMagnitude += itk::Math::sqr(gradient[k]);
  ++k;

  gradient[k] = 0.5 * (it.GetPixel(center + strideZ) - it.GetPixel(center - strideZ));
  gradient[k] *= this->m_ScaleCoefficients[k];
  gradMagnitude += Math::sqr(gradient[k]);

  if (gradMagnitude == 0.0)
  {
    return threshold;
  }

  gradMagnitude = std::sqrt(gradMagnitude) / static_cast<PixelType>(m_StencilRadius);

  for (double & j : gradient)
  {
    j /= gradMagnitude;
  }

  if (gradient[2] > 1.0)
  {
    gradient[2] = 1.0;
  }
  if (gradient[2] < -1.0)
  {
    gradient[2] = -1.0;
  }
  double theta = std::acos(gradient[2]);

  double phi = std::atan(gradient[1] / gradient[0]);
  if (Math::AlmostEquals(gradient[0], PixelType{}))
  {
    phi = Math::pi * 0.5;
  }

  const double cosTheta = std::cos(theta);
  const double sinTheta = std::sin(theta);
  const double cosPhi = std::cos(phi);
  const double sinPhi = std::sin(phi);

  const double rSinTheta = m_StencilRadius * sinTheta;
  const double rCosThetaCosPhi = m_StencilRadius * cosTheta * cosPhi;
  const double rCosThetaSinPhi = m_StencilRadius * cosTheta * sinPhi;
  const double rSinPhi = m_StencilRadius * sinPhi;
  const double rCosPhi = m_StencilRadius * cosPhi;

  // Point 1: angle = 0;
  position[0] = Math::Round<SizeValueType>(m_StencilRadius + rCosThetaCosPhi);
  position[1] = Math::Round<SizeValueType>(m_StencilRadius + rCosThetaSinPhi);
  position[2] = Math::Round<SizeValueType>(m_StencilRadius - rSinTheta);

  threshold += it.GetPixel(position[0] + strideY * position[1] + strideZ * position[2]);

  // Point 2: angle = 90;
  position[0] = Math::Round<SizeValueType>(m_StencilRadius - rSinPhi);
  position[1] = Math::Round<SizeValueType>(m_StencilRadius + rCosPhi);
  position[2] = m_StencilRadius;

  threshold += it.GetPixel(position[0] + strideY * position[1] + strideZ * position[2]);

  // Point 3: angle = 180;
  position[0] = Math::Round<SizeValueType>(m_StencilRadius - rCosThetaCosPhi);
  position[1] = Math::Round<SizeValueType>(m_StencilRadius - rCosThetaSinPhi);
  position[2] = Math::Round<SizeValueType>(m_StencilRadius + rSinTheta);

  threshold += it.GetPixel(position[0] + strideY * position[1] + strideZ * position[2]);

  // Point 4: angle = 270;
  position[0] = Math::Round<SizeValueType>(m_StencilRadius + rSinPhi);
  position[1] = Math::Round<SizeValueType>(m_StencilRadius - rCosPhi);
  position[2] = m_StencilRadius;

  threshold += it.GetPixel(position[0] + strideY * position[1] + strideZ * position[2]);

  threshold *= 0.25;
  return threshold;
}

template <typename TImage>
auto
MinMaxCurvatureFlowFunction<TImage>::ComputeUpdate(const NeighborhoodType & it,
                                                   void *                   globalData,
                                                   const FloatOffsetType &  offset) -> PixelType
{
  const PixelType update = this->Superclass::ComputeUpdate(it, globalData, offset);

  if (update == 0.0)
  {
    return update;
  }

  const PixelType threshold = this->ComputeThreshold(Dispatch<ImageDimension>(), it);

  const NeighborhoodInnerProduct<ImageType> innerProduct;
  const PixelType                           avgValue = innerProduct(it, m_StencilOperator);

  if (avgValue < threshold)
  {
    return (std::max(update, PixelType{}));
  }

  return (std::min(update, PixelType{}));
}
} // end namespace itk

#endif
