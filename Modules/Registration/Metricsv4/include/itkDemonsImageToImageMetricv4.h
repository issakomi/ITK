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
#ifndef itkDemonsImageToImageMetricv4_h
#define itkDemonsImageToImageMetricv4_h

#include "itkImageToImageMetricv4.h"

#include "itkDemonsImageToImageMetricv4GetValueAndDerivativeThreader.h"

namespace itk
{

/** \class DemonsImageToImageMetricv4
 *
 *  \brief Class implementing demons metric.
 *
 *  The implementation is taken from itkDemonsRegistrationFunction.
 *
 *  The metric derivative can be calculated using image derivatives
 *  either from the fixed or moving images. The default is to use fixed-image
 *  gradients. See ObjectToObjectMetric::SetGradientSource to change
 *  this behavior.
 *
 *  An intensity threshold is used, below which image pixels are considered
 *  equal for the purpose of derivative calculation. The threshold can be
 *  changed by calling SetIntensityDifferenceThreshold.
 *
 *  \note This metric supports only moving transforms with local support and
 *  with a number of local parameters that matches the moving image dimension.
 *  In particular, it's meant to be used with itkDisplacementFieldTransform and
 *  derived classes.
 *
 *  See DemonsImageToImageMetricv4GetValueAndDerivativeThreader::ProcessPoint
 *  for algorithm implementation.
 *
 * \sa itkImageToImageMetricv4
 * \ingroup ITKMetricsv4
 */
template <typename TFixedImage,
          typename TMovingImage,
          typename TVirtualImage = TFixedImage,
          typename TInternalComputationValueType = double,
          typename TMetricTraits =
            DefaultImageToImageMetricTraitsv4<TFixedImage, TMovingImage, TVirtualImage, TInternalComputationValueType>>
class ITK_TEMPLATE_EXPORT DemonsImageToImageMetricv4
  : public ImageToImageMetricv4<TFixedImage, TMovingImage, TVirtualImage, TInternalComputationValueType, TMetricTraits>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(DemonsImageToImageMetricv4);

  /** Standard class type aliases. */
  using Self = DemonsImageToImageMetricv4;
  using Superclass =
    ImageToImageMetricv4<TFixedImage, TMovingImage, TVirtualImage, TInternalComputationValueType, TMetricTraits>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(DemonsImageToImageMetricv4);

  /** Superclass types */
  using typename Superclass::MeasureType;
  using typename Superclass::DerivativeType;

  using typename Superclass::FixedImagePointType;
  using typename Superclass::FixedImagePixelType;
  using typename Superclass::FixedImageGradientType;

  using typename Superclass::MovingImagePointType;
  using typename Superclass::MovingImagePixelType;
  using typename Superclass::MovingImageGradientType;

  using typename Superclass::MovingTransformType;
  using typename Superclass::JacobianType;
  using VirtualImageType = typename Superclass::VirtualImageType;
  using typename Superclass::VirtualIndexType;
  using typename Superclass::VirtualPointType;
  using VirtualSPointSetType = typename Superclass::VirtualPointSetType;
  using typename Superclass::NumberOfParametersType;

  /** It should be possible to derive the internal computation type from the class object. */
  using InternalComputationValueType = TInternalComputationValueType;

  using typename Superclass::ImageDimensionType;

  /* Image dimension accessors */
  static constexpr ImageDimensionType VirtualImageDimension = TVirtualImage::ImageDimension;
  static constexpr ImageDimensionType FixedImageDimension = TFixedImage::ImageDimension;
  static constexpr ImageDimensionType MovingImageDimension = TMovingImage::ImageDimension;

  void
  Initialize() override;

  /** Accessors for the image intensity difference threshold use
   *  in derivative calculation */
  /** @ITKStartGrouping */
  itkGetConstMacro(IntensityDifferenceThreshold, TInternalComputationValueType);
  itkSetMacro(IntensityDifferenceThreshold, TInternalComputationValueType);
  /** @ITKEndGrouping */
  /** Get the denominator threshold used in derivative calculation. */
  itkGetConstMacro(DenominatorThreshold, TInternalComputationValueType);

protected:
  itkGetConstMacro(Normalizer, TInternalComputationValueType);

  DemonsImageToImageMetricv4();
  ~DemonsImageToImageMetricv4() override = default;

  friend class DemonsImageToImageMetricv4GetValueAndDerivativeThreader<
    ThreadedImageRegionPartitioner<Superclass::VirtualImageDimension>,
    Superclass,
    Self>;
  friend class DemonsImageToImageMetricv4GetValueAndDerivativeThreader<ThreadedIndexedContainerPartitioner,
                                                                       Superclass,
                                                                       Self>;
  using DemonsDenseGetValueAndDerivativeThreaderType = DemonsImageToImageMetricv4GetValueAndDerivativeThreader<
    ThreadedImageRegionPartitioner<Superclass::VirtualImageDimension>,
    Superclass,
    Self>;
  using DemonsSparseGetValueAndDerivativeThreaderType =
    DemonsImageToImageMetricv4GetValueAndDerivativeThreader<ThreadedIndexedContainerPartitioner, Superclass, Self>;

  void
  PrintSelf(std::ostream & os, Indent indent) const override;

private:
  /** Threshold below which the denominator term is considered zero.
   *  Fixed programmatically in constructor. */
  TInternalComputationValueType m_DenominatorThreshold{};

  /** Threshold below which two intensity value are assumed to match. */
  TInternalComputationValueType m_IntensityDifferenceThreshold{};

  /* Used to normalize derivative calculation. Automatically calculated */
  TInternalComputationValueType m_Normalizer{};
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkDemonsImageToImageMetricv4.hxx"
#endif

#endif
