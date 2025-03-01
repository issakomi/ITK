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

#include "itkAnnulusOperator.h"
#include "itkStdStreamStateSave.h"
#include "itkTestingMacros.h"

int
itkAnnulusOperatorTest(int, char *[])
{
  // Save the format stream variables for std::cout
  // They will be restored when coutState goes out of scope
  // scope.
  const itk::StdStreamStateSave coutState(std::cout);

  constexpr unsigned int Dimension = 2;
  using PixelType = float;
  using OperatorType = itk::AnnulusOperator<PixelType, Dimension>;

  OperatorType normalizedAnnulus;

  ITK_EXERCISE_BASIC_OBJECT_METHODS((&normalizedAnnulus), AnnulusOperator, NeighborhoodOperator);


  normalizedAnnulus.NormalizeOn();
  normalizedAnnulus.SetInnerRadius(3);
  normalizedAnnulus.SetThickness(2);

  constexpr bool brightCenter = false;
  ITK_TEST_SET_GET_BOOLEAN((&normalizedAnnulus), BrightCenter, brightCenter);

  try
  {
    normalizedAnnulus.CreateOperator();
  }
  catch (const itk::ExceptionObject & e)
  {
    std::cout << e;
    return EXIT_FAILURE;
  }
  catch (const std::exception & e)
  {
    std::cout << "Std exception" << e.what();
    return EXIT_FAILURE;
  }
  catch (...)
  {
    std::cout << "Unknown exception" << std::endl;
    return EXIT_FAILURE;
  }

  OperatorType::SizeType normalizedAnnulusSize;
  normalizedAnnulusSize = normalizedAnnulus.GetSize();

  std::cout << std::endl;
  std::cout << "Annulus, N = " << normalizedAnnulusSize << ", r = " << normalizedAnnulus.GetInnerRadius()
            << ", t = " << normalizedAnnulus.GetThickness() << std::endl;

  for (unsigned int i = 0, k = 0; i < normalizedAnnulusSize[1]; ++i)
  {
    for (unsigned int j = 0; j < normalizedAnnulusSize[0]; ++j, ++k)
    {
      std::cout.width(6);
      std::cout.precision(3);
      std::cout << normalizedAnnulus[k] << '\t';
    }
    std::cout << std::endl;
  }

  // Compute again having the center bright
  std::cout << std::endl << std::endl;
  std::cout << "Rebuilding annulus with a bright center";
  normalizedAnnulus.BrightCenterOn();
  normalizedAnnulus.CreateOperator();
  std::cout << ", N = " << normalizedAnnulusSize << ", r = " << normalizedAnnulus.GetInnerRadius()
            << ", t = " << normalizedAnnulus.GetThickness() << std::endl;


  for (unsigned int i = 0, k = 0; i < normalizedAnnulusSize[1]; ++i)
  {
    for (unsigned int j = 0; j < normalizedAnnulusSize[0]; ++j, ++k)
    {
      std::cout.width(6);
      std::cout.precision(3);
      std::cout << normalizedAnnulus[k] << '\t';
    }
    std::cout << std::endl;
  }

  // Compute an annulus by hand
  std::cout << std::endl << std::endl;
  std::cout << "Creating an annulus by specifying values";
  OperatorType annulus;

  constexpr bool normalize = false;
  ITK_TEST_SET_GET_BOOLEAN((&annulus), Normalize, normalize);

  constexpr double innerRadius = 2;
  annulus.SetInnerRadius(innerRadius);
  ITK_TEST_SET_GET_VALUE(innerRadius, annulus.GetInnerRadius());

  constexpr double thickness = 1;
  annulus.SetThickness(thickness);
  ITK_TEST_SET_GET_VALUE(thickness, annulus.GetThickness());

  constexpr typename OperatorType::PixelType exteriorValue = 1;
  annulus.SetExteriorValue(exteriorValue);
  ITK_TEST_SET_GET_VALUE(exteriorValue, annulus.GetExteriorValue());

  constexpr typename OperatorType::PixelType annulusValue = 8;
  annulus.SetAnnulusValue(annulusValue);
  ITK_TEST_SET_GET_VALUE(annulusValue, annulus.GetAnnulusValue());

  constexpr typename OperatorType::PixelType interiorValue = 4;
  annulus.SetInteriorValue(interiorValue);
  ITK_TEST_SET_GET_VALUE(interiorValue, annulus.GetInteriorValue());

  annulus.CreateOperator();

  OperatorType::SizeType annulusSize;
  annulusSize = annulus.GetSize();
  std::cout << ", N = " << annulusSize << ", r = " << annulus.GetInnerRadius() << ", t = " << annulus.GetThickness()
            << ", i = " << annulus.GetInteriorValue() << ", a = " << annulus.GetAnnulusValue()
            << ", e = " << annulus.GetExteriorValue() << std::endl;

  for (unsigned int i = 0, k = 0; i < annulusSize[1]; ++i)
  {
    for (unsigned int j = 0; j < annulusSize[0]; ++j, ++k)
    {
      std::cout.width(3);
      std::cout.precision(3);
      std::cout << annulus[k] << '\t';
    }
    std::cout << std::endl;
  }

  // Try making the inner radius 0
  std::cout << std::endl << std::endl;
  std::cout << "Setting inner radius to zero";
  annulus.SetInnerRadius(0);
  annulus.CreateOperator();

  annulusSize = annulus.GetSize();
  std::cout << ", N = " << annulusSize << ", r = " << annulus.GetInnerRadius() << ", t = " << annulus.GetThickness()
            << ", i = " << annulus.GetInteriorValue() << ", a = " << annulus.GetAnnulusValue()
            << ", e = " << annulus.GetExteriorValue() << std::endl;

  for (unsigned int i = 0, k = 0; i < annulusSize[1]; ++i)
  {
    for (unsigned int j = 0; j < annulusSize[0]; ++j, ++k)
    {
      std::cout.width(3);
      std::cout.precision(3);
      std::cout << annulus[k] << '\t';
    }
    std::cout << std::endl;
  }


  // Try making the thickness 0
  std::cout << std::endl << std::endl;
  std::cout << "Setting thickness radius to zero";
  annulus.SetInnerRadius(2);
  annulus.SetThickness(0);
  annulus.CreateOperator();

  annulusSize = annulus.GetSize();
  std::cout << ", N = " << annulusSize << ", r = " << annulus.GetInnerRadius() << ", t = " << annulus.GetThickness()
            << ", i = " << annulus.GetInteriorValue() << ", a = " << annulus.GetAnnulusValue()
            << ", e = " << annulus.GetExteriorValue() << std::endl;

  for (unsigned int i = 0, k = 0; i < annulusSize[1]; ++i)
  {
    for (unsigned int j = 0; j < annulusSize[0]; ++j, ++k)
    {
      std::cout.width(3);
      std::cout.precision(3);
      std::cout << annulus[k] << '\t';
    }
    std::cout << std::endl;
  }


  // Specifying a spacing
  std::cout << std::endl << std::endl;
  std::cout << "Specifying a spacing";
  OperatorType::SpacingType spacing;
  spacing[0] = 0.5;
  spacing[1] = 0.25;

  annulus.SetSpacing(spacing);
  ITK_TEST_SET_GET_VALUE(spacing, annulus.GetSpacing());

  annulus.SetInnerRadius(2);
  annulus.SetThickness(1);

  annulus.CreateOperator();

  annulusSize = annulus.GetSize();
  std::cout << ", N = " << annulusSize << ", r = " << annulus.GetInnerRadius() << ", t = " << annulus.GetThickness()
            << ", s = " << annulus.GetSpacing() << ", i = " << annulus.GetInteriorValue()
            << ", a = " << annulus.GetAnnulusValue() << ", e = " << annulus.GetExteriorValue() << std::endl;

  for (unsigned int i = 0, k = 0; i < annulusSize[1]; ++i)
  {
    for (unsigned int j = 0; j < annulusSize[0]; ++j, ++k)
    {
      std::cout.width(3);
      std::cout.precision(3);
      std::cout << annulus[k] << '\t';
    }
    std::cout << std::endl;
  }


  std::cout << "Test finished." << std::endl;
  return EXIT_SUCCESS;
}
