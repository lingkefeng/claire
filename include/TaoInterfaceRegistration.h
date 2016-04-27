/**
 *  Description: prototype interface for tao optimizer for registration
 *  Copyright (c) 2015-2016.
 *  All rights reserved.
 *  This file is part of PGLISTR library.
 *
 *  PGLISTR is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  PGLISTR is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with PGLISTR.  If not, see <http://www.gnu.org/licenses/>.
 *
*/

#ifndef _TAOINTERFACEREGISTRATION_H_
#define _TAOINTERFACEREGISTRATION_H_

#include "RegOpt.h"
#include "RegUtils.h"

namespace reg
{

// the following functions interface the tao solver
PetscErrorCode EvaluateObjective(Tao,Vec,ScalarType*,void*);
PetscErrorCode EvaluateGradient(Tao,Vec,Vec,void*);
PetscErrorCode EvaluateObjectiveGradient(Tao,Vec,ScalarType*,Vec,void*);

PetscErrorCode ConstructHessian(Tao,Vec,Mat*,Mat*,MatStructure*,void*);
PetscErrorCode EvaluateHessian(Tao,Vec,Mat,Mat,void*);
PetscErrorCode HessianMatVec(Mat,Vec,Vec);
PetscErrorCode PrecondMatVec(PC,Vec,Vec);

PetscErrorCode PrecondSetup(PC);

PetscErrorCode OptimizationMonitor(Tao,void*);
PetscErrorCode KrylovMonitor(KSP,PetscInt,PetscReal,void*);
PetscErrorCode DispKSPConvReason(KSPConvergedReason);
PetscErrorCode DispLSConvReason(TaoLineSearchConvergedReason);
PetscErrorCode DispTaoConvReason(TaoConvergedReason);



} // end of name space

#endif // _TAOINTERFACEREGISTRATION_H_