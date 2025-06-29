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
#ifndef itkScatterMatrixImageFunction_hxx
#define itkScatterMatrixImageFunction_hxx

#include "itkConstNeighborhoodIterator.h"

namespace itk
{

template <typename TInputImage, typename TCoordinate>
ScatterMatrixImageFunction<TInputImage, TCoordinate>::ScatterMatrixImageFunction()
  : m_NeighborhoodRadius(1)
{}

template <typename TInputImage, typename TCoordinate>
void
ScatterMatrixImageFunction<TInputImage, TCoordinate>::PrintSelf(std::ostream & os, Indent indent) const
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NeighborhoodRadius: " << m_NeighborhoodRadius << std::endl;
}

template <typename TInputImage, typename TCoordinate>
auto
ScatterMatrixImageFunction<TInputImage, TCoordinate>::EvaluateAtIndex(const IndexType & index) const -> RealType
{
  RealType covariance;

  using PixelType = typename TInputImage::PixelType;
  using PixelComponentType = typename PixelType::ValueType;

  using PixelComponentRealType = typename NumericTraits<PixelComponentType>::RealType;

  const unsigned int VectorDimension = PixelType::Dimension;

  covariance = vnl_matrix<PixelComponentRealType>(VectorDimension, VectorDimension);
  covariance.fill(PixelComponentRealType{});

  if (!this->GetInputImage())
  {
    covariance.fill(NumericTraits<PixelComponentRealType>::max());
    return covariance;
  }

  if (!this->IsInsideBuffer(index))
  {
    covariance.fill(NumericTraits<PixelComponentRealType>::max());
    return covariance;
  }

  // Create an N-d neighborhood kernel, using a zeroflux boundary condition
  auto kernelSize = InputImageType::SizeType::Filled(m_NeighborhoodRadius);

  ConstNeighborhoodIterator<InputImageType> it(
    kernelSize, this->GetInputImage(), this->GetInputImage()->GetBufferedRegion());

  // Set the iterator at the desired location
  it.SetLocation(index);

  // Walk the neighborhood
  const unsigned int size = it.Size();
  for (unsigned int i = 0; i < size; ++i)
  {
    for (unsigned int dimx = 0; dimx < VectorDimension; ++dimx)
    {
      for (unsigned int dimy = 0; dimy < VectorDimension; ++dimy)
      {
        covariance[dimx][dimy] += static_cast<PixelComponentRealType>(it.GetPixel(i)[dimx]) *
                                  static_cast<PixelComponentRealType>(it.GetPixel(i)[dimy]);
      }
    }
  }
  for (unsigned int dimx = 0; dimx < VectorDimension; ++dimx)
  {
    for (unsigned int dimy = 0; dimy < VectorDimension; ++dimy)
    {
      covariance[dimx][dimy] /= static_cast<double>(size);
    }
  }

  return (covariance);
}
} // end namespace itk

#endif
