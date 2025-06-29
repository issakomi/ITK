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
#ifndef itkRegionBasedLevelSetFunctionData_hxx
#define itkRegionBasedLevelSetFunctionData_hxx


namespace itk
{
template <typename TInputImage, typename TFeatureImage>
RegionBasedLevelSetFunctionData<TInputImage, TFeatureImage>::RegionBasedLevelSetFunctionData()
  : m_HeavisideFunctionOfLevelSetImage(nullptr)
{}

template <typename TInputImage, typename TFeatureImage>
void
RegionBasedLevelSetFunctionData<TInputImage, TFeatureImage>::CreateHeavisideFunctionOfLevelSetImage(
  const InputImageType * image)
{
  const InputRegionType region = image->GetLargestPossibleRegion();

  this->m_HeavisideFunctionOfLevelSetImage = InputImageType::New();
  this->m_HeavisideFunctionOfLevelSetImage->CopyInformation(image);
  this->m_HeavisideFunctionOfLevelSetImage->SetRegions(region);
  this->m_HeavisideFunctionOfLevelSetImage->AllocateInitialized();

  const InputPointType origin = image->GetOrigin();

  this->m_Start = this->m_HeavisideFunctionOfLevelSetImage->TransformPhysicalPointToIndex(origin);

  for (unsigned int i = 0; i < ImageDimension; ++i)
  {
    this->m_End[i] = this->m_Start[i] + static_cast<InputIndexValueType>(region.GetSize()[i]) - 1;
  }
}

template <typename TInputImage, typename TFeatureImage>
auto
RegionBasedLevelSetFunctionData<TInputImage, TFeatureImage>::GetIndex(const FeatureIndexType & featureIndex)
  -> InputIndexType
{
  InputIndexType index;

  for (unsigned int i = 0; i < ImageDimension; ++i)
  {
    index[i] = featureIndex[i] - static_cast<InputIndexValueType>(this->m_Start[i]);
  }

  return index;
}

template <typename TInputImage, typename TFeatureImage>
auto
RegionBasedLevelSetFunctionData<TInputImage, TFeatureImage>::GetFeatureIndex(const InputIndexType & inputIndex)
  -> FeatureIndexType
{
  FeatureIndexType index;

  for (unsigned int i = 0; i < ImageDimension; ++i)
  {
    index[i] = inputIndex[i] + static_cast<InputIndexValueType>(this->m_Start[i]);
  }

  return index;
}
} // end namespace itk

#endif
