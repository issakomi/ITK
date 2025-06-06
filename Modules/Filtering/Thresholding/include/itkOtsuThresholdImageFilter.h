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

#ifndef itkOtsuThresholdImageFilter_h
#define itkOtsuThresholdImageFilter_h

#include "itkHistogramThresholdImageFilter.h"
#include "itkOtsuThresholdCalculator.h"

namespace itk
{

/**
 * \class OtsuThresholdImageFilter
 * \brief Threshold an image using the Otsu Threshold
 *
 * This filter creates a binary thresholded image that separates an
 * image into foreground and background components. The filter
 * computes the threshold using the OtsuThresholdCalculator and
 * applies that threshold to the input image using the
 * BinaryThresholdImageFilter.
 *
 * \author Richard Beare
 * \author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.
 *
 * This implementation was taken from the Insight Journal paper:
 * https://doi.org/10.54294/efycla
 *
 * \sa HistogramThresholdImageFilter
 *
 * \ingroup Multithreaded
 * \ingroup ITKThresholding
 *
 * \sphinx
 * \sphinxexample{Filtering/Thresholding/SeparateGroundUsingOtsu,Separate Foreground And Background Using Otsu Method}
 * \endsphinx
 */

template <typename TInputImage, typename TOutputImage, typename TMaskImage = TOutputImage>
class ITK_TEMPLATE_EXPORT OtsuThresholdImageFilter
  : public HistogramThresholdImageFilter<TInputImage, TOutputImage, TMaskImage>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(OtsuThresholdImageFilter);

  /** Standard Self type alias */
  using Self = OtsuThresholdImageFilter;
  using Superclass = HistogramThresholdImageFilter<TInputImage, TOutputImage, TMaskImage>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(OtsuThresholdImageFilter);

  using InputImageType = TInputImage;
  using OutputImageType = TOutputImage;
  using MaskImageType = TMaskImage;

  /** Image pixel value type alias. */
  using InputPixelType = typename InputImageType::PixelType;
  using OutputPixelType = typename OutputImageType::PixelType;
  using MaskPixelType = typename MaskImageType::PixelType;

  /** Image related type alias. */
  using InputImagePointer = typename InputImageType::Pointer;
  using OutputImagePointer = typename OutputImageType::Pointer;

  using InputSizeType = typename InputImageType::SizeType;
  using InputIndexType = typename InputImageType::IndexType;
  using InputImageRegionType = typename InputImageType::RegionType;
  using OutputSizeType = typename OutputImageType::SizeType;
  using OutputIndexType = typename OutputImageType::IndexType;
  using OutputImageRegionType = typename OutputImageType::RegionType;
  using MaskSizeType = typename MaskImageType::SizeType;
  using MaskIndexType = typename MaskImageType::IndexType;
  using MaskImageRegionType = typename MaskImageType::RegionType;

  using typename Superclass::HistogramType;
  using CalculatorType = OtsuThresholdCalculator<HistogramType, InputPixelType>;

  /** Image related type alias. */
  static constexpr unsigned int InputImageDimension = InputImageType::ImageDimension;
  static constexpr unsigned int OutputImageDimension = OutputImageType::ImageDimension;

  /** Should the threshold value be mid-point of the bin or the maximum?
   * Default is to return bin maximum. */
  /** @ITKStartGrouping */
  itkSetMacro(ReturnBinMidpoint, bool);
  itkGetConstReferenceMacro(ReturnBinMidpoint, bool);
  itkBooleanMacro(ReturnBinMidpoint);
  /** @ITKEndGrouping */
protected:
  OtsuThresholdImageFilter() { this->SetCalculator(CalculatorType::New()); }
  ~OtsuThresholdImageFilter() override = default;

  void
  GenerateData() override
  {
    auto calc = static_cast<CalculatorType *>(this->GetModifiableCalculator());
    calc->SetReturnBinMidpoint(m_ReturnBinMidpoint);
    this->Superclass::GenerateData();
  }

  void
  VerifyPreconditions() const override
  {
    Superclass::VerifyPreconditions();
    if (dynamic_cast<const CalculatorType *>(Superclass::GetCalculator()) == nullptr)
    {
      itkExceptionMacro("Invalid OtsuThresholdCalculator.");
    }
  }

private:
  bool m_ReturnBinMidpoint{ false };
};

} // end namespace itk

#endif
