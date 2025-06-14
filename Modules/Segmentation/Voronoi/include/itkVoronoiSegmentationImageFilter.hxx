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
#ifndef itkVoronoiSegmentationImageFilter_hxx
#define itkVoronoiSegmentationImageFilter_hxx

#include "itkImageRegionIteratorWithIndex.h"

namespace itk
{

template <typename TInputImage, typename TOutputImage, typename TBinaryPriorImage>
void
VoronoiSegmentationImageFilter<TInputImage, TOutputImage, TBinaryPriorImage>::SetMeanPercentError(double x)
{
  m_MeanPercentError = x;
  m_MeanTolerance = x * m_Mean;
}

template <typename TInputImage, typename TOutputImage, typename TBinaryPriorImage>
void
VoronoiSegmentationImageFilter<TInputImage, TOutputImage, TBinaryPriorImage>::SetSTDPercentError(double x)
{
  m_STDPercentError = x;
  m_STDTolerance = x * m_STD;
}

template <typename TInputImage, typename TOutputImage, typename TBinaryPriorImage>
bool
VoronoiSegmentationImageFilter<TInputImage, TOutputImage, TBinaryPriorImage>::TestHomogeneity(IndexList & Plist)
{
  auto   num = static_cast<int>(Plist.size());
  double getp = NAN;
  double addp = 0;
  double addpp = 0;

  const InputImageType * inputImage = this->GetInput();

  for (int i = 0; i < num; ++i)
  {
    getp = static_cast<double>(inputImage->GetPixel(Plist[i]));
    addp = addp + getp;
    addpp = addpp + getp * getp;
  }

  double savemean = 0;
  double saveSTD = -1;
  if (num > 1)
  {
    savemean = addp / num;
    saveSTD = std::sqrt((addpp - (addp * addp) / (num)) / (num - 1));
  }

  //   // jvm - Mahalanobis distance
  //   if (savevar > 0 && itk::Math::abs(savemean - m_Mean) / m_Var < 2.5)
  //     return true;
  //   else
  //     return false;

  savemean -= m_Mean;
  saveSTD -= m_STD;
  if ((savemean > -m_MeanTolerance) && (savemean < m_MeanTolerance) && (saveSTD < m_STDTolerance))
  {
    return true;
  }

  return false;
}

template <typename TInputImage, typename TOutputImage, typename TBinaryPriorImage>
void
VoronoiSegmentationImageFilter<TInputImage, TOutputImage, TBinaryPriorImage>::TakeAPrior(
  const BinaryObjectImage * aprior)
{
  const RegionType region = this->GetInput()->GetRequestedRegion();

  itk::ImageRegionConstIteratorWithIndex<BinaryObjectImage> ait(aprior, region);
  itk::ImageRegionConstIteratorWithIndex<InputImageType>    iit(this->GetInput(), region);

  this->m_Size = this->GetInput()->GetRequestedRegion().GetSize();

  int   num = 0;
  float addp = 0;
  float addpp = 0;
  float currp = NAN;

  unsigned int minx = 0;
  unsigned int miny = 0;
  unsigned int maxx = 0;
  unsigned int maxy = 0;
  bool         status = false;
  for (unsigned int i = 0; i < this->m_Size[1]; ++i)
  {
    for (unsigned int j = 0; j < this->m_Size[0]; ++j)
    {
      if ((status == 0) && (ait.Get()))
      {
        miny = i;
        minx = j;
        maxy = i;
        maxx = j;
        status = true;
      }
      else if ((status == 1) && (ait.Get()))
      {
        maxy = i;
        if (minx > j)
        {
          minx = j;
        }
        if (maxx < j)
        {
          maxx = j;
        }
      }
      ++ait;
    }
  }

  float addb = 0;
  int   numb = 0;

  ait.GoToBegin();
  iit.GoToBegin();
  for (unsigned int i = 0; i < miny; ++i)
  {
    for (unsigned int j = 0; j < this->m_Size[0]; ++j)
    {
      ++ait;
      ++iit;
    }
  }
  for (unsigned int i = miny; i <= maxy; ++i)
  {
    for (unsigned int j = 0; j < minx; ++j)
    {
      ++ait;
      ++iit;
    }
    for (unsigned int j = minx; j <= maxx; ++j)
    {
      if (ait.Get())
      {
        ++num;
        currp = static_cast<float>(iit.Get());
        addp += currp;
        addpp += currp * currp;
      }
      else
      {
        ++numb;
        currp = static_cast<float>(iit.Get());
        addb += currp;
      }
      ++ait;
      ++iit;
    }
    for (unsigned int j = maxx + 1; j < this->m_Size[0]; ++j)
    {
      ++ait;
      ++iit;
    }
  }

  m_Mean = addp / num;
  m_STD = std::sqrt((addpp - (addp * addp) / num) / (num - 1));
  const float b_Mean = addb / numb;

  if (this->GetUseBackgroundInAPrior())
  {
    m_MeanTolerance = itk::Math::abs(m_Mean - b_Mean) * this->GetMeanDeviation();
  }
  else
  {
    m_MeanTolerance = m_Mean * m_MeanPercentError;
  }
  m_STDTolerance = m_STD * m_STDPercentError;
}

template <typename TInputImage, typename TOutputImage, typename TBinaryPriorImage>
void
VoronoiSegmentationImageFilter<TInputImage, TOutputImage, TBinaryPriorImage>::PrintSelf(std::ostream & os,
                                                                                        Indent         indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Mean: " << m_Mean << std::endl;
  os << indent << "STD: " << m_STD << std::endl;
  os << indent << "MeanTolerance: " << m_MeanTolerance << std::endl;
  os << indent << "STDTolerance: " << m_STDTolerance << std::endl;
  os << indent << "MeanPercentError: " << m_MeanPercentError << std::endl;
  os << indent << "STDPercentError: " << m_STDPercentError << std::endl;
}
} // namespace itk

#endif
