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
#ifndef itkFEMSolverCrankNicolson_hxx
#define itkFEMSolverCrankNicolson_hxx


#include "itkFEMLoadNode.h"
#include "itkFEMLoadElementBase.h"
#include "itkFEMLoadBC.h"
#include "itkFEMLoadBCMFC.h"
#include "itkFEMLoadLandmark.h"
#include "itkMath.h"

namespace itk
{
namespace fem
{
#define TOTE


template <unsigned int VDimension>
SolverCrankNicolson<VDimension>::SolverCrankNicolson()
  : m_TimeStep(0.5)
  , m_Rho(1.)
  , m_Alpha(0.5)
  , m_CurrentMaxSolution(1.0)
  , m_UseMassMatrix(true)
  , m_ForceTotalIndex(5)
  , m_ForceTMinus1Index(2)
  , m_SolutionTMinus1Index(2)
  , m_SolutionVectorTMinus1Index(3)
  , m_TotalSolutionIndex(1)
  , m_DifferenceMatrixIndex(1)
  , m_DiffMatrixBySolutionTMinus1Index(4)
{
  // BUG FIXME NOT SURE IF SOLVER IS USING VECTOR INDEX 1 FOR BCs
  // vector
  // vector
  // vector
  // vector
  // vector
  // solution
  // solution
  // solution
  // matrix
  // matrix
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::InitializeForSolution()
{
  this->m_LinearSystem->SetSystemOrder(this->m_NGFN + this->m_NMFC);
  this->m_LinearSystem->SetNumberOfVectors(6);
  this->m_LinearSystem->SetNumberOfSolutions(3);
  this->m_LinearSystem->SetNumberOfMatrices(2);
  this->m_LinearSystem->InitializeMatrix(m_SumMatrixIndex);
  this->m_LinearSystem->InitializeMatrix(m_DifferenceMatrixIndex);
  this->m_LinearSystem->InitializeVector(m_ForceTIndex);
  this->m_LinearSystem->InitializeVector(m_ForceTotalIndex);
  this->m_LinearSystem->InitializeVector(m_ForceTMinus1Index);
  this->m_LinearSystem->InitializeVector(m_SolutionVectorTMinus1Index);
  this->m_LinearSystem->InitializeVector(m_DiffMatrixBySolutionTMinus1Index);
  this->m_LinearSystem->InitializeSolution(m_SolutionTIndex);
  this->m_LinearSystem->InitializeSolution(m_TotalSolutionIndex);
  this->m_LinearSystem->InitializeSolution(m_SolutionTMinus1Index);
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::AssembleKandM()
{
  // If no DOFs exist in a system, we have nothing to do
  if (this->m_NGFN <= 0)
  {
    return;
  }

  Float lhsval;
  Float rhsval;
  this->m_NMFC = 0; // number of MFC in a system

  // Temporary storage for pointers to LoadBCMFC objects
  using MFCArray = std::vector<LoadBCMFC::Pointer>;
  MFCArray mfcLoads;

  //
  // Before we can start the assembly procedure, we need to know,
  // how many boundary conditions (MFCs) there are in a system.
  //
  mfcLoads.clear();

  int numLoads = this->m_FEMObject->GetLoadContainer()->Size();
  for (int l = 0; l < numLoads; ++l)
  {
    if (LoadBCMFC::Pointer l1 = dynamic_cast<LoadBCMFC *>(this->m_FEMObject->GetLoad(l).GetPointer()))
    {
      // Store the index of an LoadBCMFC object for later
      l1->SetIndex(this->m_NMFC);
      mfcLoads.push_back(l1);
      // Increase the number of MFC
      this->m_NMFC++;
    }
  }
  //
  // Now we can assemble the master stiffness matrix
  // from element stiffness matrices.
  //
  InitializeForSolution();

  //
  // Step over all elements
  //
  int numElements = this->m_FEMObject->GetElementContainer()->Size();
  for (int e = 0; e < numElements; ++e)
  {
    vnl_matrix<Float> Ke;
    // Copy the element stiffness matrix for faster access.
    this->m_FEMObject->GetElement(e)->GetStiffnessMatrix(Ke);
    vnl_matrix<Float> Me;
    // Copy the element mass matrix for faster access.
    this->m_FEMObject->GetElement(e)->GetMassMatrix(Me);
    // ... same for element DOF
    int Ne = this->m_FEMObject->GetElement(e)->GetNumberOfDegreesOfFreedom();

    Me = Me * m_Rho;
    // Step over all rows in in element matrix
    for (int j = 0; j < Ne; ++j)
    {
      // Step over all columns in in element matrix
      for (int k = 0; k < Ne; ++k)
      {
        // Error checking. all GFN should be =>0 and <NGFN
        if (this->m_FEMObject->GetElement(e)->GetDegreeOfFreedom(j) >= this->m_NGFN ||
            this->m_FEMObject->GetElement(e)->GetDegreeOfFreedom(k) >= this->m_NGFN)
        {
          throw FEMExceptionSolution(__FILE__, __LINE__, "SolverCrankNicolson::AssembleKandM()", "Illegal GFN!");
        }

        // Here we finally update the corresponding element
        // in the master stiffness matrix. We first check if
        // element in Ke is zero, to prevent zeros from being
        // allocated in sparse matrix.
        //
        if (Math::NotExactlyEquals(Ke(j, k), Float(0.0)) || Math::NotExactlyEquals(Me(j, k), Float(0.0)))
        {
          // Left hand side matrix
          lhsval = (Me(j, k) + m_Alpha * m_TimeStep * Ke(j, k));
          this->m_LinearSystem->AddMatrixValue(this->m_FEMObject->GetElement(e)->GetDegreeOfFreedom(j),
                                               this->m_FEMObject->GetElement(e)->GetDegreeOfFreedom(k),
                                               lhsval,
                                               m_SumMatrixIndex);
          // Right hand side matrix
          rhsval = (Me(j, k) - (1. - m_Alpha) * m_TimeStep * Ke(j, k));
          this->m_LinearSystem->AddMatrixValue(this->m_FEMObject->GetElement(e)->GetDegreeOfFreedom(j),
                                               this->m_FEMObject->GetElement(e)->GetDegreeOfFreedom(k),
                                               rhsval,
                                               m_DifferenceMatrixIndex);
        }
      }
    }
  }
  //
  // Step over all the loads to add the landmark contributions to the
  // appropriate place in the stiffness matrix
  //
  // int numLoads = m_FEMObject->GetLoadContainer()->Size();
  for (int l2 = 0; l2 < numLoads; ++l2)
  {
    if (LoadLandmark::Pointer l3 = dynamic_cast<LoadLandmark *>(this->m_FEMObject->GetLoad(l2).GetPointer()))
    {
      Element::ConstPointer ep = (l3->GetElementArray()[0]);
      Element::MatrixType   Le;
      ep->GetLandmarkContributionMatrix(l3->GetEta(), Le);
      int Ne = ep->GetNumberOfDegreesOfFreedom();
      // Step over all rows in element matrix
      for (int j = 0; j < Ne; ++j)
      {
        // Step over all columns in element matrix
        for (int k = 0; k < Ne; ++k)
        {
          // error checking, all GFN should be >=0 and < NGFN
          if (ep->GetDegreeOfFreedom(j) >= this->m_NGFN || ep->GetDegreeOfFreedom(k) >= this->m_NGFN)
          {
            throw FEMExceptionSolution(__FILE__, __LINE__, "SolverCrankNicolson::AssembleKandM()", "Illegal GFN!");
          }

          // Now update the corresponding element in the master
          // stiffness matrix and omit the zeros for the sparseness
          if (Math::NotExactlyEquals(Le(j, k), Float(0.0)))
          {
            // lhs matrix
            lhsval = m_Alpha * m_TimeStep * Le(j, k);
            this->m_LinearSystem->AddMatrixValue(
              ep->GetDegreeOfFreedom(j), ep->GetDegreeOfFreedom(k), lhsval, m_SumMatrixIndex);
            // rhs matrix
            rhsval = (1. - m_Alpha) * m_TimeStep * Le(j, k);
            this->m_LinearSystem->AddMatrixValue(
              ep->GetDegreeOfFreedom(j), ep->GetDegreeOfFreedom(k), rhsval, m_DifferenceMatrixIndex);
          }
        }
      }
    }
  }

  // Step over all types of BCs
  this->ApplyBC(); // BUG -- are BCs applied appropriately to the problem?
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::AssembleFforTimeStep(int dim)
{
  // If no DOFs exist in a system, we have nothing to do
  if (this->m_NGFN <= 0)
  {
    return;
  }

  this->AssembleF(dim); // assuming AssembleF uses index 0 in vector!

  using BCTermType = std::map<Element::DegreeOfFreedomIDType, Float>;
  BCTermType bcterm;

  int numLoads = this->m_FEMObject->GetLoadContainer()->Size();
  for (int l2 = 0; l2 < numLoads; ++l2)
  {
    if (LoadBC::Pointer l1 = dynamic_cast<LoadBC *>(this->m_FEMObject->GetLoad(l2).GetPointer()))
    {
      bcterm[l1->GetElement()->GetDegreeOfFreedom(l1->GetDegreeOfFreedom())] = l1->GetValue()[dim];
    }
  } // end for LoadArray::iterator l
  // Now set the solution t_minus1 vector to fit the BCs
  for (auto & q : bcterm)
  {
    this->m_LinearSystem->SetVectorValue(q.first, 0.0, m_SolutionVectorTMinus1Index); // FIXME?
    this->m_LinearSystem->SetSolutionValue(q.first, 0.0, m_SolutionTMinus1Index);     // FIXME?
    this->m_LinearSystem->SetSolutionValue(q.first, 0.0, m_TotalSolutionIndex);
  }

  this->m_LinearSystem->MultiplyMatrixVector(
    m_DiffMatrixBySolutionTMinus1Index, m_DifferenceMatrixIndex, m_SolutionVectorTMinus1Index);
  for (unsigned int index = 0; index < this->m_NGFN; ++index)
  {
    RecomputeForceVector(index);
  }
  // Now set the solution and force vector to fit the BCs
  for (auto & q : bcterm)
  {
    this->m_LinearSystem->SetVectorValue(q.first, q.second, m_ForceTIndex);
  }
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::RecomputeForceVector(unsigned int index)
{ //
  Float ft = this->m_LinearSystem->GetVectorValue(index, m_ForceTIndex);
  Float ftm1 = this->m_LinearSystem->GetVectorValue(index, m_ForceTMinus1Index);
  Float utm1 = this->m_LinearSystem->GetVectorValue(index, m_DiffMatrixBySolutionTMinus1Index);
  Float f = m_TimeStep * (m_Alpha * ft + (1. - m_Alpha) * ftm1) + utm1;

  this->m_LinearSystem->SetVectorValue(index, f, m_ForceTIndex);
}

// ----------------------------------------------------------------------------
template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::GenerateData()
{
  // Call Solver
  this->RunSolver();
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::RunSolver()
{
  if (m_Iterations == 0)
  {
    if (m_UseMassMatrix)
    {
      this->AssembleKandM();
    }
    else
    {
      this->InitializeForSolution();
      this->AssembleK();
    }
  }

  if (m_UseMassMatrix)
  {
    this->AssembleFforTimeStep();
  }
  else
  {
    this->AssembleF();
  }

  // FIXME - must verify that this is correct use of wrapper
  // FIXME Initialize the solution vector
  this->m_LinearSystem->InitializeSolution(m_SolutionTIndex);
  this->m_LinearSystem->Solve();

  ++m_Iterations;
  // call this externally    AddToDisplacements();
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::FindBracketingTriplet(Float * a, Float * b, Float * c)
{
  // In 1-D domain, we want to find a < b < c , s.t.  f(b) < f(a) && f(b) < f(c)
  // See Numerical Recipes

  Float Gold = 1.618034;
  Float Glimit = 100.0;
  Float Tiny = 1.e-20;

  Float ax, bx, cx;

  ax = 0.0;
  bx = 1.;
  Float fc;
  Float fa = itk::Math::abs(EvaluateResidual(ax));
  Float fb = itk::Math::abs(EvaluateResidual(bx));

  Float ulim, u, r, q, fu, dum;

  if (fb > fa)
  {
    dum = ax;
    ax = bx;
    bx = dum;
    dum = fb;
    fb = fa;
    fa = dum;
  }

  cx = bx + Gold * (bx - ax); // first guess for c - the 3rd pt needed to
                              // bracket the min
  fc = itk::Math::abs(EvaluateResidual(cx));

  while( fb > fc /*&& itk::Math::abs(ax) < 3. && itk::Math::abs(bx) < 3. && itk::Math::abs(cx) <
                    3.*/)
  {
    r = (bx - ax) * (fb - fc);
    q = (bx - cx) * (fb - fa);
    Float denom = (2.0 * GSSign(GSMax(itk::Math::abs(q - r), Tiny), q - r));
    u = (bx) - ((bx - cx) * q - (bx - ax) * r) / denom;
    ulim = bx + Glimit * (cx - bx);
    if ((bx - u) * (u - cx) > 0.0)
    {
      fu = itk::Math::abs(EvaluateResidual(u));
      if (fu < fc)
      {
        ax = bx;
        bx = u;
        *a = ax;
        *b = bx;
        *c = cx;
        return;
      }
      else if (fu > fb)
      {
        cx = u;
        *a = ax;
        *b = bx;
        *c = cx;
        return;
      }

      u = cx + Gold * (cx - bx);
      fu = itk::Math::abs(EvaluateResidual(u));
    }
    else if ((cx - u) * (u - ulim) > 0.0)
    {
      fu = itk::Math::abs(EvaluateResidual(u));
      if (fu < fc)
      {
        bx = cx;
        cx = u;
        u = cx + Gold * (cx - bx);
        fb = fc;
        fc = fu;
        fu = itk::Math::abs(EvaluateResidual(u));
      }
    }
    else if ((u - ulim) * (ulim - cx) >= 0.0)
    {
      u = ulim;
      fu = itk::Math::abs(EvaluateResidual(u));
    }
    else
    {
      u = cx + Gold * (cx - bx);
      fu = itk::Math::abs(EvaluateResidual(u));
    }

    ax = bx;
    bx = cx;
    cx = u;
    fa = fb;
    fb = fc;
    fc = fu;
  }

  if (itk::Math::abs(ax) > 1.e3 || itk::Math::abs(bx) > 1.e3 || itk::Math::abs(cx) > 1.e3)
  {
    ax = -2.0;
    bx = 1.0;
    cx = 2.0;
  } // to avoid crazy numbers caused by bad bracket (u goes nuts)

  *a = ax;
  *b = bx;
  *c = cx;
}

template <unsigned int VDimension>
Element::Float
SolverCrankNicolson<VDimension>::BrentsMethod(Float tol, unsigned int MaxIters)
{
  // We should now have a, b and c, as well as f(a), f(b), f(c),
  // where b gives the minimum energy position;

  Float CGOLD = 0.3819660;
  Float ZEPS = 1.e-10;

  Float ax = 0.0, bx = 1.0, cx = 2.0;

  FindBracketingTriplet(&ax, &bx, &cx);

  Float xmin;

  unsigned int iter;

  Float a, b, d = 0., etemp, fu, fv, fw, fx, p, q, r, tol1, tol2, u, v, w, x, xm;

  Float e = 0.0; // the distance moved on the step before last;

  a = ((ax < cx) ? ax : cx);
  b = ((ax > cx) ? ax : cx);

  x = w = v = bx;
  fw = fv = fx = itk::Math::abs(EvaluateResidual(x));
  for (iter = 1; iter <= MaxIters; ++iter)
  {
    xm = 0.5 * (a + b);
    tol2 = 2.0 * (tol1 = tol * itk::Math::abs(x) + ZEPS);
    if (itk::Math::abs(x - xm) <= (tol2 - 0.5 * (b - a)))
    {
      xmin = x;
      SetEnergyToMin(xmin);
      return fx;
    }

    if (itk::Math::abs(e) > tol1)
    {
      r = (x - w) * (fx - fv);
      q = (x - v) * (fx - fw);
      p = (x - v) * q - (x - w) * r;
      q = 2.0 * (q - r);
      if (q > 0.0)
      {
        p = -1. * p;
      }
      q = itk::Math::abs(q);
      etemp = e;
      e = d;
      if (itk::Math::abs(p) >= itk::Math::abs(0.5 * q * etemp) || p <= q * (a - x) || p >= q * (b - x))
      {
        d = CGOLD * (e = (x >= xm ? a - x : b - x));
      }
      else
      {
        if (q == 0.0)
        {
          q = q + ZEPS;
        }
        d = p / q;
        u = x + d;
        if (u - a < tol2 || b - u < tol2)
        {
          d = GSSign(tol1, xm - x);
        }
      }
    }
    else
    {
      d = CGOLD * (e = (x >= xm ? a - x : b - x));
    }

    u = (itk::Math::abs(d) >= tol1 ? x + d : x + GSSign(tol1, d));
    fu = itk::Math::abs(EvaluateResidual(u));
    if (fu <= fx)
    {
      if (u >= x)
      {
        a = x;
      }
      else
      {
        b = x;
      }
      v = w;
      w = x;
      x = u;
      fv = fw;
      fw = fx;
      fx = fu;
    }
    else
    {
      if (u < x)
      {
        a = u;
      }
      else
      {
        b = u;
      }
      if (fu <= fw || Math::ExactlyEquals(w, x))
      {
        v = w;
        w = u;
        fv = fw;
        fw = fu;
      }
      else if (fu <= fv || Math::ExactlyEquals(v, x) || Math::ExactlyEquals(v, w))
      {
        v = u;
        fv = fu;
      }
    }
  }
  xmin = x;
  SetEnergyToMin(xmin);
  return fx;
}

template <unsigned int VDimension>
Element::Float
SolverCrankNicolson<VDimension>::GoldenSection(Float tol, unsigned int MaxIters)
{
  // We should now have a, b and c, as well as f(a), f(b), f(c),
  // where b gives the minimum energy position;

  Float ax, bx, cx;

  FindBracketingTriplet(&ax, &bx, &cx);
  Float xmin, fmin;

  Float f1, f2, x0, x1, x2, x3;

  Float R = 0.6180339;
  Float C = (1.0 - R);

  x0 = ax;
  x3 = cx;
  if (itk::Math::abs(cx - bx) > itk::Math::abs(bx - ax))
  {
    x1 = bx;
    x2 = bx + C * (cx - bx);
  }
  else
  {
    x2 = bx;
    x1 = bx - C * (bx - ax);
  }
  f1 = itk::Math::abs(EvaluateResidual(x1));
  f2 = itk::Math::abs(EvaluateResidual(x2));
  unsigned int iters = 0;
  while (itk::Math::abs(x3 - x0) > tol * (itk::Math::abs(x1) + itk::Math::abs(x2)) && iters < MaxIters)
  {
    ++iters;
    if (f2 < f1)
    {
      x0 = x1;
      x1 = x2;
      x2 = R * x1 + C * x3;
      f1 = f2;
      f2 = itk::Math::abs(EvaluateResidual(x2));
    }
    else
    {
      x3 = x2;
      x2 = x1;
      x1 = R * x2 + C * x0;
      f2 = f1;
      f1 = itk::Math::abs(EvaluateResidual(x1));
    }
  }

  if (f1 < f2)
  {
    xmin = x1;
    fmin = f1;
  }
  else
  {
    xmin = x2;
    fmin = f2;
  }

  SetEnergyToMin(xmin);
  return fmin;
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::SetEnergyToMin(Float xmin)
{
  for (unsigned int j = 0; j < this->m_NGFN; ++j)
  {
    Float SolVal;
    Float FVal;
#ifdef LOCE
    SolVal = xmin * this->m_LinearSystem->GetSolutionValue(j, m_SolutionTIndex) +
             (1. - xmin) * this->m_LinearSystem->GetSolutionValue(j, m_SolutionTMinus1Index);

    FVal = xmin * this->m_LinearSystem->GetVectorValue(j, m_ForceTIndex) +
           (1. - xmin) * this->m_LinearSystem->GetVectorValue(j, m_ForceTMinus1Index);
#endif
#ifdef TOTE
    SolVal = xmin * this->m_LinearSystem->GetSolutionValue(j, m_SolutionTIndex); // FOR TOT E
    FVal = xmin * this->m_LinearSystem->GetVectorValue(j, m_ForceTIndex);
#endif
    this->m_LinearSystem->SetSolutionValue(j, SolVal, m_SolutionTIndex);
    this->m_LinearSystem->SetVectorValue(j, FVal, m_ForceTIndex);
  }
}

template <unsigned int VDimension>
Element::Float
SolverCrankNicolson<VDimension>::GetDeformationEnergy(Float t)
{
  Float DeformationEnergy = 0.0;
  Float iSolVal, jSolVal;
  for (unsigned int i = 0; i < this->m_NGFN; ++i)
  {
// forming  U^T F
#ifdef LOCE
    iSolVal = t * (this->m_LinearSystem->GetSolutionValue(i, m_SolutionTIndex)) +
              (1. - t) * this->m_LinearSystem->GetSolutionValue(i, m_SolutionTMinus1Index);
#endif
#ifdef TOTE
    iSolVal = t * (this->m_LinearSystem->GetSolutionValue(i, m_SolutionTIndex));
#endif
    // forming U^T K U
    Float TempRowVal = 0.0;
    for (unsigned int j = 0; j < this->m_NGFN; ++j)
    {
#ifdef LOCE
      jSolVal = t * (this->m_LinearSystem->GetSolutionValue(j, m_SolutionTIndex)) +
                (1. - t) * this->m_LinearSystem->GetSolutionValue(j, m_SolutionTMinus1Index);
#endif
#ifdef TOTE
      jSolVal = t * (this->m_LinearSystem->GetSolutionValue(j, m_SolutionTIndex)) +
                this->m_LinearSystem->GetSolutionValue(j, m_TotalSolutionIndex); // FOR TOT E
#endif
      TempRowVal += this->m_LinearSystem->GetMatrixValue(i, j, m_SumMatrixIndex) * jSolVal;
    }
    DeformationEnergy += iSolVal * TempRowVal;
  }
  return DeformationEnergy;
}

template <unsigned int VDimension>
Element::Float
SolverCrankNicolson<VDimension>::EvaluateResidual(Float t)
{
  Float ForceEnergy = 0.0, FVal = 0.0;
  Float DeformationEnergy = 0.0;
  Float iSolVal, jSolVal;
  for (unsigned int i = 0; i < this->m_NGFN; ++i)
  {
// forming  U^T F
#ifdef LOCE
    iSolVal = t * (this->m_LinearSystem->GetSolutionValue(i, m_SolutionTIndex)) +
              (1. - t) * this->m_LinearSystem->GetSolutionValue(i, m_SolutionTMinus1Index);
    FVal = this->m_LinearSystem->GetVectorValue(i, m_ForceTIndex);
    FVal = t * FVal + (1. - t) * this->m_LinearSystem->GetVectorValue(i, m_ForceTMinus1Index);

    ForceEnergy += iSolVal * FVal;
#endif
#ifdef TOTE
    FVal = FVal + 0.0;
    iSolVal = t * (this->m_LinearSystem->GetSolutionValue(i, m_SolutionTIndex)) +
              this->m_LinearSystem->GetSolutionValue(i, m_TotalSolutionIndex); // FOR TOT E
    ForceEnergy += iSolVal * (this->m_LinearSystem->GetVectorValue(i, m_ForceTotalIndex) +
                              t * this->m_LinearSystem->GetVectorValue(i, m_ForceTIndex)); //
                                                                                           // FOR
                                                                                           // TOT
                                                                                           // E
#endif
    // forming U^T K U
    Float TempRowVal = 0.0;
    for (unsigned int j = 0; j < this->m_NGFN; ++j)
    {
#ifdef LOCE
      jSolVal = t * (this->m_LinearSystem->GetSolutionValue(j, m_SolutionTIndex)) +
                (1. - t) * this->m_LinearSystem->GetSolutionValue(j, m_SolutionTMinus1Index);
#endif
#ifdef TOTE
      jSolVal = t * (this->m_LinearSystem->GetSolutionValue(j, m_SolutionTIndex)) +
                this->m_LinearSystem->GetSolutionValue(j, m_TotalSolutionIndex); // FOR TOT E
#endif
      TempRowVal += this->m_LinearSystem->GetMatrixValue(i, j, m_SumMatrixIndex) * jSolVal;
    }
    DeformationEnergy += iSolVal * TempRowVal;
  }
  auto Energy = (Float)itk::Math::abs(DeformationEnergy - ForceEnergy);
  return Energy;
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::AddToDisplacements(Float optimum)
{
  //
  // Copy the resulting displacements from
  // solution vector back to node objects.
  //
  Float maxs = 0.0, CurrentTotSolution, CurrentSolution, CurrentForce;
  Float mins2 = 0.0, maxs2 = 0.0;
  Float absmax = 0.0;
  for (unsigned int i = 0; i < this->m_NGFN; ++i)
  {
#ifdef TOTE
    CurrentSolution = this->m_LinearSystem->GetSolutionValue(i, m_SolutionTIndex);
#endif
    if (CurrentSolution < mins2)
    {
      mins2 = CurrentSolution;
    }
    else if (CurrentSolution > maxs2)
    {
      maxs2 = CurrentSolution;
    }
    if (itk::Math::abs(CurrentSolution) > absmax)
    {
      absmax = itk::Math::abs(CurrentSolution);
    }

//  note: set rather than add - i.e. last solution of system not total solution
#ifdef LOCE
    CurrentSolution = optimum * this->m_LinearSystem->GetSolutionValue(i, m_SolutionTIndex) +
                      (1. - optimum) * this->m_LinearSystem->GetVectorValue(i, m_SolutionVectorTMinus1Index);
    CurrentForce = optimum * this->m_LinearSystem->GetVectorValue(i, m_ForceTIndex) +
                   (1. - optimum) * this->m_LinearSystem->GetVectorValue(i, m_ForceTMinus1Index);
    this->m_LinearSystem->SetVectorValue(i, CurrentSolution, m_SolutionVectorTMinus1Index);
    this->m_LinearSystem->SetSolutionValue(i, CurrentSolution, m_SolutionTMinus1Index);
    this->m_LinearSystem->SetVectorValue(i, CurrentForce, m_ForceTMinus1Index); // now set t
                                                                                // minus one
                                                                                // force vector
                                                                                // correctly
#endif
#ifdef TOTE
    CurrentSolution = optimum * CurrentSolution;
    CurrentForce = optimum * this->m_LinearSystem->GetVectorValue(i, m_ForceTIndex);
    this->m_LinearSystem->SetVectorValue(i, CurrentSolution, m_SolutionVectorTMinus1Index); // FOR
    // TOT
    // E
    this->m_LinearSystem->SetSolutionValue(i, CurrentSolution, m_SolutionTMinus1Index); // FOR
    // TOT
    // E
    this->m_LinearSystem->SetVectorValue(i, CurrentForce, m_ForceTMinus1Index);
#endif

    this->m_LinearSystem->AddSolutionValue(i, CurrentSolution, m_TotalSolutionIndex);
    this->m_LinearSystem->AddVectorValue(i, CurrentForce, m_ForceTotalIndex);
    CurrentTotSolution = this->m_LinearSystem->GetSolutionValue(i, m_TotalSolutionIndex);

    if (itk::Math::abs(CurrentTotSolution) > maxs)
    {
      maxs = itk::Math::abs(CurrentTotSolution);
    }
  }

  m_CurrentMaxSolution = absmax;
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::PrintMinMaxOfSolution()
{
  //
  // Copy the resulting displacements from
  // solution vector back to node objects.
  //
  Float mins = 0.0, maxs = 0.0;
  Float mins2 = 0.0, maxs2 = 0.0;
  for (unsigned int i = 0; i < this->m_NGFN; ++i)
  {
    Float CurrentSolution = this->m_LinearSystem->GetSolutionValue(i, m_SolutionTIndex);
    if (CurrentSolution < mins2)
    {
      mins2 = CurrentSolution;
    }
    else if (CurrentSolution > maxs2)
    {
      maxs2 = CurrentSolution;
    }
    CurrentSolution = this->m_LinearSystem->GetSolutionValue(i, m_TotalSolutionIndex);
    if (CurrentSolution < mins)
    {
      mins = CurrentSolution;
    }
    else if (CurrentSolution > maxs)
    {
      maxs = CurrentSolution;
    }
  }
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::AverageLastTwoDisplacements(Float t)
{
  Float maxs = 0.0;
  for (unsigned int i = 0; i < this->m_NGFN; ++i)
  {
    Float temp = this->m_LinearSystem->GetSolutionValue(i, m_SolutionTIndex);
    Float temp2 = this->m_LinearSystem->GetSolutionValue(i, m_SolutionTMinus1Index);
    Float newsol = t * (temp) + (1. - t) * temp2;
    this->m_LinearSystem->SetSolutionValue(i, newsol, m_SolutionTMinus1Index);
    this->m_LinearSystem->SetVectorValue(i, newsol, m_SolutionVectorTMinus1Index);
    this->m_LinearSystem->SetSolutionValue(i, newsol, m_SolutionTIndex);
    if (newsol > maxs)
    {
      maxs = newsol;
    }
  }
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::ZeroVector(int which)
{
  for (unsigned int i = 0; i < this->m_NGFN; ++i)
  {
    this->m_LinearSystem->SetVectorValue(i, 0.0, which);
  }
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::PrintDisplacements()
{
  std::cout << " printing current displacements " << std::endl;
  for (unsigned int i = 0; i < this->m_NGFN; ++i)
  {
    std::cout << this->m_LinearSystem->GetSolutionValue(i, m_TotalSolutionIndex) << std::endl;
  }
}

template <unsigned int VDimension>
void
SolverCrankNicolson<VDimension>::PrintForce()
{
  std::cout << " printing current forces " << std::endl;
  for (unsigned int i = 0; i < this->m_NGFN; ++i)
  {
    std::cout << this->m_LinearSystem->GetVectorValue(i, m_ForceTIndex) << std::endl;
  }
}

} // end namespace fem
} // end namespace itk

#endif
