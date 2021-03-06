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
 *  along with CLAIRE.  If not, see <http://www.gnu.org/licenses/>.
 ************************************************************************/

#ifndef _DISTANCEMEASURE_CPP_
#define _DISTANCEMEASURE_CPP_

#include "DistanceMeasure.hpp"




namespace reg {




/********************************************************************
 * @brief default constructor
 *******************************************************************/
DistanceMeasure::DistanceMeasure() {
    this->Initialize();
}




/********************************************************************
 * @brief default destructor
 *******************************************************************/
DistanceMeasure::~DistanceMeasure() {
    this->ClearMemory();
}




/********************************************************************
 * @brief constructor
 *******************************************************************/
DistanceMeasure::DistanceMeasure(RegOpt* opt) {
    this->Initialize();
    this->m_Opt = opt;
}




/********************************************************************
 * @brief init variables
 *******************************************************************/
PetscErrorCode DistanceMeasure::Initialize() {
    PetscFunctionBegin;

    this->m_Opt = NULL;

    this->m_Mask = NULL;
    this->m_AuxVar1 = NULL;
    this->m_AuxVar2 = NULL;
    this->m_TemplateImage = NULL;
    this->m_ReferenceImage = NULL;
    this->m_StateVariable = NULL;
    this->m_AdjointVariable = NULL;
    this->m_IncStateVariable = NULL;
    this->m_IncAdjointVariable = NULL;
    this->m_WorkVecField1 = NULL;
    this->m_WorkVecField2 = NULL;
    this->m_WorkVecField3 = NULL;

    PetscFunctionReturn(0);
}




/********************************************************************
 * @brief clean up
 *******************************************************************/
PetscErrorCode DistanceMeasure::ClearMemory() {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    PetscFunctionReturn(ierr);
}





/********************************************************************
 * @brief set up scale
 *******************************************************************/
PetscErrorCode DistanceMeasure::SetupScale() {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    PetscFunctionReturn(ierr);
}





/********************************************************************
 * @brief set reference image (i.e., the fixed image)
 *******************************************************************/
PetscErrorCode DistanceMeasure::SetReferenceImage(Vec mR) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    this->m_Opt->Enter(__func__);

    ierr = Assert(mR != NULL, "null pointer"); CHKERRQ(ierr);
    this->m_ReferenceImage = mR;

    this->m_Opt->Exit(__func__);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief set reference image (i.e., the fixed image)
 *******************************************************************/
PetscErrorCode DistanceMeasure::SetTemplateImage(Vec mT) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    this->m_Opt->Enter(__func__);

    ierr = Assert(mT != NULL, "null pointer"); CHKERRQ(ierr);
    this->m_TemplateImage = mT;

    this->m_Opt->Exit(__func__);

    PetscFunctionReturn(ierr);
}



/********************************************************************
 * @brief set temporary vector fields 
 *******************************************************************/
PetscErrorCode DistanceMeasure::SetWorkVecField(VecField* v, int id) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    this->m_Opt->Enter(__func__);

    ierr = Assert(v != NULL, "null pointer"); CHKERRQ(ierr);
    switch (id) {
        case 1:
           this->m_WorkVecField1 = v;
           break;
        case 2:
           this->m_WorkVecField2 = v;
           break;
        case 3:
           this->m_WorkVecField3 = v;
           break;
	default:
           ierr = ThrowError("id not defined"); CHKERRQ(ierr);
           break;
  }

    this->m_Opt->Exit(__func__);

    PetscFunctionReturn(ierr);
}



/********************************************************************
 * @brief set mask
 *******************************************************************/
PetscErrorCode DistanceMeasure::SetMask(Vec mask) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    this->m_Opt->Enter(__func__);

    ierr = Assert(mask != NULL, "null pointer"); CHKERRQ(ierr);
    this->m_Mask = mask;

    this->m_Opt->Exit(__func__);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief set state variable
 *******************************************************************/
PetscErrorCode DistanceMeasure::SetStateVariable(Vec m) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    this->m_Opt->Enter(__func__);

    ierr = Assert(m != NULL, "null pointer"); CHKERRQ(ierr);
    this->m_StateVariable = m;

    this->m_Opt->Exit(__func__);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief set incremental state variable
 *******************************************************************/
PetscErrorCode DistanceMeasure::SetIncStateVariable(Vec mtilde) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    this->m_Opt->Enter(__func__);

    ierr = Assert(mtilde != NULL, "null pointer"); CHKERRQ(ierr);
    this->m_IncStateVariable = mtilde;

    this->m_Opt->Exit(__func__);

    PetscFunctionReturn(ierr);
}



/********************************************************************
 * @brief set adjoint variable
 *******************************************************************/
PetscErrorCode DistanceMeasure::SetAdjointVariable(Vec lambda) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    this->m_Opt->Enter(__func__);

    ierr = Assert(lambda != NULL, "null pointer"); CHKERRQ(ierr);
    this->m_AdjointVariable = lambda;

    this->m_Opt->Exit(__func__);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief set adjoint variable
 *******************************************************************/
PetscErrorCode DistanceMeasure::SetIncAdjointVariable(Vec lambdatilde) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    this->m_Opt->Enter(__func__);

    ierr = Assert(lambdatilde != NULL, "null pointer"); CHKERRQ(ierr);
    this->m_IncAdjointVariable = lambdatilde;

    this->m_Opt->Exit(__func__);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief set auxilary variable
 *******************************************************************/
PetscErrorCode DistanceMeasure::SetAuxVariable(Vec x, int id) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    this->m_Opt->Enter(__func__);

    ierr = Assert(x != NULL, "null pointer"); CHKERRQ(ierr);
    if (id == 1) {
        this->m_AuxVar1 = x;
    } else if (id == 2) {
        this->m_AuxVar2 = x;
    } else {
        ierr = ThrowError("id not defined"); CHKERRQ(ierr);
    }
    this->m_Opt->Exit(__func__);

    PetscFunctionReturn(ierr);
}



}  // namespace reg




#endif  // _DISTANCEMEASURE_CPP_
