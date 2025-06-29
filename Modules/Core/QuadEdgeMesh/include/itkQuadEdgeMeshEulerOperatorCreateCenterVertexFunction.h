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
#ifndef itkQuadEdgeMeshEulerOperatorCreateCenterVertexFunction_h
#define itkQuadEdgeMeshEulerOperatorCreateCenterVertexFunction_h

#include "itkQuadEdgeMeshFunctionBase.h"

namespace itk
{
/**
 * \class QuadEdgeMeshEulerOperatorCreateCenterVertexFunction
 * \brief Create a vertex at the barycenter of the given face.
 *
 * \ingroup QEMeshModifierFunctions
 * \ingroup ITKQuadEdgeMesh
 */
template <typename TMesh, typename TQEType>
class ITK_TEMPLATE_EXPORT QuadEdgeMeshEulerOperatorCreateCenterVertexFunction
  : public QuadEdgeMeshFunctionBase<TMesh, TQEType *>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(QuadEdgeMeshEulerOperatorCreateCenterVertexFunction);

  /** Standard class type aliases. */
  using Self = QuadEdgeMeshEulerOperatorCreateCenterVertexFunction;
  using Superclass = QuadEdgeMeshFunctionBase<TMesh, TQEType *>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  itkNewMacro(Self);
  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(QuadEdgeMeshEulerOperatorCreateCenterVertexFunction);

  /** Type of QuadEdge with which to apply slicing. */
  using QEType = TQEType;

  using typename Superclass::MeshType;
  using typename Superclass::OutputType;

  using PointIdentifier = typename MeshType::PointIdentifier;
  using PointType = typename MeshType::PointType;
  using CoordinateType = typename MeshType::CoordinateType;
#ifndef ITK_FUTURE_LEGACY_REMOVE
  using CoordRepType ITK_FUTURE_DEPRECATED(
    "ITK 6 discourages using `CoordRepType`. Please use `CoordinateType` instead!") = CoordinateType;
#endif
  using VectorType = typename MeshType::VectorType;

  /** Evaluate at the specified input position */
  virtual OutputType
  Evaluate(QEType * e);

  PointIdentifier
  GetNewPointID()
  {
    return (this->m_NewPointID);
  }

protected:
  QuadEdgeMeshEulerOperatorCreateCenterVertexFunction()
    : m_NewPointID((PointIdentifier)0)
  {}

  ~QuadEdgeMeshEulerOperatorCreateCenterVertexFunction() override = default;

private:
  PointIdentifier m_NewPointID{};
};
} // namespace itk

#include "itkQuadEdgeMeshEulerOperatorCreateCenterVertexFunction.hxx"

#endif // itkQuadEdgeMeshEulerOperatorCreateCenterVertexFunction_h
