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
#ifndef itkMultiLabelSTAPLEImageFilter_h
#define itkMultiLabelSTAPLEImageFilter_h

#include "itkImage.h"
#include "itkImageToImageFilter.h"

#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"

#include "vector"
#include "itkArray.h"
#include "itkArray2D.h"
#include "itkNumericTraits.h"

namespace itk
{
/**
 * \class MultiLabelSTAPLEImageFilter
 *
 * \brief This filter performs a pixelwise combination of an arbitrary number
 * of input images, where each of them represents a segmentation of the same
 * scene (i.e., image).
 *
 * The labelings in the images are weighted relative to each other based on
 * their "performance" as estimated by an expectation-maximization
 * algorithm. In the process, a ground truth segmentation is estimated, and
 * the estimated performances of the individual segmentations are relative to
 * this estimated ground truth.
 *
 * The algorithm is based on the binary STAPLE algorithm by Warfield et al. as
 * published originally in \cite warfield2002.
 *
 * The multi-label algorithm implemented here is described in detail in
 * \cite rohlfing2004.
 *
 * \par INPUTS
 * All input volumes to this filter must be segmentations of an image,
 * that is, they must have discrete pixel values where each value represents
 * a different segmented object.
 *
 * Input volumes must all contain the same size RequestedRegions. Not all
 * input images must contain all possible labels, but all label values must
 * have the same meaning in all images.
 *
 * The filter can optionally be provided with estimates for the a priori class
 * probabilities through the SetPriorProbabilities function. If no estimate is
 * provided, one is automatically generated by analyzing the relative
 * frequencies of the labels in the input images.
 *
 * \par OUTPUTS
 * The filter produces a single output volume. Each output pixel
 * contains the label that has the highest probability of being the correct
 * label, based on the performance models of the individual segmentations.
 * If the maximum probability is not unique, i.e., if more than one label have
 * a maximum probability, then an "undecided" label is assigned to that output
 * pixel.
 *
 * By default, the label used for undecided pixels is the maximum label value
 * used in the input images plus one. Since it is possible for an image with
 * 8 bit pixel values to use all 256 possible label values, it is permissible
 * to combine 8 bit (i.e., byte) images into a 16 bit (i.e., short) output
 * image.
 *
 * In addition to the combined image, the estimated confusion matrices for
 * each of the input segmentations can be obtained through the
 * GetConfusionMatrix member function.
 *
 * \par PARAMETERS
 * The label used for "undecided" labels can be set using
 * SetLabelForUndecidedPixels. This functionality can be unset by calling
 * UnsetLabelForUndecidedPixels.
 *
 * A termination threshold for the EM iteration can be defined by calling
 * SetTerminationUpdateThreshold. The iteration terminates once no single
 * parameter of any confusion matrix changes by less than this
 * threshold. Alternatively, a maximum number of iterations can be specified
 * by calling SetMaximumNumberOfIterations. The algorithm may still terminate
 * after a smaller number of iterations if the termination threshold criterion
 * is satisfied.
 *
 * \par EVENTS
 * This filter invokes IterationEvent() at each iteration of the E-M
 * algorithm. Setting the AbortGenerateData() flag will cause the algorithm to
 * halt after the current iteration and produce results just as if it had
 * converged. The algorithm makes no attempt to report its progress since the
 * number of iterations needed cannot be known in advance.
 *
 * \author Torsten Rohlfing, SRI International, Neuroscience Program
 *
 * \ingroup ITKLabelVoting
 */
template <typename TInputImage, typename TOutputImage = TInputImage, typename TWeights = float>
class ITK_TEMPLATE_EXPORT MultiLabelSTAPLEImageFilter : public ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(MultiLabelSTAPLEImageFilter);

  /** Standard class type aliases. */
  using Self = MultiLabelSTAPLEImageFilter;
  using Superclass = ImageToImageFilter<TInputImage, TOutputImage>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(MultiLabelSTAPLEImageFilter);

  /** Extract some information from the image types.  Dimensionality
   * of the two images is assumed to be the same. */
  using OutputPixelType = typename TOutputImage::PixelType;
  using InputPixelType = typename TInputImage::PixelType;

  /** Extract some information from the image types.  Dimensionality
   * of the two images is assumed to be the same. */
  static constexpr unsigned int ImageDimension = TOutputImage::ImageDimension;

  /** Image type alias support */
  using InputImageType = TInputImage;
  using OutputImageType = TOutputImage;
  using InputImagePointer = typename InputImageType::Pointer;
  using OutputImagePointer = typename OutputImageType::Pointer;

  /** Superclass type alias. */
  using typename Superclass::OutputImageRegionType;

  /** Iterator types. */
  using InputConstIteratorType = ImageRegionConstIterator<TInputImage>;
  using OutputIteratorType = ImageRegionIterator<TOutputImage>;

  /** Confusion matrix type alias. */
  using WeightsType = TWeights;
  using ConfusionMatrixType = Array2D<WeightsType>;
  using PriorProbabilitiesType = Array<WeightsType>;

  /** Get the number of elapsed iterations of the iterative E-M algorithm. */
  itkGetConstMacro(ElapsedNumberOfIterations, unsigned int);

  /** Set maximum number of iterations.
   */
  /** @ITKStartGrouping */
  void
  SetMaximumNumberOfIterations(const unsigned int mit)
  {
    this->m_MaximumNumberOfIterations = mit;
    this->m_HasMaximumNumberOfIterations = true;
    this->Modified();
  }
  itkGetConstMacro(MaximumNumberOfIterations, unsigned int);
  /** @ITKEndGrouping */

  /** True if the MaximumNumberOfIterations has been manually set. **/
  itkGetConstMacro(HasMaximumNumberOfIterations, bool);

  /** Unset the maximum number of iterations, and rely on the TerminationUpdateThreshold.
   */
  void
  UnsetMaximumNumberOfIterations()
  {
    if (this->m_HasMaximumNumberOfIterations)
    {
      this->m_HasMaximumNumberOfIterations = false;
      this->Modified();
    }
  }

  /** Set termination threshold based on confusion matrix parameter updates.
   */
  /** @ITKStartGrouping */
  itkSetMacro(TerminationUpdateThreshold, TWeights);
  itkGetConstMacro(TerminationUpdateThreshold, TWeights);
  /** @ITKEndGrouping */

  /** Set label value for undecided pixels.
   */
  void
  SetLabelForUndecidedPixels(const OutputPixelType l)
  {
    this->m_LabelForUndecidedPixels = l;
    this->m_HasLabelForUndecidedPixels = true;
    this->Modified();
  }

  /** Get label value used for undecided pixels.
   *
   * After updating the filter, this function returns the actual label value
   * used for undecided pixels in the current output. Note that this value
   * is overwritten when SetLabelForUndecidedPixels is called and the new
   * value only becomes effective upon the next filter update.
   */
  itkGetMacro(LabelForUndecidedPixels, OutputPixelType);

  /** True if LabelForUndecidedPixels has been manually set. */
  itkGetMacro(HasLabelForUndecidedPixels, bool);

  /** Unset label value for undecided pixels and turn on automatic selection.
   */
  void
  UnsetLabelForUndecidedPixels()
  {
    if (this->m_HasLabelForUndecidedPixels)
    {
      this->m_HasLabelForUndecidedPixels = false;
      this->Modified();
    }
  }

  /** Set manual estimates for the a priori class probabilities.
   *
   * The size of the array must be greater than the value of the
   * largest label. The index into the array corresponds to the label
   * value in the segmented image for the class.
   */
  void
  SetPriorProbabilities(const PriorProbabilitiesType & ppa)
  {
    this->m_PriorProbabilities = ppa;
    this->m_HasPriorProbabilities = true;
    this->Modified();
  }

  /** Get prior class probabilities.
   *
   * After updating the filter, this function returns the actual prior class
   * probabilities. If these were not previously set by a call to
   * SetPriorProbabilities, then they are estimated from the input
   * segmentations and the result is available through this function.
   */
  itkGetConstReferenceMacro(PriorProbabilities, PriorProbabilitiesType);

  /** True if PriorProbabilities has been manually set. */
  itkGetMacro(HasPriorProbabilities, bool);

  /** Unset prior class probabilities and turn on automatic estimation.
   */
  void
  UnsetPriorProbabilities()
  {
    if (this->m_HasPriorProbabilities)
    {
      this->m_HasPriorProbabilities = false;
      this->Modified();
    }
  }

  /** Get confusion matrix for the i-th input segmentation.
   */
  const ConfusionMatrixType &
  GetConfusionMatrix(const unsigned int i) const
  {
    return this->m_ConfusionMatrixArray[i];
  }

protected:
  MultiLabelSTAPLEImageFilter()
    : m_LabelForUndecidedPixels(OutputPixelType{})
    , m_TerminationUpdateThreshold(1e-5)
  {}
  ~MultiLabelSTAPLEImageFilter() override = default;

  void
  GenerateData() override;

  void
  PrintSelf(std::ostream &, Indent) const override;

  /** Determine maximum value among all input images' pixels */
  typename TInputImage::PixelType
  ComputeMaximumInputValue();

  // Override since the filter needs all the data for the algorithm
  void
  GenerateInputRequestedRegion() override;

  // Override since the filter produces all of its output
  void
  EnlargeOutputRequestedRegion(DataObject *) override;

private:
  size_t m_TotalLabelCount{ 0 };

  OutputPixelType m_LabelForUndecidedPixels{};
  bool            m_HasLabelForUndecidedPixels{ false };

  bool                   m_HasPriorProbabilities{ false };
  PriorProbabilitiesType m_PriorProbabilities{};

  void
  InitializePriorProbabilities();

  std::vector<ConfusionMatrixType> m_ConfusionMatrixArray{};
  std::vector<ConfusionMatrixType> m_UpdatedConfusionMatrixArray{};

  void
  AllocateConfusionMatrixArray();
  void
  InitializeConfusionMatrixArrayFromVoting();

  bool         m_HasMaximumNumberOfIterations{ false };
  unsigned int m_MaximumNumberOfIterations{ 0 };
  unsigned int m_ElapsedNumberOfIterations{ 0u };

  TWeights m_TerminationUpdateThreshold{};
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkMultiLabelSTAPLEImageFilter.hxx"
#endif

#endif
