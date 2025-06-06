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

#include "itkConstSliceIterator.h"
#include "itkImageRegionIterator.h"
#include "itkNeighborhoodIterator.h"
#include <iostream>

template <typename TPixelType, unsigned int VDimension>
void
FillRegionSequential(itk::SmartPointer<itk::Image<TPixelType, VDimension>> I)
{
  itk::Size<VDimension> Index;
  unsigned long         Location[VDimension];


  itk::ImageRegionIterator<itk::Image<TPixelType, VDimension>> data(I, I->GetRequestedRegion());

  Index = (I->GetRequestedRegion()).GetSize();

  unsigned int ArrayLength = 1;
  for (unsigned int iDim = 0; iDim < VDimension; ++iDim)
  {
    Location[iDim] = 0;
    ArrayLength *= Index[iDim];
  }

  for (unsigned int i = 0; i < ArrayLength; ++i, ++data)
  {
    TPixelType   value = 0;
    unsigned int mult = 1;
    for (unsigned int iDim = 0; iDim < VDimension; ++iDim, mult *= 10)
    {
      value += mult * Location[VDimension - iDim - 1];
    }
    data.Set(value);

    unsigned int iDim = VDimension - 1;
    bool         done = false;
    while (!done)
    {
      ++Location[iDim];
      if (Location[iDim] == Index[(VDimension - 1) - iDim])
      {
        Location[iDim] = 0;
      }
      else
      {
        done = true;
      }
      if (iDim == 0)
      {
        done = true;
      }
      else
      {
        --iDim;
      }
    }
  }
}

template <typename TPixelType, unsigned int VDimension>
void
PrintRegion(itk::SmartPointer<itk::Image<TPixelType, VDimension>> I)
{
  long rsz[VDimension];
  long Location[VDimension];

  std::copy(I->GetRequestedRegion().GetSize().m_InternalArray,
            I->GetRequestedRegion().GetSize().m_InternalArray + VDimension,
            rsz);
  std::fill_n(Location, VDimension, 0);

  for (unsigned int iDim = 0; iDim < VDimension; ++iDim)
  {
    std::cout << "iDim = " << iDim << std::endl;
    std::cout << "\tRegionSize = " << I->GetRequestedRegion().GetSize().m_InternalArray[iDim] << std::endl;
    std::cout << "\tRegionStartIndex = " << I->GetRequestedRegion().GetIndex()[iDim] << std::endl;
  }

  itk::ImageRegionIterator<itk::Image<TPixelType, VDimension>> iter(I, I->GetRequestedRegion());

  for (; !iter.IsAtEnd(); ++iter)
  {
    std::cout << iter.Get() << ' ';

    unsigned int iDim = VDimension - 1;
    bool         done = false;
    while (!done)
    {
      ++Location[iDim];
      if (Location[iDim] == rsz[(VDimension - 1) - iDim])
      {
        std::cout << std::endl;
        Location[iDim] = 0;
      }
      else
      {
        done = true;
      }
      if (iDim == 0)
      {
        done = true;
      }
      else
      {
        --iDim;
      }
    }
  }
}

template <typename TContainer>
void
PrintSlice(TContainer s_container)
{
  std::cout << '[';
  for (auto iter = s_container.Begin(); iter < s_container.End(); ++iter)
  {
    std::cout << *iter << ' ';
  }
  std::cout << ']' << std::endl;
}


int
itkSliceIteratorTest(int, char *[])
{

  // tests non-const slice iterator
  try
  {
    itk::ImageRegion<2> reg;
    itk::Size<2>        hoodRadius;
    itk::Size<2>        imgSize;
    itk::Index<2>       zeroIndex;
    zeroIndex[0] = zeroIndex[1] = 0;
    imgSize[0] = imgSize[1] = 20;
    hoodRadius[0] = hoodRadius[1] = 2;
    reg.SetIndex(zeroIndex);
    reg.SetSize(imgSize);

    const std::slice                                         hslice(10, 5, 1); // slice through the horizontal center
    const std::slice                                         vslice(2, 5, 5);  // slice through the vertical center
    itk::Neighborhood<int, 2>                                temp;
    const itk::SliceIterator<int, itk::Neighborhood<int, 2>> hnsi(&temp, hslice);
    const itk::SliceIterator<int, itk::Neighborhood<int, 2>> vnsi(&temp, vslice);
    const itk::ConstSliceIterator<int, itk::Neighborhood<int, 2>> hnsi2(&temp, hslice);
    const itk::ConstSliceIterator<int, itk::Neighborhood<int, 2>> vnsi2(&temp, vslice);

    itk::Neighborhood<int, 2> op;
    op.SetRadius(hoodRadius);

    itk::Index<2> idx;
    idx[0] = idx[1] = 0;

    const itk::Image<int, 2>::Pointer ip = itk::Image<int, 2>::New();
    ip->SetRegions(reg);
    ip->Allocate();

    FillRegionSequential<int, 2>(ip);
    PrintRegion<int, 2>(ip);

    itk::NeighborhoodIterator<itk::Image<int, 2>> it(hoodRadius, ip, reg);

    for (it.GoToBegin(); !it.IsAtEnd(); ++it)
    {
      temp = it.GetNeighborhood();
      temp.Print(std::cout);
      PrintSlice(hnsi);
      PrintSlice(vnsi);
      PrintSlice(hnsi2);
      PrintSlice(vnsi2);
    }
  }
  catch (const itk::ExceptionObject & err)
  {
    err.Print(std::cerr);
    return 2;
  }

  return EXIT_SUCCESS;
}
