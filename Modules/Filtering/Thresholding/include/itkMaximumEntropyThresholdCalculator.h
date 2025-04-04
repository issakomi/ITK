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

#ifndef itkMaximumEntropyThresholdCalculator_h
#define itkMaximumEntropyThresholdCalculator_h

#include "itkHistogramThresholdCalculator.h"

namespace itk
{

/**
 * \class MaximumEntropyThresholdCalculator
 * \brief Computes the MaximumEntropy's threshold for an image.
 *
 * Implements Kapur-Sahoo-Wong (Maximum Entropy) thresholding method
 * \cite kapur1985.
 *
 * This class is templated over the input histogram type.
 * \warning This calculator assumes that the input histogram has only one dimension.
 *
 * \author Richard Beare. Department of Medicine, Monash University,
 * Melbourne, Australia.
 * \author Gaetan Lehmann. Biologie du Developpement et de la Reproduction, INRA de Jouy-en-Josas, France.
 *
 * This implementation was taken from the Insight Journal paper:
 * https://doi.org/10.54294/efycla
 *
 * \ingroup Operators
 * \ingroup ITKThresholding
 */
template <typename THistogram, typename TOutput = double>
class ITK_TEMPLATE_EXPORT MaximumEntropyThresholdCalculator : public HistogramThresholdCalculator<THistogram, TOutput>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(MaximumEntropyThresholdCalculator);

  /** Standard class type aliases. */
  using Self = MaximumEntropyThresholdCalculator;
  using Superclass = HistogramThresholdCalculator<THistogram, TOutput>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(MaximumEntropyThresholdCalculator);

  /** Type definition for the input image. */
  using HistogramType = THistogram;
  using OutputType = TOutput;

protected:
  MaximumEntropyThresholdCalculator() = default;
  ~MaximumEntropyThresholdCalculator() override = default;
  void
  GenerateData() override;
};

} // end namespace itk


#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkMaximumEntropyThresholdCalculator.hxx"
#endif

#endif
