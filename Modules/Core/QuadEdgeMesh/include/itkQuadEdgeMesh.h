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
#ifndef itkQuadEdgeMesh_h
#define itkQuadEdgeMesh_h

#include <cstdarg>
#include <queue>
#include <vector>
#include <list>

#include "itkMesh.h"

#include "itkQuadEdgeMeshTraits.h"
#include "itkQuadEdgeMeshLineCell.h"
#include "itkQuadEdgeMeshPolygonCell.h"

#include "itkQuadEdgeMeshFrontIterator.h"
#include "itkConceptChecking.h"


namespace itk
{
/**
 * \class QuadEdgeMesh
 *
 * \brief Mesh class for 2D manifolds embedded in ND space.
 *
 * \author Alexandre Gouaillard, Leonardo Florez-Valencia, Eric Boix
 *
 * This implementation was contributed as a paper to the Insight Journal
 * https://doi.org/10.54294/4mx7kk
 *
 * \ingroup ITKQuadEdgeMesh
 */
template <typename TPixel,
          unsigned int VDimension,
          typename TTraits = QuadEdgeMeshTraits<TPixel, VDimension, bool, bool>>
class ITK_TEMPLATE_EXPORT QuadEdgeMesh : public Mesh<TPixel, VDimension, TTraits>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(QuadEdgeMesh);

  /** Input template parameters. */
  using Traits = TTraits;
  using PixelType = TPixel;

  /** Standard type alias. */
  using Self = QuadEdgeMesh;
  using Superclass = Mesh<TPixel, VDimension, Traits>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Convenient constants obtained from MeshTraits. */
  static constexpr unsigned int PointDimension = Traits::PointDimension;
  static constexpr unsigned int MaxTopologicalDimension = Traits::MaxTopologicalDimension;

  /** Types defined in superclass. */
  using typename Superclass::CellPixelType;
  using typename Superclass::CoordinateType;
  using typename Superclass::PointIdentifier;
  using typename Superclass::PointHashType;
  using typename Superclass::PointType;
  using typename Superclass::CellTraits;

  using PointIdInternalIterator = typename CellTraits::PointIdInternalIterator;
  using PointIdIterator = typename CellTraits::PointIdIterator;

  // Point section:
  using typename Superclass::PointsContainer;
  using typename Superclass::PointsContainerPointer;
  using CoordRepArrayType = CoordinateType[Self::PointDimension];

  // Point data section:
  using typename Superclass::PointDataContainer;
  using typename Superclass::PointDataContainerPointer;
  using typename Superclass::PointDataContainerIterator;
  using typename Superclass::PointsContainerConstIterator;
  using typename Superclass::PointsContainerIterator;

  // Cell section:
  using typename Superclass::CellIdentifier;
  using typename Superclass::CellType;
  using typename Superclass::CellAutoPointer;
  using typename Superclass::CellFeatureIdentifier;
  using typename Superclass::CellFeatureCount;
  using typename Superclass::CellMultiVisitorType;
  using typename Superclass::CellsContainer;
  using typename Superclass::CellsContainerPointer;

  using typename Superclass::CellsContainerConstIterator;
  using typename Superclass::CellsContainerIterator;

  using typename Superclass::CellLinksContainer;
  using typename Superclass::CellLinksContainerPointer;
  using typename Superclass::CellLinksContainerIterator;

  // Cell data section:
  using typename Superclass::CellDataContainer;
  using typename Superclass::CellDataContainerPointer;
  using typename Superclass::CellDataContainerIterator;

  // Point / Cell correspondence section:
  using typename Superclass::PointCellLinksContainer;
  using typename Superclass::PointCellLinksContainerIterator;

  // BoundaryAssignments section:
  using typename Superclass::BoundaryAssignmentsContainer;
  using typename Superclass::BoundaryAssignmentsContainerPointer;
  using typename Superclass::BoundaryAssignmentsContainerVector;

  // Miscellaneous section:
  using typename Superclass::BoundingBoxPointer;
  using typename Superclass::BoundingBoxType;
  using typename Superclass::RegionType;
  using typename Superclass::InterpolationWeightType;

  /** Specific types for a quad-edge structure. */
  using PrimalDataType = typename Traits::PrimalDataType;
  using DualDataType = typename Traits::DualDataType;
  using QEPrimal = typename Traits::QEPrimal;
  using QEDual = typename Traits::QEDual;
  using QEType = typename Traits::QEPrimal;
  // See the TODO entry dated from 2005-05-28
  // struct QEType : public QEPrimal, public QEDual {}
  using VertexRefType = typename Traits::VertexRefType;
  using FaceRefType = typename Traits::FaceRefType;
  using VectorType = typename Traits::VectorType;

  /** Possible specialized cell types. */
  using EdgeCellType = QuadEdgeMeshLineCell<CellType>;
  using PolygonCellType = QuadEdgeMeshPolygonCell<CellType>;

  /** Free insertion indexes. */
  using FreePointIndexesType = std::queue<PointIdentifier>;
  using FreeCellIndexesType = std::queue<CellIdentifier>;

  /** Auxiliary types. */
  using PointIdList = std::vector<PointIdentifier>;
  using EdgeListType = std::list<QEPrimal *>;
#ifndef ITK_FUTURE_LEGACY_REMOVE
  using EdgeListPointerType [[deprecated("Please just use `EdgeListType *` instead!")]] = EdgeListType *;
#endif
  using MeshClassCellsAllocationMethodEnum = itk::MeshEnums::MeshClassCellsAllocationMethod;

  /** Reserved PointIdentifier designated to represent the absence of Point */
  static const PointIdentifier m_NoPoint;

  /** Reserved CellIdentifier designated to represent the absence of Face */
  static const CellIdentifier m_NoFace;

public:
  /** Basic Object interface. */
  /** @ITKStartGrouping */
  itkNewMacro(Self);
  itkOverrideGetNameOfClassMacro(QuadEdgeMesh);
  /** @ITKEndGrouping */

#if !defined(ITK_WRAPPING_PARSER)
  /** FrontIterator definitions */
  itkQEDefineFrontIteratorMethodsMacro(Self);
#endif

public:
  // Multithreading framework: not tested yet.
  bool
  RequestedRegionIsOutsideOfTheBufferedRegion() override
  {
    return (false);
  }

  void
  Initialize() override;

  /** another way of deleting all the cells */
  virtual void
  Clear();

  CellsContainer *
  GetEdgeCells()
  {
    return m_EdgeCellsContainer;
  }
  const CellsContainer *
  GetEdgeCells() const
  {
    return m_EdgeCellsContainer;
  }
  void
  SetEdgeCells(CellsContainer * edgeCells)
  {
    m_EdgeCellsContainer = edgeCells;
  }
  void
  SetEdgeCell(CellIdentifier cellId, CellAutoPointer & cellPointer)
  {
    m_EdgeCellsContainer->InsertElement(cellId, cellPointer.ReleaseOwnership());
  }

  /** Overloaded to avoid a bug in Mesh that prevents proper inheritance
   * Refer to
   * https://public.kitware.com/pipermail/insight-users/2005-March/012459.html
   * and
   * https://public.kitware.com/pipermail/insight-users/2005-April/012613.html
   */
  void
  CopyInformation(const DataObject * itkNotUsed(data)) override
  {}
  void
  Graft(const DataObject * data) override;

  /** squeeze the point container to be able to write the file properly */
  void
  SqueezePointsIds();

  /** overloaded method for backward compatibility */
  void
  BuildCellLinks() const
  {}

#if !defined(ITK_WRAPPING_PARSER)
  /** overloaded method for backward compatibility */
  void
  SetBoundaryAssignments(int itkNotUsed(dimension), BoundaryAssignmentsContainer * itkNotUsed(container))
  {}

  /** overloaded method for backward compatibility */
  BoundaryAssignmentsContainerPointer
  GetBoundaryAssignments(int itkNotUsed(dimension))
  {
    return (nullptr);
  }

  /** overloaded method for backward compatibility */
  const BoundaryAssignmentsContainerPointer
  GetBoundaryAssignments(int itkNotUsed(dimension)) const
  {
    return (nullptr);
  }

#endif

  /** overloaded method for backward compatibility */
  void
  SetBoundaryAssignment(int                   itkNotUsed(dimension),
                        CellIdentifier        itkNotUsed(cellId),
                        CellFeatureIdentifier itkNotUsed(featureId),
                        CellIdentifier        itkNotUsed(boundaryId))
  {}

  /** overloaded method for backward compatibility */
  bool
  GetBoundaryAssignment(int                   itkNotUsed(dimension),
                        CellIdentifier        itkNotUsed(cellId),
                        CellFeatureIdentifier itkNotUsed(featureId),
                        CellIdentifier *      itkNotUsed(boundaryId))
  {
    return (false); // ALEX: is it the good way?
  }

  /** overloaded method for backward compatibility */
  bool
  RemoveBoundaryAssignment(int                   itkNotUsed(dimension),
                           CellIdentifier        itkNotUsed(cellId),
                           CellFeatureIdentifier itkNotUsed(featureId))
  {
    return (false); // ALEX: is it the good way?
  }

  /** overloaded method for backward compatibility */
  bool
  GetCellBoundaryFeature(int                   itkNotUsed(dimension),
                         CellIdentifier        itkNotUsed(cellId),
                         CellFeatureIdentifier itkNotUsed(featureId),
                         CellAutoPointer &     itkNotUsed(cellAP)) const
  {
    return (false);
  }

  /** overloaded method for backward compatibility */
  CellIdentifier
  GetCellBoundaryFeatureNeighbors(int                        itkNotUsed(dimension),
                                  CellIdentifier             itkNotUsed(cellId),
                                  CellFeatureIdentifier      itkNotUsed(featureId),
                                  std::set<CellIdentifier> * itkNotUsed(cellSet))
  {
    return CellIdentifier{};
  }

  /** NOTE ALEX: this method do not use CellFeature and thus could be recoded */
  CellIdentifier
  GetCellNeighbors(CellIdentifier itkNotUsed(cellId), std::set<CellIdentifier> * itkNotUsed(cellSet))
  {
    return CellIdentifier{};
  }

  /** overloaded method for backward compatibility */
  bool
  GetAssignedCellBoundaryIfOneExists(int                   itkNotUsed(dimension),
                                     CellIdentifier        itkNotUsed(cellId),
                                     CellFeatureIdentifier itkNotUsed(featureId),
                                     CellAutoPointer &     itkNotUsed(cellAP)) const
  {
    return (false); // ALEX: is it the good way?
  }

  /** overloaded method for backward compatibility */
  void
  SetCell(CellIdentifier cId, CellAutoPointer & cell);

  /** Methods to simplify point/edge insertion/search. */
  virtual PointIdentifier
  FindFirstUnusedPointIndex();

  virtual CellIdentifier
  FindFirstUnusedCellIndex();

  virtual void
  PushOnContainer(EdgeCellType * newEdge);

  // Adding Point/Edge/Face methods
  virtual PointIdentifier
  AddPoint(const PointType & p);

  /** */
  virtual QEPrimal *
  AddEdge(const PointIdentifier & orgPid, const PointIdentifier & destPid);

  virtual QEPrimal *
  AddEdgeWithSecurePointList(const PointIdentifier & orgPid, const PointIdentifier & destPid);

  /** Add a polygonal face to the Mesh, suppose QE layer ready */
  virtual void
  AddFace(QEPrimal * entry);

  /** Add a polygonal face to the Mesh. The list of points
   * is expected to be ordered counter-clock wise. The inside
   * of the new face will be on the left side of the edges
   * formed by consecutive points in this list. */
  virtual QEPrimal *
  AddFace(const PointIdList & points);

  virtual QEPrimal *
  AddFaceWithSecurePointList(const PointIdList & points);

  virtual QEPrimal *
  AddFaceWithSecurePointList(const PointIdList & points, bool CheckEdges);

  /** Adds a triangular face to the Mesh */
  virtual QEPrimal *
  AddFaceTriangle(const PointIdentifier & aPid, const PointIdentifier & bPid, const PointIdentifier & cPid);

  /** Deletion methods */
  virtual void
  DeletePoint(const PointIdentifier & pid);

  virtual void
  DeleteEdge(const PointIdentifier & orgPid, const PointIdentifier & destPid);

  virtual void
  DeleteEdge(QEPrimal * e);

  virtual void
  LightWeightDeleteEdge(EdgeCellType * edgeCell);

  virtual void
  LightWeightDeleteEdge(QEPrimal * e);

  virtual void
  DeleteFace(FaceRefType faceToDelete);

  //
  bool
  GetPoint(PointIdentifier pid, PointType * pt) const
  {
    return (Superclass::GetPoint(pid, pt));
  }

  virtual PointType
  GetPoint(const PointIdentifier & pid) const;

  virtual VectorType
  GetVector(const PointIdentifier & pid) const;

  virtual QEPrimal *
  GetEdge() const;

  virtual QEPrimal *
  GetEdge(const CellIdentifier & eid) const;

  virtual QEPrimal *
  FindEdge(const PointIdentifier & pid0) const;

  virtual QEPrimal *
  FindEdge(const PointIdentifier & pid0, const PointIdentifier & pid1) const;

  virtual EdgeCellType *
  FindEdgeCell(const PointIdentifier & pid0, const PointIdentifier & pid1) const;

  ///  Compute the euclidean length of argument edge
  CoordinateType
  ComputeEdgeLength(QEPrimal * e);

  PointIdentifier
  ComputeNumberOfPoints() const;

  CellIdentifier
  ComputeNumberOfFaces() const;

  CellIdentifier
  ComputeNumberOfEdges() const;

  PointIdentifier
  Splice(QEPrimal * a, QEPrimal * b);

  // for reusability of a mesh in the MeshToMesh filter
  void
  ClearFreePointAndCellIndexesLists()
  {
    while (!this->m_FreePointIndexes.empty())
    {
      this->m_FreePointIndexes.pop();
    }
    while (!this->m_FreeCellIndexes.empty())
    {
      this->m_FreeCellIndexes.pop();
    }
  }

  CellIdentifier
  GetNumberOfFaces() const
  {
    return (m_NumberOfFaces);
  }
  CellIdentifier
  GetNumberOfEdges() const
  {
    return (m_NumberOfEdges);
  }

protected:
  /** Constructor and Destructor. */
  /** @ITKStartGrouping */
  QuadEdgeMesh();
  ~QuadEdgeMesh() override;
  /** @ITKEndGrouping */
  /** Release the memory of each one of the cells independently. */
  virtual void
  ClearCellsContainer();

  CellsContainerPointer m_EdgeCellsContainer{};

private:
  CellIdentifier m_NumberOfFaces{};
  CellIdentifier m_NumberOfEdges{};

protected:
  FreePointIndexesType m_FreePointIndexes{};
  FreeCellIndexesType  m_FreeCellIndexes{};
};
} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkQuadEdgeMesh.hxx"
#endif

#endif
