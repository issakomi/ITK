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

#include "itkCenteredSimilarity2DTransform.h"
#include "itkMath.h"

namespace
{
bool
CheckEqual(const itk::Point<double, 2> & p1, const itk::Point<double, 2> & p2)
{
  constexpr double epsilon = 1e-10;

  for (unsigned int i = 0; i < 2; ++i)
  {
    if (itk::Math::abs(p1[i] - p2[i]) > epsilon)
    {
      std::cout << p1 << " != " << p2 << ":[ FAILED ]" << std::endl;
      return false;
    }
  }
  std::cout << p1 << " == " << p2 << ":[ PASSED ]" << std::endl;
  return true;
}

} // namespace

int
itkSimilarity2DTransformTest(int, char *[])
{

  std::cout << "==================================" << std::endl;
  std::cout << "Testing Similarity 2D Transform" << std::endl << std::endl;

  constexpr double       epsilon = 1e-10;
  constexpr unsigned int N = 2;
  bool                   Ok = true;

  using SimilarityTransformType = itk::CenteredSimilarity2DTransform<double>;
  auto transform = SimilarityTransformType::New();

  // Test the identity transform
  std::cout << "Testing Identity:";
  transform->SetIdentity();

  SimilarityTransformType::InputPointType::ValueType pInit[2] = { 10, 10 };
  SimilarityTransformType::InputPointType            p = pInit;
  SimilarityTransformType::OutputPointType           r;

  r = transform->TransformPoint(p);
  for (unsigned int i = 0; i < N; ++i)
  {
    if (itk::Math::abs(p[i] - r[i]) > epsilon)
    {
      Ok = false;
      break;
    }
  }
  if (!Ok)
  {
    std::cerr << "Error with Identity transform" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " [ PASSED ] " << std::endl;


  // Test SetAngle/GetAngle
  auto transform1 = SimilarityTransformType::New();
  auto transform2 = SimilarityTransformType::New();
  transform1->SetIdentity();
  constexpr double angle1 = .125;
  transform1->SetAngle(angle1);
  transform2->SetMatrix(transform1->GetMatrix());
  std::cout << "Testing SetAngle(" << angle1 << ")/GetAngle():";
  constexpr double epsilon2 = 1e-5;
  if (itk::Math::abs(transform2->GetAngle() - angle1) > epsilon2)
  {
    std::cerr << "Error with SetAngle/GetAngle:" << std::endl;
    std::cerr << "transform1->SetAngle: " << angle1 << std::endl;
    std::cerr << "transform2->GetAngle: " << transform2->GetAngle() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " [ PASSED ] " << std::endl;


  transform1->SetIdentity();
  transform1->SetAngle(-angle1);
  transform2->SetMatrix(transform1->GetMatrix());
  std::cout << "Testing SetAngle(" << -angle1 << ")/GetAngle():";
  if (itk::Math::abs(transform2->GetAngle() - (-angle1)) > epsilon2)
  {
    std::cerr << "Error with SetAngle/GetAngle:" << std::endl;
    std::cerr << "transform1->SetAngle: " << -angle1 << std::endl;
    std::cerr << "transform2->GetAngle: " << transform2->GetAngle() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " [ PASSED ] " << std::endl;

  // Test the Set/Get Parameters
  std::cout << "Testing Set/GetParameters():" << std::endl;
  SimilarityTransformType::ParametersType params(6);
  for (unsigned int i = 0; i < 6; ++i)
  {
    params[i] = i + 1;
  }

  std::cout << "Input Parameters = " << params << std::endl;

  transform->SetParameters(params);
  SimilarityTransformType::ParametersType outputParams(6);

  outputParams = transform->GetParameters();

  std::cout << "Output Parameters = " << outputParams << std::endl;
  for (unsigned int i = 0; i < 4; ++i) // do not test for the offset
  {
    if (itk::Math::abs(outputParams[i] - params[i]) > epsilon)
    {
      Ok = false;
      break;
    }
  }
  if (!Ok)
  {
    std::cerr << "Error with Set/GetParameters:" << std::endl;
    std::cerr << "Input:" << params << std::endl;
    std::cerr << "Output:" << outputParams << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " [ PASSED ] " << std::endl;


  // 15 degrees in radians
  transform->SetCenter({}); // Explicitly reset center to default values
  transform->SetIdentity();
  const double angle = 15.0 * std::atan(1.0f) / 45.0;
  const double sinth = std::sin(angle);
  const double costh = std::cos(angle);

  std::cout << "Testing Rotation:";
  transform->SetAngle(angle);

  // Rotate an itk::Point
  SimilarityTransformType::InputPointType q;
  p = pInit;
  q[0] = p[0] * costh - p[1] * sinth;
  q[1] = p[0] * sinth + p[1] * costh;
  r = transform->TransformPoint(p);
  for (unsigned int i = 0; i < N; ++i)
  {
    if (itk::Math::abs(q[i] - r[i]) > epsilon)
    {
      Ok = false;
      break;
    }
  }
  if (!Ok)
  {
    std::cerr << "Error rotating point   : " << p << std::endl;
    std::cerr << "Result should be       : " << q << std::endl;
    std::cerr << "Reported Result is     : " << r << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " [ PASSED ] " << std::endl;


  std::cout << "Testing Translation:";

  transform->SetAngle(0);

  SimilarityTransformType::OffsetType::ValueType ioffsetInit[2] = { 1, 4 };
  const SimilarityTransformType::OffsetType      ioffset = ioffsetInit;

  transform->SetOffset(ioffset);

  q = p + ioffset;

  r = transform->TransformPoint(p);
  for (unsigned int i = 0; i < N; ++i)
  {
    if (itk::Math::abs(q[i] - r[i]) > epsilon)
    {
      Ok = false;
      break;
    }
  }
  if (!Ok)
  {
    std::cerr << "Error translating point: " << p << std::endl;
    std::cerr << "Result should be       : " << q << std::endl;
    std::cerr << "Reported Result is     : " << r << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " [ PASSED ] " << std::endl;


  // Testing the Jacobian
  std::cout << "Testing Jacobian:";
  SimilarityTransformType::JacobianType jacobian0;
  transform->ComputeJacobianWithRespectToParameters(p, jacobian0);

  if (itk::Math::NotExactlyEquals(jacobian0[0][0], 10) || itk::Math::NotExactlyEquals(jacobian0[0][1], -10) ||
      itk::Math::NotExactlyEquals(jacobian0[0][2], 0) || itk::Math::NotExactlyEquals(jacobian0[0][3], 0) ||
      itk::Math::NotExactlyEquals(jacobian0[0][4], 1) || itk::Math::NotExactlyEquals(jacobian0[0][5], 0) ||
      itk::Math::NotExactlyEquals(jacobian0[1][0], 10) || itk::Math::NotExactlyEquals(jacobian0[1][1], 10) ||
      itk::Math::NotExactlyEquals(jacobian0[1][2], 0) || itk::Math::NotExactlyEquals(jacobian0[1][3], 0) ||
      itk::Math::NotExactlyEquals(jacobian0[1][4], 0) || itk::Math::NotExactlyEquals(jacobian0[1][5], 1))
  {
    std::cerr << "Error with Jacobian: " << jacobian0 << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << " [ PASSED ] " << std::endl;


  {
    // Test instantiation, inverse computation, back transform etc.
    using TransformType = itk::Similarity2DTransform<double>;
    auto t1 = TransformType::New();

    // Set parameters
    TransformType::ParametersType parameters(t1->GetNumberOfParameters());

    parameters[0] = 2.0;
    parameters[1] = -21.0 / 180.0 * itk::Math::pi;
    parameters[2] = 12.0;
    parameters[3] = -8.9;

    TransformType::InputPointType center;
    center[0] = 67.8;
    center[1] = -0.2;

    t1->SetParameters(parameters);
    t1->SetCenter(center);

    TransformType::InputPointType p1;
    p1[0] = 96.8;
    p1[1] = -3.2;

    TransformType::InputPointType p2;
    p2 = t1->TransformPoint(p1);

    // Test inverse
    TransformType::Pointer t2;
    t1->CloneInverseTo(t2);

    TransformType::InputPointType p3;
    p3 = t2->TransformPoint(p2);

    std::cout << "Test CloneInverseTo(): ";
    if (!CheckEqual(p1, p3))
    {
      return EXIT_FAILURE;
    }

    auto t2dash = TransformType::New();
    t1->GetInverse(t2dash);
    TransformType::InputPointType p3dash;
    p3dash = t2dash->TransformPoint(p2);

    std::cout << "Test GetInverse(): ";
    if (!CheckEqual(p1, p3dash))
    {
      return EXIT_FAILURE;
    }

    t2dash = dynamic_cast<TransformType *>(t1->GetInverseTransform().GetPointer());
    if (!t2dash)
    {
      std::cout << "Cannot compute inverse transformation" << std::endl;
      return EXIT_FAILURE;
    }
    p3dash = t2dash->TransformPoint(p2);

    std::cout << "Test GetInverseTransform(): ";
    if (!CheckEqual(p1, p3dash))
    {
      return EXIT_FAILURE;
    }

    // Test clone
    TransformType::Pointer t3;
    t1->CloneTo(t3);

    TransformType::InputPointType p4;
    p4 = t3->TransformPoint(p1);

    std::cout << "Test Clone(): ";
    if (!CheckEqual(p2, p4))
    {
      return EXIT_FAILURE;
    }

    // Test singular matrix transform
    parameters[0] = 0.0; // scale
    parameters[1] = -2;
    parameters[2] = 12.0;
    parameters[3] = -8.9;
    t1->SetParameters(parameters);
    const bool computedInverse = t1->GetInverse(t2);
    if (computedInverse)
    {
      std::cout << "Did not report singular matrix when computed inverse of singular matrix" << std::endl;
      return EXIT_FAILURE;
    }

    // Test compose
    auto t4 = TransformType::New();

    parameters[0] = 0.6;
    parameters[1] = 14.7 / 180.0 * itk::Math::pi;
    parameters[2] = 4.0;
    parameters[3] = 4.0;

    center[0] = 67.1;
    center[1] = 67.1;

    t4->SetParameters(parameters);
    t4->SetCenter(center);

    TransformType::Pointer t5;
    // t1 scale was 0.0, generating div by zero.
    t1->SetScale(0.2);
    t1->CloneTo(t5);
    t5->Compose(t4, false);

    TransformType::InputPointType p5 = t1->TransformPoint(p1);
    TransformType::InputPointType p6 = t4->TransformPoint(p5);
    TransformType::InputPointType p7 = t5->TransformPoint(p1);

    std::cout << "Test Compose(.,false): ";
    if (!CheckEqual(p6, p7))
    {
      return EXIT_FAILURE;
    }

    t1->CloneTo(t5);
    t5->Compose(t4, true);

    p5 = t4->TransformPoint(p1);
    p6 = t1->TransformPoint(p5);
    p7 = t5->TransformPoint(p1);

    std::cout << "Test Compose(.,true): ";
    if (!CheckEqual(p6, p7))
    {
      return EXIT_FAILURE;
    }

    // Really test the jacobian
    std::cout << "Testing Jacobian: ";
    TransformType::JacobianType jacobian;
    t4->ComputeJacobianWithRespectToParameters(p1, jacobian);

    TransformType::JacobianType approxJacobian = jacobian;
    for (unsigned int k = 0; k < t1->GetNumberOfParameters(); ++k)
    {
      constexpr double              delta = 0.001;
      TransformType::ParametersType plusParameters;
      TransformType::ParametersType minusParameters;

      plusParameters = parameters;
      minusParameters = parameters;
      plusParameters[k] += delta;
      minusParameters[k] -= delta;

      TransformType::OutputPointType plusPoint;
      TransformType::OutputPointType minusPoint;

      t4->SetParameters(plusParameters);
      plusPoint = t4->TransformPoint(p1);
      t4->SetParameters(minusParameters);
      minusPoint = t4->TransformPoint(p1);
      for (unsigned int j = 0; j < 2; ++j)
      {
        const double approxDerivative = (plusPoint[j] - minusPoint[j]) / (2.0 * delta);
        const double computedDerivative = jacobian[j][k];
        approxJacobian[j][k] = approxDerivative;
        if (itk::Math::abs(approxDerivative - computedDerivative) > 1e-4)
        {
          std::cerr << "Error computing Jacobian [" << j << "][" << k << ']' << std::endl;
          std::cerr << "Result should be: " << approxDerivative << std::endl;
          std::cerr << "Reported result is: " << computedDerivative << std::endl;
          std::cerr << " [ FAILED ] " << std::endl;
          return EXIT_FAILURE;
        } // if
      } // for j

    } // for k

    std::cout << " [ PASSED ] " << std::endl;
  }

  {
    // Test instantiation, inverse computation, back transform etc.
    using TransformType = SimilarityTransformType;
    auto t1 = TransformType::New();

    // Set parameters
    TransformType::ParametersType parameters(t1->GetNumberOfParameters());

    parameters[0] = 2.0;
    parameters[1] = -21.0 / 180.0 * itk::Math::pi;
    parameters[2] = 12.0;
    parameters[3] = -8.9;
    parameters[4] = 67.8;
    parameters[5] = -0.2;

    t1->SetParameters(parameters);

    TransformType::InputPointType p1;
    p1[0] = 96.8;
    p1[1] = -3.2;

    TransformType::InputPointType p2;
    p2 = t1->TransformPoint(p1);

    // Test inverse
    TransformType::Pointer t2;
    t1->CloneInverseTo(t2);

    TransformType::InputPointType p3;
    p3 = t2->TransformPoint(p2);

    std::cout << "Test CloneInverseTo(): ";
    if (!CheckEqual(p1, p3))
    {
      return EXIT_FAILURE;
    }

    auto t2dash = TransformType::New();
    t1->GetInverse(t2dash);
    TransformType::InputPointType p3dash;
    p3dash = t2dash->TransformPoint(p2);

    std::cout << "Test GetInverse(): ";
    if (!CheckEqual(p1, p3dash))
    {
      return EXIT_FAILURE;
    }

    t2dash = dynamic_cast<TransformType *>(t1->GetInverseTransform().GetPointer());
    if (!t2dash)
    {
      std::cout << "Cannot compute inverse transformation" << std::endl;
      return EXIT_FAILURE;
    }
    p3dash = t2dash->TransformPoint(p2);

    std::cout << "Test GetInverseTransform(): ";
    if (!CheckEqual(p1, p3dash))
    {
      return EXIT_FAILURE;
    }

    // Test clone
    TransformType::Pointer t3;
    t1->CloneTo(t3);

    TransformType::InputPointType p4;
    p4 = t3->TransformPoint(p1);

    std::cout << "Test Clone(): ";
    if (!CheckEqual(p2, p4))
    {
      return EXIT_FAILURE;
    }

    // Test compose
    auto t4 = TransformType::New();

    parameters[0] = 0.6;
    parameters[1] = 14.7 / 180.0 * itk::Math::pi;
    parameters[2] = 4.0;
    parameters[3] = 4.0;
    parameters[4] = 67.1;
    parameters[5] = 67.1;

    t4->SetParameters(parameters);

    TransformType::Pointer t5;
    t1->CloneTo(t5);
    t5->Compose(t4, false);

    TransformType::InputPointType p5 = t1->TransformPoint(p1);
    TransformType::InputPointType p6 = t4->TransformPoint(p5);
    TransformType::InputPointType p7 = t5->TransformPoint(p1);

    std::cout << "Test Compose(.,false): ";
    if (!CheckEqual(p6, p7))
    {
      return EXIT_FAILURE;
    }

    t1->CloneTo(t5);
    t5->Compose(t4, true);

    p5 = t4->TransformPoint(p1);
    p6 = t1->TransformPoint(p5);
    p7 = t5->TransformPoint(p1);

    std::cout << "Test Compose(.,true): ";
    if (!CheckEqual(p6, p7))
    {
      return EXIT_FAILURE;
    }

    // Really test the jacobian
    std::cout << "Testing Jacobian: ";
    TransformType::JacobianType jacobian;
    t4->ComputeJacobianWithRespectToParameters(p1, jacobian);

    TransformType::JacobianType approxJacobian = jacobian;
    for (unsigned int k = 0; k < t1->GetNumberOfParameters(); ++k)
    {
      constexpr double              delta = 0.001;
      TransformType::ParametersType plusParameters;
      TransformType::ParametersType minusParameters;

      plusParameters = parameters;
      minusParameters = parameters;
      plusParameters[k] += delta;
      minusParameters[k] -= delta;

      TransformType::OutputPointType plusPoint;
      TransformType::OutputPointType minusPoint;

      t4->SetParameters(plusParameters);
      plusPoint = t4->TransformPoint(p1);
      t4->SetParameters(minusParameters);
      minusPoint = t4->TransformPoint(p1);
      for (unsigned int j = 0; j < 2; ++j)
      {
        const double approxDerivative = (plusPoint[j] - minusPoint[j]) / (2.0 * delta);
        const double computedDerivative = jacobian[j][k];
        approxJacobian[j][k] = approxDerivative;
        if (itk::Math::abs(approxDerivative - computedDerivative) > 1e-4)
        {
          std::cerr << "Error computing Jacobian [" << j << "][" << k << ']' << std::endl;
          std::cerr << "Result should be: " << approxDerivative << std::endl;
          std::cerr << "Reported result is: " << computedDerivative << std::endl;
          std::cerr << " [ FAILED ] " << std::endl;
          return EXIT_FAILURE;
        } // if
      } // for j

    } // for k

    std::cout << " [ PASSED ] " << std::endl;
  }

  {
    // Test Set/Get Matrix and Set/Get Offset
    using TransformType = itk::Similarity2DTransform<double>;
    auto t1 = TransformType::New();
    auto t2 = TransformType::New();

    TransformType::InputPointType center;
    center[0] = 9.0;
    center[1] = 10.0;

    TransformType::ParametersType parameters(t1->GetNumberOfParameters());
    for (unsigned int j = 0; j < t1->GetNumberOfParameters(); ++j)
    {
      parameters[j] = static_cast<double>(j) + 1.0;
    }
    parameters[1] *= itk::Math::pi / 180.0;

    t1->SetCenter(center);
    t1->SetParameters(parameters);

    t2->SetCenter(center);
    t2->SetMatrix(t1->GetMatrix());
    t2->SetOffset(t1->GetOffset());

    // check the transformed points are the same
    TransformType::InputPointType ip;
    ip[0] = 8.0;
    ip[1] = 9.0;

    const TransformType::OutputPointType op1 = t1->TransformPoint(ip);
    const TransformType::OutputPointType op2 = t2->TransformPoint(ip);

    t1->Print(std::cout);
    std::cout << "Test Set/GetMatrix() and Set/GetOffset(): ";
    if (!CheckEqual(op1, op2))
    {
      return EXIT_FAILURE;
    }

    // check that parameters are the same
    TransformType::ParametersType pdash = t2->GetParameters();

    std::cout << "Test Set/GetMatrix() and Set/GetOffset(): ";
    for (unsigned int j = 0; j < t1->GetNumberOfParameters(); ++j)
    {
      if (itk::Math::abs(parameters[j] - pdash[j]) > epsilon)
      {
        std::cerr.precision(static_cast<int>(itk::Math::abs(std::log10(epsilon))));
        std::cerr << "Test failed!" << std::endl;
        std::cerr << "Error in parameters at index [" << j << "]" << std::endl;
        std::cerr << "Expected value " << parameters << std::endl;
        std::cerr << " differs from " << pdash;
        std::cerr << " by more than " << epsilon << std::endl;
        return EXIT_FAILURE;
      }
    }
    std::cout << " [ PASSED ] " << std::endl;
  }

  return EXIT_SUCCESS;
}
