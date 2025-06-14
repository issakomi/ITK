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
#ifndef itkTubeSpatialObject_hxx
#define itkTubeSpatialObject_hxx


#include "itkMath.h"

namespace itk
{

template <unsigned int TDimension, typename TTubePointType>
TubeSpatialObject<TDimension, TTubePointType>::TubeSpatialObject()
{
  this->SetTypeName("TubeSpatialObject");

  this->Clear();

  this->Update();
}

template <unsigned int TDimension, typename TTubePointType>
void
TubeSpatialObject<TDimension, TTubePointType>::Clear()
{
  Superclass::Clear();

  this->GetProperty().SetRed(1);
  this->GetProperty().SetGreen(0);
  this->GetProperty().SetBlue(0);
  this->GetProperty().SetAlpha(1);

  m_Root = false;
  m_ParentPoint = -1;
  m_EndRounded = true; // default end-type is flat

  this->Modified();
}

template <unsigned int TDimension, typename TTubePointType>
void
TubeSpatialObject<TDimension, TTubePointType>::CopyInformation(const DataObject * data)
{
  // Standard call to the superclass' method
  Superclass::CopyInformation(data);

  // Attempt to cast data to an ImageBase
  const auto * soData = dynamic_cast<const TubeSpatialObject<TDimension> *>(data);

  if (soData == nullptr)
  {
    // pointer could not be cast back down
    itkExceptionMacro("itk::TubeSpatialObject::CopyInformation() cannot cast "
                      << typeid(data).name() << " to " << typeid(TubeSpatialObject<TDimension> *).name());
  }

  // check if we are the same type
  const auto * source = dynamic_cast<const Self *>(data);
  if (!source)
  {
    std::cerr << "CopyInformation: objects are not of the same type" << std::endl;
    return;
  }

  // copy the ivars
  this->SetRoot(source->GetRoot());
  this->SetEndRounded(source->GetEndRounded());

  // Don't copy parent info
  // this->SetParentPoint(source->GetParentPoint());
}

template <unsigned int TDimension, typename TTubePointType>
typename LightObject::Pointer
TubeSpatialObject<TDimension, TTubePointType>::InternalClone() const
{
  typename LightObject::Pointer loPtr = Superclass::InternalClone();

  const typename Self::Pointer rval = dynamic_cast<Self *>(loPtr.GetPointer());
  if (rval.IsNull())
  {
    itkExceptionMacro("downcast to type " << this->GetNameOfClass() << " failed.");
  }

  rval->SetEndRounded(this->GetEndRounded());
  rval->SetParentPoint(this->GetParentPoint());
  rval->SetRoot(this->GetRoot());

  return loPtr;
}

template <unsigned int TDimension, typename TTubePointType>
void
TubeSpatialObject<TDimension, TTubePointType>::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "ParentPoint : " << m_ParentPoint << std::endl;
  itkPrintSelfBooleanMacro(EndRounded);
  itkPrintSelfBooleanMacro(Root);
}

template <unsigned int TDimension, typename TTubePointType>
void
TubeSpatialObject<TDimension, TTubePointType>::ComputeMyBoundingBox()
{
  itkDebugMacro("Computing tube bounding box");

  auto it = this->m_Points.begin();
  auto end = this->m_Points.end();

  if (it == end)
  {
    const typename BoundingBoxType::PointType pnt{};
    this->GetModifiableMyBoundingBoxInObjectSpace()->SetMinimum(pnt);
    this->GetModifiableMyBoundingBoxInObjectSpace()->SetMaximum(pnt);
    return;
  }

  PointType pt = it->GetPositionInObjectSpace();
  double    ptRadius = it->GetRadiusInObjectSpace();

  // Compute a bounding box in object space
  PointType tmpPt;
  for (unsigned int d = 0; d < TDimension; ++d)
  {
    tmpPt[d] = pt[d] - ptRadius;
  }
  this->GetModifiableMyBoundingBoxInObjectSpace()->SetMinimum(tmpPt);
  this->GetModifiableMyBoundingBoxInObjectSpace()->SetMaximum(tmpPt);

  for (unsigned int d = 0; d < TDimension; ++d)
  {
    tmpPt[d] = pt[d] + ptRadius;
  }
  this->GetModifiableMyBoundingBoxInObjectSpace()->ConsiderPoint(tmpPt);

  ++it;
  while (it != end)
  {
    pt = it->GetPositionInObjectSpace();
    ptRadius = it->GetRadiusInObjectSpace();
    for (unsigned int d = 0; d < TDimension; ++d)
    {
      tmpPt[d] = pt[d] - ptRadius;
    }
    this->GetModifiableMyBoundingBoxInObjectSpace()->ConsiderPoint(tmpPt);

    for (unsigned int d = 0; d < TDimension; ++d)
    {
      tmpPt[d] = pt[d] + ptRadius;
    }
    this->GetModifiableMyBoundingBoxInObjectSpace()->ConsiderPoint(tmpPt);

    ++it;
  }
  this->GetModifiableMyBoundingBoxInObjectSpace()->ComputeBoundingBox();
}

template <unsigned int TDimension, typename TTubePointType>
bool
TubeSpatialObject<TDimension, TTubePointType>::IsInsideInObjectSpace(const PointType & point) const
{
  if (this->GetMyBoundingBoxInObjectSpace()->IsInside(point))
  {
    auto it = this->m_Points.begin();
    auto first = it;
    auto it2 = it;
    ++it2;
    auto end = this->m_Points.end();
    auto last = end;
    --last;

    const PointType firstP = first->GetPositionInObjectSpace();
    const double    firstR = first->GetRadiusInObjectSpace();
    const PointType lastP = last->GetPositionInObjectSpace();
    const double    lastR = last->GetRadiusInObjectSpace();

    bool withinEndCap = false;
    while (it2 != end)
    {
      // Check if the point is on the normal plane
      PointType a = it->GetPositionInObjectSpace();
      PointType b = it2->GetPositionInObjectSpace();

      if (!m_EndRounded)
      {
        withinEndCap = false;
        double firstDist = a.EuclideanDistanceTo(firstP);
        double lastDist = a.EuclideanDistanceTo(lastP);
        if (firstDist <= firstR || lastDist <= firstR)
        {
          withinEndCap = true;
        }
        else
        {
          firstDist = b.EuclideanDistanceTo(firstP);
          lastDist = b.EuclideanDistanceTo(lastP);
          if (firstDist <= lastR || lastDist <= lastR)
          {
            withinEndCap = true;
          }
        }
      }

      double A = 0;
      double B = 0;

      for (unsigned int i = 0; i < TDimension; ++i)
      {
        A += (b[i] - a[i]) * (point[i] - a[i]);
        B += (b[i] - a[i]) * (b[i] - a[i]);
      }

      if (B != 0)
      {
        double lambda = A / B;
        B = std::sqrt(B);

        double       lambdaMin = 0;
        double       lambdaMax = 1;
        const double lambdaMinR = it->GetRadiusInObjectSpace();
        const double lambdaMaxR = it2->GetRadiusInObjectSpace();
        if (m_EndRounded || !withinEndCap)
        {
          lambdaMin = -(lambdaMinR / B);
          lambdaMax = 1 + (lambdaMaxR / B);
          if (lambdaMax < (lambdaMinR / B))
          {
            lambdaMax = (lambdaMinR / B);
          }
          if (lambdaMin > (1 - (lambdaMaxR / B)))
          {
            lambdaMin = 1 - (lambdaMaxR / B);
          }
        }

        if (lambda >= lambdaMin && lambda <= lambdaMax)
        {
          if (lambda < 0)
          {
            lambda = 0;
          }
          else if (lambda > 1)
          {
            lambda = 1;
          }

          const double lambdaR = lambdaMinR + lambda * (lambdaMaxR - lambdaMinR);

          PointType p;
          for (unsigned int i = 0; i < TDimension; ++i)
          {
            p[i] = a[i] + lambda * (b[i] - a[i]);
          }

          double tempDist = point.EuclideanDistanceTo(p);
          if (tempDist <= lambdaR)
          {
            return true;
          }
        }
      }
      ++it;
      ++it2;
    }
  }
  return false;
}

template <unsigned int TDimension, typename TTubePointType>
unsigned int
TubeSpatialObject<TDimension, TTubePointType>::RemoveDuplicatePointsInObjectSpace(double minSpacingInObjectSpace)
{
  int nPoints = 0;

  auto it = this->m_Points.begin();
  while (it != this->m_Points.end())
  {
    const PointType pnt = it->GetPositionInObjectSpace();
    ++it;
    if (it != this->m_Points.end())
    {
      const PointType pnt2 = it->GetPositionInObjectSpace();
      const double    dist = pnt.EuclideanDistanceTo(pnt2);
      if (dist <= minSpacingInObjectSpace)
      {
        it = this->m_Points.erase(it);
        ++nPoints;
        --it;
      }
    }
  }
  return nPoints;
}

template <unsigned int TDimension, typename TTubePointType>
bool
TubeSpatialObject<TDimension, TTubePointType>::ComputeTangentsAndNormals()
{
  itkDebugMacro("Computing the tangent vectors of the tube");

  // Discrete Frenet Frame computation from
  //   http://purl.flvc.org/fsu/fd/FSU_migr_etd-7477
  //   Discrete Frenet Frame with Application to Structural Biology and
  //      Kinematics.
  //   Lu, Yuanting et al.
  //   FSU Dissertation FSU_migr_etd-7477, June 27, 2013

  const int length = this->GetNumberOfPoints();
  if (length == 0)
  {
    return false;
  }


  if (length == 1)
  {
    const VectorType t(0.0);
    ((TubePointType *)(this->GetPoint(0)))->SetTangentInObjectSpace(t);
    return true;
  }

  unsigned int it1 = 0;
  unsigned int it2 = 1;
  unsigned int it3 = 2;

  while (it3 < static_cast<unsigned int>(length))
  {
    // Compute tangent using the adjacent points
    const PointType & x1 = this->GetPoint(it1)->GetPositionInObjectSpace();
    const PointType & x3 = this->GetPoint(it3)->GetPositionInObjectSpace();
    double            l{ 0.0 };
    VectorType        t;
    for (unsigned int i = 0; i < TDimension; ++i)
    {
      t[i] = (x3[i] - x1[i]);
      l = l + t[i] * t[i];
    }
    l = std::sqrt(l);
    // If the adjacent points correspond, use the current point and one forward point
    if (Math::AlmostEquals(l, 0.0) || std::isnan(l))
    {
      const PointType & x2 = this->GetPoint(it2)->GetPositionInObjectSpace();
      l = 0;
      for (unsigned int i = 0; i < TDimension; ++i)
      {
        t[i] = (x3[i] - x2[i]);
        l = l + t[i] * t[i];
      }
      l = std::sqrt(l);
      // If the forward point and the current point correspond, then RemoveDuplicatePointsInObjectSpace was not called
      if (Math::AlmostEquals(l, 0.0) || std::isnan(l))
      {
        std::cerr << "TubeSpatialObject::ComputeTangentAndNormals() : "
                  << "length between two consecutive points is 0"
                  << " (use RemoveDuplicatePointsInObjectSpace())" << std::endl
                  << "   p1 = " << x1 << std::endl
                  << "   p2 = " << x2 << std::endl
                  << "   p3 = " << x3 << std::endl;
        return false;
      }
    }
    for (unsigned int i = 0; i < TDimension; ++i)
    {
      t[i] /= l;
    }

    ((TubePointType *)(this->GetPoint(it2)))->SetTangentInObjectSpace(t);
    ++it1;
    ++it2;
    ++it3;
  }

  // Calculate tangents are the first and last point on a tube
  {
    it1 = 0;
    it2 = 1;
    const VectorType & ta = ((TubePointType *)(this->GetPoint(it2)))->GetTangentInObjectSpace();
    ((TubePointType *)(this->GetPoint(it1)))->SetTangentInObjectSpace(ta);
  }
  {
    it1 = length - 1;
    it2 = length - 2;
    const VectorType & tb = ((TubePointType *)(this->GetPoint(it2)))->GetTangentInObjectSpace();
    ((TubePointType *)(this->GetPoint(it1)))->SetTangentInObjectSpace(tb);
  }

  CovariantVectorType prevN1{};
  prevN1[TDimension - 1] = 1;
  CovariantVectorType prevN2{};
  prevN2[TDimension - 2] = 1;

  it1 = 0;
  it2 = 1;
  while (it1 < static_cast<unsigned int>(length))
  {
    const VectorType & t = ((TubePointType *)(this->GetPoint(it1)))->GetTangentInObjectSpace();

    it2 = it1 + 1;
    if (it2 >= static_cast<unsigned int>(length))
    {
      it2 = it1 - 1;
    }
    const VectorType & t2 = ((TubePointType *)(this->GetPoint(it2)))->GetTangentInObjectSpace();

    // Compute the normal
    CovariantVectorType n1;
    if constexpr (TDimension == 2)
    {
      // The normal to the tangent in 2D is the orthogonal direction to the tangent.
      n1[0] = t[1];
      n1[1] = -t[0];
      if (it1 != 0)
      {
        double l{ 0.0 };
        for (unsigned int i = 0; i < TDimension; ++i)
        {
          l += n1[i] * prevN1[i];
        }
        if (l < 0)
        {
          n1 *= -1;
        }
      }
      ((TubePointType *)(this->GetPoint(it1)))->SetNormal1InObjectSpace(n1);
      prevN1 = n1;
    }
    else if constexpr (TDimension == 3)
    {
      // The normal to the tangent in 3D is the cross product of adjacent tangent directions.
      n1[0] = t[1] * t2[2] - t[2] * t2[1];
      n1[1] = t[2] * t2[0] - t[0] * t2[2];
      n1[2] = t[0] * t2[1] - t[1] * t2[0];

      const double l_n1 = std::sqrt(n1[0] * n1[0] + n1[1] * n1[1] + n1[2] * n1[2]);
      if (Math::AlmostEquals(l_n1, 0.0) || std::isnan(l_n1))
      {
        if (it1 != 0)
        {
          n1 = prevN1;
        }
        else
        {
          // if the normal is null, pick an orthogonal direction
          double d = std::sqrt(t[0] * t[0] + t[1] * t[1]);
          if (d != 0)
          {
            n1[0] = t[1] / d;
            n1[1] = -t[0] / d;
            n1[2] = 0;
          }
          else
          {
            d = std::sqrt(t[1] * t[1] + t[2] * t[2]);
            if (d != 0)
            {
              n1[0] = 0;
              n1[1] = t[2] / d;
              n1[2] = -t[1] / d;
            }
            else
            {
              n1 = prevN1;
            }
          }
        }
      }
      else
      {
        n1 /= l_n1;
      }

      // The second normal is the cross product of the tangent and the first normal
      CovariantVectorType n2;
      n2[0] = t[1] * n1[2] - t[2] * n1[1];
      n2[1] = t[2] * n1[0] - t[0] * n1[2];
      n2[2] = t[0] * n1[1] - t[1] * n1[0];

      const double l_n2 = std::sqrt(n2[0] * n2[0] + n2[1] * n2[1] + n2[2] * n2[2]);
      if (Math::AlmostEquals(l_n2, 0.0) || std::isnan(l_n2))
      {
        n2 = prevN2;
      }
      else
      {
        n2 /= l_n2;
      }

      if (it1 != 0)
      {
        double l = 0;
        for (unsigned int i = 0; i < TDimension; ++i)
        {
          l += n1[i] * prevN1[i];
        }
        if (l < 0)
        {
          n1 *= -1;
        }
        l = 0;
        for (unsigned int i = 0; i < TDimension; ++i)
        {
          l += n2[i] * prevN2[i];
        }
        if (l < 0)
        {
          n2 *= -1;
        }
      }

      ((TubePointType *)(this->GetPoint(it1)))->SetNormal1InObjectSpace(n1);
      ((TubePointType *)(this->GetPoint(it1)))->SetNormal2InObjectSpace(n2);

      prevN1 = n1;
      prevN2 = n2;
    }

    ++it1;
  }

  return true;
}


} // end namespace itk

#endif // end itkTubeSpatialObject_hxx
