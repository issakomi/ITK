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
#include <iostream>

#include "itkBSplineSmoothingOnUpdateDisplacementFieldTransform.h"
#include "itkMath.h"
#include "itkTestingMacros.h"

/**
 * Test the UpdateTransformParameters and related methods,
 * introduced by this derivation.
 *
 * TODO: Create a more complete numerical test for the smoothing.
 */

int
itkBSplineSmoothingOnUpdateDisplacementFieldTransformTest(int, char *[])
{

  constexpr unsigned int dimensions = 2;
  using DisplacementTransformType = itk::BSplineSmoothingOnUpdateDisplacementFieldTransform<double, dimensions>;

  /* Create a displacement field transform */
  auto displacementTransform = DisplacementTransformType::New();

  ITK_EXERCISE_BASIC_OBJECT_METHODS(
    displacementTransform, BSplineSmoothingOnUpdateDisplacementFieldTransform, DisplacementFieldTransform);


  constexpr typename DisplacementTransformType::ArrayType::ValueType controlPointsUpdateFieldVal = 4;
  auto                                                               controlPointsUpdateField =
    itk::MakeFilled<typename DisplacementTransformType::ArrayType>(controlPointsUpdateFieldVal);
  ITK_TEST_SET_GET_VALUE(controlPointsUpdateField, displacementTransform->GetNumberOfControlPointsForTheUpdateField());

  constexpr typename DisplacementTransformType::ArrayType::ValueType controlPointsTotalFieldVal = 0;
  auto                                                               controlPointsTotalField =
    itk::MakeFilled<typename DisplacementTransformType::ArrayType>(controlPointsTotalFieldVal);
  ITK_TEST_SET_GET_VALUE(controlPointsTotalField, displacementTransform->GetNumberOfControlPointsForTheTotalField());

  using FieldType = DisplacementTransformType::DisplacementFieldType;
  auto field = FieldType::New(); // This is based on itk::Image

  FieldType::SizeType   size;
  FieldType::IndexType  start;
  FieldType::RegionType region;
  constexpr int         dimLength = 20;
  size.Fill(dimLength);
  start.Fill(0);
  region.SetSize(size);
  region.SetIndex(start);
  field->SetRegions(region);
  field->Allocate();

  constexpr DisplacementTransformType::OutputVectorType zeroVector{};
  field->FillBuffer(zeroVector);

  displacementTransform->SetDisplacementField(field);

  /* Test SmoothDisplacementFieldBSpline */
  std::cout << "Test SmoothDisplacementFieldBSpline" << std::endl;
  DisplacementTransformType::ParametersType                paramsFill(displacementTransform->GetNumberOfParameters());
  constexpr DisplacementTransformType::ParametersValueType paramsFillValue = 0.0;
  paramsFill.Fill(paramsFillValue);
  // Add an outlier to visually see that some smoothing is taking place.
  constexpr unsigned int outlier = dimLength * dimensions * 4 + dimLength * dimensions / 2;
  paramsFill(outlier) = 99.0;
  paramsFill(outlier + 1) = 99.0;

  auto meshSizeForUpdateField = itk::MakeFilled<DisplacementTransformType::ArrayType>(15);
  displacementTransform->SetMeshSizeForTheUpdateField(meshSizeForUpdateField);

  auto meshSizeForTotalField = itk::MakeFilled<DisplacementTransformType::ArrayType>(30);
  displacementTransform->SetMeshSizeForTheTotalField(meshSizeForTotalField);

  constexpr typename DisplacementTransformType::SplineOrderType splineOrder = 3;
  displacementTransform->SetSplineOrder(splineOrder);
  ITK_TEST_SET_GET_VALUE(splineOrder, displacementTransform->GetSplineOrder());

  auto enforceStationaryBoundary = true;
  ITK_TEST_SET_GET_BOOLEAN(displacementTransform, EnforceStationaryBoundary, enforceStationaryBoundary);

  displacementTransform->SetParameters(paramsFill);

  const DisplacementTransformType::NumberOfParametersType numberOfParameters =
    displacementTransform->GetNumberOfParameters();

  DisplacementTransformType::DerivativeType update1(numberOfParameters);
  update1.Fill(1.2);

  displacementTransform->UpdateTransformParameters(update1);
  DisplacementTransformType::ParametersType params = displacementTransform->GetParameters();

  /* We should see 0's on all boundaries from the smoothing routine */
  unsigned int linelength = dimLength * dimensions;
  for (unsigned int i = 0; i < displacementTransform->GetNumberOfParameters(); ++i)
  {
    bool ok = true;

    if (i < linelength && params[i] > 1e-6)
    {
      ok = false;
    }

    if (i % linelength == 0 && params[i] > 1e-6)
    {
      ok = false;
    }

    if (i % linelength == (linelength - 1) && params[i] > 1e-6)
    {
      ok = false;
    }

    if (!ok)
    {
      std::cout << "0-valued boundaries not found when expected "
                << "after smoothing." << std::endl;
      std::cout << "params: " << std::endl << params << std::endl;
      return EXIT_FAILURE;
    }
  }

  /* Check that we have some smoothing around the outlier we set above. */
  std::cout << "Parameters *after* SmoothDisplacementFieldBSpline, around "
            << "outlier: " << std::endl;

  for (int i = -2; i < 3; ++i)
  {
    for (int j = -2; j < 3; ++j)
    {
      const unsigned int index = outlier + static_cast<unsigned int>(i * static_cast<int>(dimLength * dimensions) + j);
      std::cout << params(index) << ' ';
      if (itk::Math::AlmostEquals(params(index), paramsFillValue))
      {
        std::cout << "Expected to read a smoothed value at this index."
                  << " Instead, read " << params(index) << std::endl;
        return EXIT_FAILURE;
      }
    }
    std::cout << std::endl;
  }

  /* Test UpdateTransformParameters */
  std::cout << "Testing UpdateTransformParameters..." << std::endl;

  /* fill with 0 */
  field->FillBuffer(zeroVector);

  DisplacementTransformType::DerivativeType update(displacementTransform->GetNumberOfParameters());

  update.Fill(1.2);
  displacementTransform->UpdateTransformParameters(update);
  params = displacementTransform->GetParameters();
  // std::cout  << "params: " << std::endl << params << std::endl;
  //<< "derivativeTruth: " << std::endl << derivative << std::endl
  /* We should see 0's on all boundaries from the smoothing routine */
  {
    linelength = dimLength * dimensions;
    for (unsigned int i = 0; i < displacementTransform->GetNumberOfParameters(); ++i)
    {
      bool ok = true;
      if (i < linelength && params[i] > 1e-6)
      {
        ok = false;
      }

      if (i % linelength == 0 && params[i] > 1e-6)
      {
        ok = false;
      }

      if (i % linelength == (linelength - 1) && params[i] > 1e-6)
      {
        ok = false;
      }

      if (!ok)
      {
        std::cout << "0-valued boundaries not found when expected "
                  << "after UpdateTransformParameters:" << std::endl;
        std::cout << "params: " << std::endl << params << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  /* Update with an uneven field to verify some smoothing is happening. */
  field->FillBuffer(zeroVector);
  update.Fill(1.0);
  update(outlier) = 99.0;
  update(outlier + 1) = 99.0;
  displacementTransform->UpdateTransformParameters(update);
  params = displacementTransform->GetParameters();
  std::cout << "UpdateTransformParameters with uneven update: " << std::endl
            << "params: " << std::endl
            << params << std::endl;
  /* Check that we have some smoothing around the outlier we set above. */
  std::cout << "Parameters *after* UpdateTransformParameters with "
            << "uneven field, around outlier: " << std::endl;
  for (int i = -2; i < 3; ++i)
  {
    for (int j = -2; j < 3; ++j)
    {
      const unsigned int index = outlier + static_cast<unsigned int>(i * static_cast<int>(dimLength * dimensions) + j);
      std::cout << params(index) << ' ';
      if (itk::Math::AlmostEquals(params(index), paramsFillValue))
      {
        std::cout << "Expected to read a smoothed value at this index."
                  << " Instead, read " << params(index) << std::endl;
        return EXIT_FAILURE;
      }
    }
    std::cout << std::endl;
  }

  return EXIT_SUCCESS;
}
