/*************************************************************************
 *  Copyright (c) 2017.
 *  All rights reserved.
 *  This file is part of the CLAIRE library.
 *
 *  CLAIRE is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CLAIRE is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CLAIRE. If not, see <http://www.gnu.org/licenses/>.
 ************************************************************************/

#ifndef _TESTINTERPOLATION_CPP_
#define _TESTINTERPOLATION_CPP_

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "UnitTestOpt.hpp"
#include "CLAIRE.hpp"
#include "VecField.hpp"

/********************************************************************
 * @brief compute synthetic image
 *******************************************************************/
PetscErrorCode ComputeSyntheticData(Vec& m, reg::RegOpt* opt) {
    PetscErrorCode ierr = 0;
    ScalarType *p_m = NULL;
    ScalarType hx[3], x1, x2, x3;
    IntType i, nl, ng;
    PetscFunctionBegin;

    opt->Enter(__func__);

    // get local and global size
    nl = opt->m_Domain.nl;
    ng = opt->m_Domain.ng;

    // get grid size
    hx[0] = opt->m_Domain.hx[0];
    hx[1] = opt->m_Domain.hx[1];
    hx[2] = opt->m_Domain.hx[2];

    // allocate data
    if (m == NULL) {
        ierr = reg::VecCreate(m, nl, ng); CHKERRQ(ierr);
    }

    ierr = VecGetArray(m, &p_m); CHKERRQ(ierr);
    for (IntType i1 = 0; i1 < opt->m_Domain.isize[0]; ++i1) {  // x1
        for (IntType i2 = 0; i2 < opt->m_Domain.isize[1]; ++i2) {  // x2
            for (IntType i3 = 0; i3 < opt->m_Domain.isize[2]; ++i3) {  // x3
                // compute coordinates (nodal grid)
                x1 = hx[0]*static_cast<ScalarType>(i1 + opt->m_Domain.istart[0]);
                x2 = hx[1]*static_cast<ScalarType>(i2 + opt->m_Domain.istart[1]);
                x3 = hx[2]*static_cast<ScalarType>(i3 + opt->m_Domain.istart[2]);

                // compute linear / flat index
                i = reg::GetLinearIndex(i1, i2, i3, opt->m_Domain.isize);

                p_m[i] =  (PetscSinReal(x1)*PetscSinReal(x1)
                          + PetscSinReal(x2)*PetscSinReal(x2)
                          + PetscSinReal(x3)*PetscSinReal(x3))/3.0;


            }  // i1
        }  // i2
    }  // i3
    ierr = VecRestoreArray(m, &p_m); CHKERRQ(ierr);

    opt->Exit(__func__);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief compute synthetic velocity field
 *******************************************************************/
PetscErrorCode ComputeSyntheticData(reg::VecField*& v, reg::RegOpt* opt) {
    PetscErrorCode ierr = 0;
    ScalarType *p_v1 = NULL, *p_v2 = NULL, *p_v3 = NULL;
    ScalarType hx[3], x1, x2, x3;
    IntType i, vcase = 1;
    PetscFunctionBegin;

    opt->Enter(__func__);

    // get grid size
    hx[0] = opt->m_Domain.hx[0];
    hx[1] = opt->m_Domain.hx[1];
    hx[2] = opt->m_Domain.hx[2];

    // allocate velocity field
    if (v == NULL) {
        try {v = new reg::VecField(opt);}
        catch (std::bad_alloc& err) {
            ierr = reg::ThrowError(err); CHKERRQ(ierr);
        }
    }

    // ierr = v->GetArrays(p_v1, p_v2, p_v3); CHKERRQ(ierr);
    ierr = VecGetArray(v->m_X1, &p_v1); CHKERRQ(ierr);
    ierr = VecGetArray(v->m_X2, &p_v2); CHKERRQ(ierr);
    ierr = VecGetArray(v->m_X3, &p_v3); CHKERRQ(ierr);

    for (IntType i1 = 0; i1 < opt->m_Domain.isize[0]; ++i1) {  // x1
        for (IntType i2 = 0; i2 < opt->m_Domain.isize[1]; ++i2) {  // x2
            for (IntType i3 = 0; i3 < opt->m_Domain.isize[2]; ++i3) {  // x3

                // compute coordinates (nodal grid)
                x1 = hx[0]*static_cast<ScalarType>(i1 + opt->m_Domain.istart[0]);
                x2 = hx[1]*static_cast<ScalarType>(i2 + opt->m_Domain.istart[1]);
                x3 = hx[2]*static_cast<ScalarType>(i3 + opt->m_Domain.istart[2]);

                // compute linear / flat index
                i = reg::GetLinearIndex(i1, i2, i3, opt->m_Domain.isize);

                if (vcase == 0) {
                    // compute the velocity field
                    p_v1[i] = 0.5*PetscSinReal(x3)*PetscCosReal(x2)*PetscSinReal(x2);
                    p_v2[i] = 0.5*PetscSinReal(x1)*PetscCosReal(x3)*PetscSinReal(x3);
                    p_v3[i] = 0.5*PetscSinReal(x2)*PetscCosReal(x1)*PetscSinReal(x1);
                } else if (vcase == 1) {
                    // compute the velocity field
                    p_v1[i] = PetscSinReal(x3)*PetscCosReal(x2)*PetscSinReal(x2);
                    p_v2[i] = PetscSinReal(x1)*PetscCosReal(x3)*PetscSinReal(x3);
                    p_v3[i] = PetscSinReal(x2)*PetscCosReal(x1)*PetscSinReal(x1);
                } else if (vcase == 2) {
                    // compute divergence freee velocity field
                    p_v1[i] = PetscCosReal(x2)*PetscCosReal(x3);
                    p_v2[i] = PetscSinReal(x3)*PetscSinReal(x1);
                    p_v3[i] = PetscCosReal(x1)*PetscCosReal(x2);
                } else if (vcase == 3) {
                    p_v1[i] = 0.5;
                    p_v2[i] = 1;
                    p_v3[i] = 0.5;
                } else if (vcase == 4) {
                    p_v1[i] = 0.0;
                    p_v2[i] = 0.0;
                    p_v3[i] = 0.0;
                } else if (vcase == 5) {
                    p_v1[i] = 1.0;
                    p_v2[i] = 1.0;
                    p_v3[i] = 1.0;
                }
            }  // i1
        }  // i2
    }  // i3

    // ierr = v->RestoreArrays(p_v1, p_v2, p_v3); CHKERRQ(ierr);
    ierr = VecRestoreArray(v->m_X1, &p_v1); CHKERRQ(ierr);
    ierr = VecRestoreArray(v->m_X2, &p_v2); CHKERRQ(ierr);
    ierr = VecRestoreArray(v->m_X3, &p_v3); CHKERRQ(ierr);


    opt->Exit(__func__);

    PetscFunctionReturn(ierr);
}

namespace reg {
namespace UnitTest {
  
PetscErrorCode TestTrajectory(RegOpt *m_Opt) {
  PetscErrorCode ierr = 0;
  PetscFunctionBegin;
  
  std::cout << "starting trajectory solver unit test" << std::endl;
  reg::VecField* v = nullptr;
  reg::VecField* t = nullptr;
#ifdef REG_HAS_CUDA
  reg::SemiLagrangianGPUNew *sl = nullptr;
#else
  reg::SemiLagrangian *sl = nullptr;
#endif
  ScalarType *X1 = nullptr;
  ScalarType *X2 = nullptr;
  ScalarType *X3 = nullptr;
  ScalarType *X = nullptr;
  
  ierr = AllocateOnce(v, m_Opt); CHKERRQ(ierr);
  ierr = AllocateOnce(t, m_Opt); CHKERRQ(ierr);
  ierr = AllocateOnce(sl, m_Opt); CHKERRQ(ierr);
  ierr = AllocateArray(X1, m_Opt->m_Domain.nl); CHKERRQ(ierr);
  ierr = AllocateArray(X2, m_Opt->m_Domain.nl); CHKERRQ(ierr);
  ierr = AllocateArray(X3, m_Opt->m_Domain.nl); CHKERRQ(ierr);
  ierr = AllocateArray(X, m_Opt->m_Domain.nl*3); CHKERRQ(ierr);
  
  ierr = ComputeSyntheticData(v, m_Opt); CHKERRQ(ierr);
  
  ierr = sl->SetWorkVecField(t); CHKERRQ(ierr);
  
  for (IntType i1 = 0; i1 < m_Opt->m_Domain.isize[0]; ++i1) {  // x1
    for (IntType i2 = 0; i2 < m_Opt->m_Domain.isize[1]; ++i2) {  // x2
      for (IntType i3 = 0; i3 < m_Opt->m_Domain.isize[2]; ++i3) {  // x3
        ScalarType x1, x2, x3;
        IntType l = GetLinearIndex(i1, i2, i3, m_Opt->m_Domain.isize);
        X[l*3+0] = m_Opt->m_Domain.hx[0]*static_cast<ScalarType>(i1 + m_Opt->m_Domain.istart[0]);
        X[l*3+1] = m_Opt->m_Domain.hx[1]*static_cast<ScalarType>(i2 + m_Opt->m_Domain.istart[1]);
        X[l*3+2] = m_Opt->m_Domain.hx[2]*static_cast<ScalarType>(i3 + m_Opt->m_Domain.istart[2]);
      }  // i1
    }  // i2
  }  // i3
  
#ifdef REG_HAS_CUDA
  ierr = sl->SetInitialTrajectory(X); CHKERRQ(ierr);
  ierr = sl->ComputeTrajectory(v, "state"); CHKERRQ(ierr);
  ierr = sl->Interpolate(t, v, "state"); CHKERRQ(ierr);
  ierr = v->Copy(t); CHKERRQ(ierr);
  ierr = sl->GetQueryPoints(X);
  ierr = v->Scale(-1.); CHKERRQ(ierr);
  ierr = sl->SetInitialTrajectory(X); CHKERRQ(ierr);
  ierr = sl->ComputeTrajectory(v, "state"); CHKERRQ(ierr);
#else
  ierr = sl->ComputeTrajectory(v, "state", X); CHKERRQ(ierr);
  ierr = sl->Interpolate(t, v, "state"); CHKERRQ(ierr);
  ierr = v->Copy(t); CHKERRQ(ierr);
  ierr = sl->GetQueryPoints(X);
  ierr = v->Scale(-1.); CHKERRQ(ierr);
  ierr = sl->ComputeTrajectory(v, "state", X); CHKERRQ(ierr);
#endif
  
  ierr = sl->GetQueryPoints(X1, X2, X3);
  double ex = 0.0;
  double ey = 0.0;
  double ez = 0.0;
  for (IntType i1 = 0; i1 < m_Opt->m_Domain.isize[0]; ++i1) {  // x1
    for (IntType i2 = 0; i2 < m_Opt->m_Domain.isize[1]; ++i2) {  // x2
      for (IntType i3 = 0; i3 < m_Opt->m_Domain.isize[2]; ++i3) {  // x3
        // compute coordinates (nodal grid)
        ScalarType x1, x2, x3;
        x1 = m_Opt->m_Domain.hx[0]*static_cast<ScalarType>(i1 + m_Opt->m_Domain.istart[0]);
        x2 = m_Opt->m_Domain.hx[1]*static_cast<ScalarType>(i2 + m_Opt->m_Domain.istart[1]);
        x3 = m_Opt->m_Domain.hx[2]*static_cast<ScalarType>(i3 + m_Opt->m_Domain.istart[2]);

        // compute linear / flat index
        IntType l = GetLinearIndex(i1, i2, i3, m_Opt->m_Domain.isize);

        ex = std::max(ex,std::abs(static_cast<double>(X1[l] - x1)));
        ey = std::max(ey,std::abs(static_cast<double>(X2[l] - x2)));
        ez = std::max(ez,std::abs(static_cast<double>(X3[l] - x3)));

      }  // i1
    }  // i2
  }  // i3
  
  std::cout << "Inf-norm on trajectory: " << std::max(ex,std::max(ey,ez)) << std::endl;
  
  
  ierr = Free(sl); CHKERRQ(ierr);
  ierr = Free(t); CHKERRQ(ierr);
  ierr = Free(v); CHKERRQ(ierr);
  ierr = FreeArray(X1); CHKERRQ(ierr);
  ierr = FreeArray(X2); CHKERRQ(ierr);
  ierr = FreeArray(X3); CHKERRQ(ierr);
  ierr = FreeArray(X); CHKERRQ(ierr);
    
  PetscFunctionReturn(ierr);
}
  
PetscErrorCode TestForwardSolver(RegOpt *m_Opt) {
  PetscErrorCode ierr = 0;
  PetscFunctionBegin;
  
  std::cout << "starting forward solver unit test" << std::endl;
  
  reg::ScaField *m0 = NULL, *m0true = NULL;
  reg::VecField* v = NULL;
  reg::VecField* t = NULL;
  reg::VecField* t2 = NULL;
  reg::TransportProblem* solver = NULL;
  ScalarType val, val0, relval;

  // make sure we do not store time history
  m_Opt->m_RegFlags.runinversion = false;
  
  ierr = AllocateOnce(v, m_Opt);
  ierr = AllocateOnce(t, m_Opt);
  ierr = AllocateOnce(t2, m_Opt);
  ierr = AllocateOnce(m0, m_Opt);
  ierr = AllocateOnce(m0true, m_Opt);

  ierr = ComputeSyntheticData(*m0true, m_Opt); CHKERRQ(ierr);
  ierr = m0->Copy(m0true); CHKERRQ(ierr);
  ierr = ComputeSyntheticData(v, m_Opt); CHKERRQ(ierr);

  ierr = AllocateOnce<TransportEquationSL>(solver, m_Opt); CHKERRQ(ierr);
  
  ierr = solver->SetControlVariable(v); CHKERRQ(ierr);
  ierr = solver->SetStateVariable(m0); CHKERRQ(ierr);
  ierr = solver->SetWorkVecField(t, 1); CHKERRQ(ierr);
  ierr = solver->SetWorkVecField(t2, 2); CHKERRQ(ierr);

  ierr = reg::DbgMsg("computing error for forward solver"); CHKERRQ(ierr);

  ierr = solver->SolveForwardProblem(); CHKERRQ(ierr);
  ierr = solver->SolveInverseProblem(); CHKERRQ(ierr);

  ierr = VecAXPY(*m0, -1.0, *m0true); CHKERRQ(ierr);
  ierr = VecNorm(*m0, NORM_2, &val); CHKERRQ(ierr);
  ierr = VecNorm(*m0true, NORM_2, &val0); CHKERRQ(ierr);
  
  relval = val;
  relval /= (val0 > 0.0 ? val0 : 1.0);

  std::cout << "numerical error: "<< std::scientific << relval
         << " (absolute " << val << ",ref: " << val0 << ")" << std::endl;

  ierr = Free(v); CHKERRQ(ierr);
  ierr = Free(t); CHKERRQ(ierr);
  ierr = Free(t2); CHKERRQ(ierr);
  ierr = Free(m0); CHKERRQ(ierr);
  ierr = Free(m0true); CHKERRQ(ierr);
  ierr = Free(solver); CHKERRQ(ierr);
  
  PetscFunctionReturn(ierr);
}

PetscErrorCode TestGradient(RegOpt *m_Opt) {
  PetscErrorCode ierr = 0;
  PetscFunctionBegin;
  
  std::cout << "starting gradient solver unit test" << std::endl;
  
  Vec m = 0;
  reg::VecField* v = NULL;
  reg::CLAIRE* registration = NULL;

  ierr = AllocateOnce(registration, m_Opt); CHKERRQ(ierr);
  ierr = ComputeSyntheticData(m, m_Opt); CHKERRQ(ierr);
  ierr = ComputeSyntheticData(v, m_Opt); CHKERRQ(ierr);

  ierr = registration->SetControlVariable(v); CHKERRQ(ierr);
  ierr = registration->InitializeSolver(); CHKERRQ(ierr);
  ierr = registration->SetReferenceImage(m); CHKERRQ(ierr);
  ierr = registration->SolveForwardProblem(NULL, m); CHKERRQ(ierr);
  ierr = registration->EvaluateGradient(NULL, NULL); CHKERRQ(ierr);

  ierr = registration->DerivativeCheckGradient(); CHKERRQ(ierr);

  if (m != NULL) {ierr = VecDestroy(&m); CHKERRQ(ierr); m = NULL;}
  if (v != NULL) {delete v; v = NULL;}
  
  ierr = Free(registration); CHKERRQ(ierr);
  
  PetscFunctionReturn(ierr);
}


PetscErrorCode TestHessian(RegOpt *m_Opt) {
  PetscErrorCode ierr = 0;
  PetscFunctionBegin;
  
  std::cout << "starting gradient solver unit test" << std::endl;
  
  Vec m = 0;
  reg::VecField* v = NULL;
  reg::CLAIRE* registration = NULL;

  ierr = AllocateOnce(registration, m_Opt); CHKERRQ(ierr);
  ierr = ComputeSyntheticData(m, m_Opt); CHKERRQ(ierr);
  ierr = ComputeSyntheticData(v, m_Opt); CHKERRQ(ierr);

  ierr = registration->SetControlVariable(v); CHKERRQ(ierr);
  ierr = registration->InitializeSolver(); CHKERRQ(ierr);
  ierr = registration->SetReferenceImage(m); CHKERRQ(ierr);
  ierr = registration->SolveForwardProblem(NULL, m); CHKERRQ(ierr);
  ierr = registration->EvaluateGradient(NULL, NULL); CHKERRQ(ierr);
  ierr = registration->HessianMatVec(NULL, NULL); CHKERRQ(ierr);

  ierr = registration->DerivativeCheckHessian(); CHKERRQ(ierr);

  if (m != NULL) {ierr = VecDestroy(&m); CHKERRQ(ierr); m = NULL;}
  if (v != NULL) {delete v; v = NULL;}
  
  ierr = Free(registration); CHKERRQ(ierr);
  
  PetscFunctionReturn(ierr);
}

}} // namespace reg

#endif // _TESTINTERPOLATION_CPP_

