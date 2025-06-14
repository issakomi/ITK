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
// Disable warning for long symbol names in this file only

#include "itkTestingMacros.h"
#include "itkSurfaceSpatialObject.h"
#include "itkMath.h"

int
itkSurfaceSpatialObjectTest(int, char *[])
{
  using SurfaceType = itk::SurfaceSpatialObject<3>;
  using SurfacePointer = SurfaceType::Pointer;
  using SurfacePointType = itk::SurfaceSpatialObjectPoint<3>;
  using VectorType = itk::CovariantVector<double, 3>;

  std::cout << "==================================" << std::endl;
  std::cout << "Testing SurfaceSpatialObject:" << std::endl << std::endl;

  SurfaceType::SurfacePointListType list;

  for (unsigned int i = 0; i < 10; ++i)
  {
    SurfacePointType p;
    p.SetPositionInObjectSpace(i, i + 1, i + 2);
    VectorType normal;
    for (unsigned int j = 0; j < 3; ++j)
    {
      normal[j] = j;
    }
    p.SetNormalInObjectSpace(normal);
    list.push_back(p);
  }

  // For coverage
  SurfacePointType p;
  p.SetPositionInObjectSpace(1, 2, 3);
  p.Print(std::cout);

  // Create a Surface Spatial Object
  const SurfacePointer Surface = SurfaceType::New();
  Surface->GetProperty().SetName("Surface 1");
  Surface->SetId(1);
  Surface->SetPoints(list);

  Surface->Update();

  // Number of points
  std::cout << "Testing Consistency: " << std::endl;
  std::cout << "Number of Points: ";

  if (Surface->GetPoints().size() != 10)
  {
    std::cout << "[FAILED]" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "[PASSED]" << std::endl;


  // Point consistency
  std::cout << "Point consistency: ";

  auto it = Surface->GetPoints().begin();

  unsigned int i = 0;
  while (it != Surface->GetPoints().end())
  {
    for (unsigned int d = 0; d < 3; ++d)
    {
      if (itk::Math::NotExactlyEquals(it->GetPositionInWorldSpace()[d], i + d))
      {
        std::cout << "[FAILED]" << std::endl;
        return EXIT_FAILURE;
      }

      if (itk::Math::NotExactlyEquals(it->GetNormalInObjectSpace()[d], d))
      {
        std::cout << "[FAILED]" << std::endl;
        return EXIT_FAILURE;
      }

      if (itk::Math::NotExactlyEquals(it->GetNormalInWorldSpace()[d], d))
      {
        std::cout << "[FAILED]" << std::endl;
        return EXIT_FAILURE;
      }
    }
    it++;
    i++;
  }

  std::cout << "[PASSED]" << std::endl;

  // Point consistency
  std::cout << "Is Inside: ";
  itk::Point<double, 3> in;
  in[0] = 1;
  in[1] = 2;
  in[2] = 3;
  itk::Point<double, 3> out;
  out[0] = 0;
  out[1] = 0;
  out[2] = 0;

  if (!Surface->IsInsideInWorldSpace(in))
  {
    std::cout << "[FAILED]" << std::endl;
    return EXIT_FAILURE;
  }

  if (Surface->IsInsideInWorldSpace(out))
  {
    std::cout << "[FAILED]" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "[PASSED]" << std::endl;

  // Testing IsEvaluableAt()
  std::cout << "IsEvaluableAt: ";
  if (!Surface->IsEvaluableAtInWorldSpace(in) || Surface->IsEvaluableAtInWorldSpace(out))
  {
    std::cout << "[FAILED]" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "[PASSED]" << std::endl;


  // Testing IsEvaluableAt()
  std::cout << "ValueAt: ";

  double value = NAN;
  if (!Surface->ValueAtInWorldSpace(in, value))
  {
    std::cout << "[FAILED]" << std::endl;
    return EXIT_FAILURE;
  }

  if (itk::Math::NotExactlyEquals(value, 1))
  {
    std::cout << "[FAILED]" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "[PASSED]" << std::endl;

  // Test Copy and Assignment for SurfacePointType
  {
    SurfacePointType pOriginal;

    // itk::SpatialObjectPoint
    pOriginal.SetId(250);
    pOriginal.SetColor(0.5, 0.4, 0.3, 0.2);
    pOriginal.SetPositionInObjectSpace(42, 41, 43);


    // itk::SurfaceSpatialObjectPoint
    auto normal = itk::MakeFilled<VectorType>(276);
    pOriginal.SetNormalInObjectSpace(normal);

    Surface->AddPoint(pOriginal);

    // Must get a copy of the added point. Each point contains a pointer
    //   to the spatial object that it is a part of.  That assignment is
    //   needed to determine the WorldSpace of the point - which is defined
    //   by the tree of spatial objects it is a part of.
    pOriginal = Surface->GetPoints().back();

    for (size_t j = 0; j < 3; ++j)
    {
      ITK_TEST_EXPECT_TRUE(itk::Math::AlmostEquals(pOriginal.GetNormalInWorldSpace()[j], normal[j]));
    }
    pOriginal.SetNormalInWorldSpace(normal);
    for (size_t j = 0; j < 3; ++j)
    {
      ITK_TEST_EXPECT_TRUE(itk::Math::AlmostEquals(pOriginal.GetNormalInObjectSpace()[j], normal[j]));
    }

    // Copy
    const SurfacePointType pCopy(pOriginal);
    // Assign
    const SurfacePointType pAssign = pOriginal;

    std::vector<SurfacePointType> pointVector;
    pointVector.push_back(pCopy);
    pointVector.push_back(pAssign);

    for (const auto & pv : pointVector)
    {
      // itk::SpatialObjectPoint
      ITK_TEST_EXPECT_EQUAL(pOriginal.GetId(), pv.GetId());
      ITK_TEST_EXPECT_TRUE(itk::Math::AlmostEquals(pOriginal.GetRed(), pv.GetRed()));
      ITK_TEST_EXPECT_TRUE(itk::Math::AlmostEquals(pOriginal.GetGreen(), pv.GetGreen()));
      ITK_TEST_EXPECT_TRUE(itk::Math::AlmostEquals(pOriginal.GetBlue(), pv.GetBlue()));
      ITK_TEST_EXPECT_TRUE(itk::Math::AlmostEquals(pOriginal.GetAlpha(), pv.GetAlpha()));
      for (size_t j = 0; j < 3; ++j)
      {
        ITK_TEST_EXPECT_TRUE(
          itk::Math::AlmostEquals(pOriginal.GetPositionInObjectSpace()[j], pv.GetPositionInObjectSpace()[j]));
      }
      // itk::SurfaceSpatialObjectPoint
      for (size_t j = 0; j < 3; ++j)
      {
        ITK_TEST_EXPECT_TRUE(
          itk::Math::AlmostEquals(pOriginal.GetNormalInObjectSpace()[j], pv.GetNormalInObjectSpace()[j]));
        ITK_TEST_EXPECT_TRUE(
          itk::Math::AlmostEquals(pOriginal.GetNormalInWorldSpace()[j], pv.GetNormalInObjectSpace()[j]));
      }
    }
  }

  std::cout << "Test finished" << std::endl;
  return EXIT_SUCCESS;
}
