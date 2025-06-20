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
#ifndef itkScalarToRGBColormapImageFilter_hxx
#define itkScalarToRGBColormapImageFilter_hxx


#include "itkImageRegionIterator.h"
#include "itkTotalProgressReporter.h"

#include "itkRedColormapFunction.h"
#include "itkGreenColormapFunction.h"
#include "itkBlueColormapFunction.h"
#include "itkGreyColormapFunction.h"
#include "itkHotColormapFunction.h"
#include "itkCoolColormapFunction.h"
#include "itkSpringColormapFunction.h"
#include "itkSummerColormapFunction.h"
#include "itkAutumnColormapFunction.h"
#include "itkWinterColormapFunction.h"
#include "itkCopperColormapFunction.h"
#include "itkHSVColormapFunction.h"
#include "itkJetColormapFunction.h"
#include "itkOverUnderColormapFunction.h"

/*
 *
 * This code was contributed in the Insight Journal paper:
 * "Meeting Andy Warhol Somewhere Over the Rainbow: RGB Colormapping and ITK"
 * by Tustison N., Zhang H., Lehmann G., Yushkevich P., Gee J.
 * https://doi.org/10.54294/tmojbk
 *
 */

namespace itk
{

template <typename TInputImage, typename TOutputImage>
ScalarToRGBColormapImageFilter<TInputImage, TOutputImage>::ScalarToRGBColormapImageFilter()
  : m_UseInputImageExtremaForScaling(true)
{
  this->SetNumberOfRequiredInputs(1);
  this->DynamicMultiThreadingOn();
  this->ThreaderUpdateProgressOff();

  using DefaultColormapType = Function::GreyColormapFunction<InputImagePixelType, OutputImagePixelType>;

  auto greyColormap = DefaultColormapType::New();
  this->SetColormap(greyColormap);
}

template <typename TInputImage, typename TOutputImage>
void
ScalarToRGBColormapImageFilter<TInputImage, TOutputImage>::BeforeThreadedGenerateData()
{
  if (this->m_UseInputImageExtremaForScaling)
  {
    ImageRegionConstIterator<InputImageType> It(this->GetInput(), this->GetInput()->GetRequestedRegion());

    InputImagePixelType minimumValue = NumericTraits<InputImagePixelType>::max();
    InputImagePixelType maximumValue = NumericTraits<InputImagePixelType>::min();

    for (It.GoToBegin(); !It.IsAtEnd(); ++It)
    {
      const InputImagePixelType value = It.Get();
      if (value < minimumValue)
      {
        minimumValue = value;
      }
      if (value > maximumValue)
      {
        maximumValue = value;
      }
    }

    this->m_Colormap->SetMinimumInputValue(minimumValue);
    this->m_Colormap->SetMaximumInputValue(maximumValue);
  }
}

template <typename TInputImage, typename TOutputImage>
void
ScalarToRGBColormapImageFilter<TInputImage, TOutputImage>::DynamicThreadedGenerateData(
  const OutputImageRegionType & outputRegionForThread)
{
  const InputImagePointer  inputPtr = this->GetInput();
  const OutputImagePointer outputPtr = this->GetOutput();

  TotalProgressReporter progressReporter(this, this->GetOutput()->GetRequestedRegion().GetNumberOfPixels());

  // Define the portion of the input to walk for this thread, using
  // the CallCopyOutputRegionToInputRegion method allows for the input
  // and output images to be different dimensions
  InputImageRegionType inputRegionForThread;

  this->CallCopyOutputRegionToInputRegion(inputRegionForThread, outputRegionForThread);

  ImageRegionConstIterator<TInputImage> inputIt(inputPtr, inputRegionForThread);
  ImageRegionIterator<TOutputImage>     outputIt(outputPtr, outputRegionForThread);

  while (!inputIt.IsAtEnd())
  {
    outputIt.Set(this->m_Colormap->operator()(inputIt.Get()));
    ++inputIt;
    ++outputIt;
    progressReporter.CompletedPixel();
  }
}

template <typename TInputImage, typename TOutputImage>
void
ScalarToRGBColormapImageFilter<TInputImage, TOutputImage>::SetColormap(RGBColormapFilterEnum map)
{
  switch (map)
  {
    case RGBColormapFilterEnum::Red:
    {
      using SpecificColormapType = Function::RedColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::Green:
    {
      using SpecificColormapType = Function::GreenColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::Blue:
    {
      using SpecificColormapType = Function::BlueColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::Grey:
    default:
    {
      using SpecificColormapType = Function::GreyColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::Hot:
    {
      using SpecificColormapType = Function::HotColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::Cool:
    {
      using SpecificColormapType = Function::CoolColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::Spring:
    {
      using SpecificColormapType = Function::SpringColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::Summer:
    {
      using SpecificColormapType = Function::SummerColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::Autumn:
    {
      using SpecificColormapType = Function::AutumnColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::Winter:
    {
      using SpecificColormapType = Function::WinterColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::Copper:
    {
      using SpecificColormapType = Function::CopperColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::Jet:
    {
      using SpecificColormapType = Function::JetColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::HSV:
    {
      using SpecificColormapType = Function::HSVColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
    case RGBColormapFilterEnum::OverUnder:
    {
      using SpecificColormapType = Function::OverUnderColormapFunction<InputImagePixelType, OutputImagePixelType>;
      auto colormap = SpecificColormapType::New();
      this->SetColormap(colormap);
      break;
    }
  }
}

template <typename TInputImage, typename TOutputImage>
void
ScalarToRGBColormapImageFilter<TInputImage, TOutputImage>::PrintSelf(std::ostream & os, Indent indent) const
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Class Name: " << this->GetNameOfClass() << std::endl;
  if (this->m_Colormap.IsNotNull())
  {
    os << indent << "Colormap " << this->m_Colormap << std::endl;
  }
  else
  {
    os << indent << "Colormap is nullptr " << std::endl;
  }
  os << indent << "Use Input Image Extrema for Scaling " << this->m_UseInputImageExtremaForScaling << std::endl;
}
} // end namespace itk

#endif
