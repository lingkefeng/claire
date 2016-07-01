/*
 *  Copyright (c) 2015-2016.
 *  All rights reserved.
 *  This file is part of the XXX library.
 *
 *  XXX is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  XXX is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XXX.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _KRYLOVINTERFACEREG_H_
#define _KRYLOVINTERFACEREG_H_

#include "RegOpt.hpp"
#include "RegUtils.hpp"

namespace reg
{

// mat vec for two level preconditioner
PetscErrorCode TwoLevelPCMatVec(Mat,Vec,Vec);
PetscErrorCode PrecondMonitor(KSP,IntType,ScalarType,void*);

PetscErrorCode KrylovMonitor(KSP,PetscInt,PetscReal,void*);
PetscErrorCode DispKSPConvReason(KSPConvergedReason);



} // end of name space

#endif // _KRYLOVINTERFACEREG_H_
