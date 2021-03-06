/*************************************************************************
 *  Copyright (c) 2016.
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

#ifndef _VECFIELD_CPP_
#define _VECFIELD_CPP_

#include "VecField.hpp"




namespace reg {




/********************************************************************
 * @brief default constructor
 *******************************************************************/
VecField::VecField() {
    this->Initialize();
}




/********************************************************************
 * @brief default destructor
 *******************************************************************/
VecField::~VecField() {
    this->ClearMemory();
}




/********************************************************************
 * @brief constructor
 *******************************************************************/
VecField::VecField(RegOpt* opt) {
    this->Initialize();
    this->SetOpt(opt);
    this->Allocate();
}




/********************************************************************
 * @brief constructor
 *******************************************************************/
VecField::VecField(RegOpt* opt, int level) {
    this->Initialize();
    this->SetOpt(opt);
    this->Allocate(level);
}




/********************************************************************
 * @brief constructor
 *******************************************************************/
VecField::VecField(IntType nl, IntType ng) {
    this->Initialize();
    this->Allocate(nl, ng);
}




/********************************************************************
 * @brief init variables
 *******************************************************************/
PetscErrorCode VecField::Initialize(void) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    this->m_Opt = NULL;

    this->m_X1 = NULL;
    this->m_X2 = NULL;
    this->m_X3 = NULL;

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief clean up
 *******************************************************************/
PetscErrorCode VecField::ClearMemory() {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    if (this->m_X1 != NULL) {
        ierr = VecDestroy(&this->m_X1); CHKERRQ(ierr);
        this->m_X1 = NULL;
    }
    if (this->m_X2 != NULL) {
        ierr = VecDestroy(&this->m_X2); CHKERRQ(ierr);
        this->m_X2 = NULL;
    }
    if (this->m_X3 != NULL) {
        ierr = VecDestroy(&this->m_X3); CHKERRQ(ierr);
        this->m_X3 = NULL;
    }

    PetscFunctionReturn(0);
}




/********************************************************************
 * @brief set the options
 *******************************************************************/
PetscErrorCode VecField::SetOpt(RegOpt* opt) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    ierr = Assert(opt != NULL, "null pointer"); CHKERRQ(ierr);
    this->m_Opt = opt;

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief function to allocate vector field
 *******************************************************************/
PetscErrorCode VecField::GetSize(IntType& nl, IntType& ng) {
    PetscErrorCode ierr = 0;
    std::stringstream ss;
    PetscFunctionBegin;

    //  get sizes
    ierr = VecGetSize(this->m_X1, &ng); CHKERRQ(ierr);
    ierr = VecGetLocalSize(this->m_X1, &nl); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief function to allocate vector field
 *******************************************************************/
PetscErrorCode VecField::Allocate() {
    PetscErrorCode ierr = 0;
    IntType nl, ng;
    PetscFunctionBegin;

    // make sure, that all pointers are deallocated
    ierr = this->ClearMemory(); CHKERRQ(ierr);

    nl = this->m_Opt->m_Domain.nl;
    ng = this->m_Opt->m_Domain.ng;

    ierr = this->Allocate(nl, ng); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief function to allocate vector field
 *******************************************************************/
PetscErrorCode VecField::Allocate(int level) {
    PetscErrorCode ierr = 0;
    IntType nl, ng;
    PetscFunctionBegin;

    // make sure, that all pointers are deallocated
    ierr = this->ClearMemory(); CHKERRQ(ierr);

    nl = this->m_Opt->m_GridCont.nl[level];
    ng = this->m_Opt->m_GridCont.ng[level];

    ierr = this->Allocate(nl, ng); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}





/********************************************************************
 * @brief function to allocate vector field
 *******************************************************************/
PetscErrorCode VecField::Allocate(IntType nl, IntType ng) {
    PetscErrorCode ierr = 0;
    std::stringstream ss;
    PetscFunctionBegin;

    // make sure, that all pointers are deallocated
    ierr = this->ClearMemory(); CHKERRQ(ierr);

    // allocate vector field
    ierr = VecCreate(PETSC_COMM_WORLD, &this->m_X1); CHKERRQ(ierr);
    ierr = VecSetSizes(this->m_X1, nl, ng); CHKERRQ(ierr);
    #ifdef REG_HAS_CUDA
        ierr = VecSetType(this->m_X1, VECCUDA); CHKERRQ(ierr);
    #else
        ierr = VecSetFromOptions(this->m_X1); CHKERRQ(ierr);
    #endif


    // allocate vector field
    ierr = VecCreate(PETSC_COMM_WORLD, &this->m_X2); CHKERRQ(ierr);
    ierr = VecSetSizes(this->m_X2, nl, ng); CHKERRQ(ierr);
    #ifdef REG_HAS_CUDA
        ierr = VecSetType(this->m_X2, VECCUDA); CHKERRQ(ierr);
    #else
        ierr = VecSetFromOptions(this->m_X2); CHKERRQ(ierr);
    #endif

    // allocate vector field
    ierr = VecCreate(PETSC_COMM_WORLD, &this->m_X3); CHKERRQ(ierr);
    ierr = VecSetSizes(this->m_X3, nl, ng); CHKERRQ(ierr);
    #ifdef REG_HAS_CUDA
        ierr = VecSetType(this->m_X3, VECCUDA); CHKERRQ(ierr);
    #else
        ierr = VecSetFromOptions(this->m_X3); CHKERRQ(ierr);
    #endif

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief copy input vector field
 *******************************************************************/
PetscErrorCode VecField::Copy(VecField* v) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    ierr = VecCopy(v->m_X1, this->m_X1); CHKERRQ(ierr);
    ierr = VecCopy(v->m_X2, this->m_X2); CHKERRQ(ierr);
    ierr = VecCopy(v->m_X3, this->m_X3); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief set value
 *******************************************************************/
PetscErrorCode VecField::SetValue(ScalarType value) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    ierr = VecSet(this->m_X1, value); CHKERRQ(ierr);
    ierr = VecSet(this->m_X2, value); CHKERRQ(ierr);
    ierr = VecSet(this->m_X3, value); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief get arrays of vector field
 *******************************************************************/
PetscErrorCode VecField::GetArrays(ScalarType*& p_x1,
                                   ScalarType*& p_x2,
                                   ScalarType*& p_x3) {
    PetscErrorCode ierr = 0;

    ierr = GetRawPointer(this->m_X1, &p_x1); CHKERRQ(ierr);
    ierr = GetRawPointer(this->m_X2, &p_x2); CHKERRQ(ierr);
    ierr = GetRawPointer(this->m_X3, &p_x3); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief get arrays of vector field
 *******************************************************************/
PetscErrorCode VecField::GetArraysRead(const ScalarType*& p_x1,
                                       const ScalarType*& p_x2,
                                       const ScalarType*& p_x3) {
    PetscErrorCode ierr = 0;

    ierr = GetRawPointerRead(this->m_X1, &p_x1); CHKERRQ(ierr);
    ierr = GetRawPointerRead(this->m_X2, &p_x2); CHKERRQ(ierr);
    ierr = GetRawPointerRead(this->m_X3, &p_x3); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief pointwise scale of a vector field
 *******************************************************************/
PetscErrorCode VecField::RestoreArrays(ScalarType*& p_x1,
                                       ScalarType*& p_x2,
                                       ScalarType*& p_x3) {
    PetscErrorCode ierr = 0;

    ierr = RestoreRawPointer(this->m_X1, &p_x1); CHKERRQ(ierr);
    ierr = RestoreRawPointer(this->m_X2, &p_x2); CHKERRQ(ierr);
    ierr = RestoreRawPointer(this->m_X3, &p_x3); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief restore arrays of vector field
 *******************************************************************/
PetscErrorCode VecField::RestoreArraysRead(const ScalarType*& p_x1,
                                           const ScalarType*& p_x2,
                                           const ScalarType*& p_x3) {
    PetscErrorCode ierr = 0;

    ierr = RestoreRawPointerRead(this->m_X1, &p_x1); CHKERRQ(ierr);
    ierr = RestoreRawPointerRead(this->m_X2, &p_x2); CHKERRQ(ierr);
    ierr = RestoreRawPointerRead(this->m_X3, &p_x3); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}

/********************************************************************
 * @brief get arrays of vector field for read/write
 *******************************************************************/
PetscErrorCode VecField::GetArraysReadWrite(ScalarType*& p_x1,
                                            ScalarType*& p_x2,
                                            ScalarType*& p_x3) {
    PetscErrorCode ierr = 0;

    ierr = GetRawPointerReadWrite(this->m_X1, &p_x1); CHKERRQ(ierr);
    ierr = GetRawPointerReadWrite(this->m_X2, &p_x2); CHKERRQ(ierr);
    ierr = GetRawPointerReadWrite(this->m_X3, &p_x3); CHKERRQ(ierr);
    
    PetscFunctionReturn(ierr);
}


/********************************************************************
 * @brief resotre arrays of vector field for read/write
 *******************************************************************/
PetscErrorCode VecField::RestoreArraysReadWrite(ScalarType*& p_x1,
                                            ScalarType*& p_x2,
                                            ScalarType*& p_x3) {
    PetscErrorCode ierr = 0;

    ierr = RestoreRawPointerReadWrite(this->m_X1, &p_x1); CHKERRQ(ierr);
    ierr = RestoreRawPointerReadWrite(this->m_X2, &p_x2); CHKERRQ(ierr);
    ierr = RestoreRawPointerReadWrite(this->m_X3, &p_x3); CHKERRQ(ierr);
    
    PetscFunctionReturn(ierr);
}


/********************************************************************
 * @brief sets the individual components of a vector field;
 * the input is a flat petsc array
 *******************************************************************/
PetscErrorCode VecField::SetComponents(Vec w) {
    PetscErrorCode ierr = 0;
    IntType nl, n;
    ScalarType *p_x1 = NULL, *p_x2 = NULL, *p_x3 = NULL;
    const ScalarType *p_w = NULL;

    PetscFunctionBegin;

    // get local size of vector field
    ierr = VecGetLocalSize(w, &n); CHKERRQ(ierr);

    ierr = Assert(this->m_X1 != NULL, "null pointer"); CHKERRQ(ierr);
    ierr = Assert(this->m_X2 != NULL, "null pointer"); CHKERRQ(ierr);
    ierr = Assert(this->m_X3 != NULL, "null pointer"); CHKERRQ(ierr);

    ierr = GetRawPointerRead(w, &p_w); CHKERRQ(ierr);
    ierr = this->GetArrays(p_x1, p_x2, p_x3); CHKERRQ(ierr);

    // compute size of each individual component
    nl = n / 3;
//    ierr = Assert(nl==this->m_Opt->m_Domain.nl,"dimension mismatch"); CHKERRQ(ierr);

#pragma omp parallel
{
#pragma omp for
    for (IntType i = 0; i < nl; ++i) {
        p_x1[i] = p_w[i     ];
        p_x2[i] = p_w[i+  nl];
        p_x3[i] = p_w[i+2*nl];
    }
}  // pragma omp parallel

    ierr = RestoreRawPointerRead(w, &p_w); CHKERRQ(ierr);
    ierr = this->RestoreArrays(p_x1, p_x2, p_x3); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief get components of vector field and store them
 * in a flat vector
 *******************************************************************/
PetscErrorCode VecField::GetComponents(Vec w) {
    PetscErrorCode ierr = 0;
    IntType nl, n;
    ScalarType *p_x1 = NULL, *p_x2 = NULL, *p_x3 = NULL, *p_w = NULL;

    PetscFunctionBegin;

    // get local size of vector field
    ierr = VecGetLocalSize(w, &n); CHKERRQ(ierr);

    ierr = GetRawPointer(w, &p_w); CHKERRQ(ierr);
    ierr = this->GetArrays(p_x1, p_x2, p_x3); CHKERRQ(ierr);

    // compute size of each individual component
    nl = n / 3;

#pragma omp parallel
{
#pragma omp for
    for (IntType i = 0; i < nl; ++i) {
        p_w[i     ] = p_x1[i];
        p_w[i+  nl] = p_x2[i];
        p_w[i+2*nl] = p_x3[i];
    }
}  // pragma omp parallel

    ierr = this->RestoreArrays(p_x1, p_x2, p_x3); CHKERRQ(ierr);
    ierr = RestoreRawPointer(w, &p_w); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief scale vector by scalar value
 *******************************************************************/
PetscErrorCode VecField::Scale(ScalarType value) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    ierr = VecScale(this->m_X1, value); CHKERRQ(ierr);
    ierr = VecScale(this->m_X2, value); CHKERRQ(ierr);
    ierr = VecScale(this->m_X3, value); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief pointwise scale of vector field
 *******************************************************************/
PetscErrorCode VecField::Scale(Vec s) {
    PetscErrorCode ierr = 0;
    IntType nl;
    ScalarType *p_v1 = NULL, *p_v2 = NULL, *p_v3 = NULL, *p_s = NULL;

    PetscFunctionBegin;

    // get local size of vector field
    ierr = VecGetLocalSize(s, &nl); CHKERRQ(ierr);

    // get pointers
    ierr = GetRawPointer(s, &p_s); CHKERRQ(ierr);
    ierr = this->GetArrays(p_v1, p_v2, p_v3); CHKERRQ(ierr);

#pragma omp parallel
{
    ScalarType scale;
#pragma omp for
    for (IntType i = 0; i < nl; ++i) {
        scale = p_s[i];
        p_v1[i] = scale*p_v1[i];
        p_v2[i] = scale*p_v2[i];
        p_v3[i] = scale*p_v3[i];
    }
}  // pragma omp parallel

    // get pointers
    ierr = this->RestoreArrays(p_v1, p_v2, p_v3); CHKERRQ(ierr);
    ierr = RestoreRawPointer(s, &p_s); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief pointwise scale of a vector field
 *******************************************************************/
PetscErrorCode VecField::Scale(VecField* v, Vec s) {
    PetscErrorCode ierr = 0;
    IntType nl;
    ScalarType *p_v1 = NULL, *p_v2 = NULL, *p_v3 = NULL, *p_s = NULL,
                *p_sv1 = NULL, *p_sv2 = NULL, *p_sv3 = NULL;

    PetscFunctionBegin;

    // get local size of vector field
    ierr = VecGetLocalSize(s, &nl); CHKERRQ(ierr);

    // get pointers
    ierr = GetRawPointer(s, &p_s); CHKERRQ(ierr);
    ierr = this->GetArrays(p_v1, p_v2, p_v3); CHKERRQ(ierr);
    ierr = v->GetArrays(p_sv1, p_sv2, p_sv3); CHKERRQ(ierr);

#pragma omp parallel
{
    ScalarType scale;
#pragma omp for
    for (IntType i = 0; i < nl; ++i) {
        scale = p_s[i];
        p_sv1[i] = scale*p_v1[i];
        p_sv2[i] = scale*p_v2[i];
        p_sv3[i] = scale*p_v3[i];
    }
}  // pragma omp parallel

    // get pointers
    ierr = RestoreRawPointer(s, &p_s); CHKERRQ(ierr);
    ierr = this->RestoreArrays(p_v1, p_v2, p_v3); CHKERRQ(ierr);
    ierr = v->RestoreArrays(p_sv1, p_sv2, p_sv3); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief interface for AXPY
 *******************************************************************/
PetscErrorCode VecField::AXPY(ScalarType s, VecField* v) {
    PetscErrorCode ierr = 0;

    PetscFunctionBegin;

    ierr = VecAXPY(this->m_X1, s, v->m_X1); CHKERRQ(ierr);
    ierr = VecAXPY(this->m_X2, s, v->m_X2); CHKERRQ(ierr);
    ierr = VecAXPY(this->m_X3, s, v->m_X3); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief interface for WAXPY
 *******************************************************************/
PetscErrorCode VecField::WAXPY(ScalarType s, VecField* v, VecField* w) {
    PetscErrorCode ierr = 0;

    PetscFunctionBegin;

    ierr = VecWAXPY(this->m_X1, s, v->m_X1, w->m_X1); CHKERRQ(ierr);
    ierr = VecWAXPY(this->m_X2, s, v->m_X2, w->m_X2); CHKERRQ(ierr);
    ierr = VecWAXPY(this->m_X3, s, v->m_X3, w->m_X3); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}



/********************************************************************
 * @brief compute pointwise norm of vector field
 *******************************************************************/
PetscErrorCode VecField::Norm(Vec xnorm) {
    PetscErrorCode ierr = 0;
    IntType i, nl;
    ScalarType *p_x1 = NULL, *p_x2 = NULL, *p_x3 = NULL, *p_x = NULL;

    PetscFunctionBegin;

    // get local size of vector field
    ierr = VecGetLocalSize(xnorm, &nl); CHKERRQ(ierr);

    ierr = this->GetArrays(p_x1, p_x2, p_x3); CHKERRQ(ierr);
    ierr = GetRawPointer(xnorm, &p_x); CHKERRQ(ierr);

#pragma omp parallel
{
#pragma omp for
    for (i = 0; i < nl; ++i) {
        p_x[i] = PetscSqrtReal(p_x1[i]*p_x1[i]
                             + p_x2[i]*p_x2[i]
                             + p_x3[i]*p_x3[i]);
    }
}  // pragma omp parallel

    ierr = RestoreRawPointer(xnorm, &p_x); CHKERRQ(ierr);
    ierr = this->RestoreArrays(p_x1, p_x2, p_x3); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief compute pointwise norm of vector field
 *******************************************************************/
PetscErrorCode VecField::Norm(ScalarType& value) {
    PetscErrorCode ierr = 0;
    IntType nl;
    ScalarType vnorm;
    int rval;
    ScalarType *p_x1 = NULL, *p_x2 = NULL, *p_x3 = NULL;

    PetscFunctionBegin;

    // get local size of vector field
    ierr = VecGetLocalSize(this->m_X1, &nl); CHKERRQ(ierr);

    vnorm = 0.0;
    ierr = this->GetArrays(p_x1, p_x2, p_x3); CHKERRQ(ierr);
    for (IntType i = 0; i < nl; ++i) {
        vnorm += p_x1[i]*p_x1[i] + p_x2[i]*p_x2[i] + p_x3[i]*p_x3[i];
    }
    ierr = this->RestoreArrays(p_x1, p_x2, p_x3); CHKERRQ(ierr);

    rval = MPI_Allreduce(&vnorm, &value, 1, MPIU_REAL, MPI_SUM, PETSC_COMM_WORLD);
    ierr = Assert(rval == MPI_SUCCESS, "mpi reduce returned error"); CHKERRQ(ierr);

    value = PetscSqrtReal(value);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief compute norm of individual components of vector field
 *******************************************************************/
PetscErrorCode VecField::Norm(ScalarType& nvx1, ScalarType& nvx2, ScalarType& nvx3) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;

    ierr = VecNorm(this->m_X1, NORM_2, &nvx1); CHKERRQ(ierr);
    ierr = VecNorm(this->m_X2, NORM_2, &nvx2); CHKERRQ(ierr);
    ierr = VecNorm(this->m_X3, NORM_2, &nvx3); CHKERRQ(ierr);

    PetscFunctionReturn(ierr);
}


/********************************************************************
 * @brief print debug info
 *******************************************************************/
PetscErrorCode VecField::DebugInfo(std::string str, int line, const char *file) {
  PetscErrorCode ierr = 0;
  PetscFunctionBegin;
  
  if (this->m_Opt->m_Verbosity > 2) {
      ScalarType *p_x1, *p_x2, *p_x3;
      IntType nl;
      size_t fingerprint = 0;
      ierr = VecGetArray(this->m_X1, &p_x1); CHKERRQ(ierr);
      ierr = VecGetArray(this->m_X2, &p_x2); CHKERRQ(ierr);
      ierr = VecGetArray(this->m_X3, &p_x3); CHKERRQ(ierr);
      // compute size of each individual component
      nl = this->m_Opt->m_Domain.nl;
  #pragma omp parallel for
      for (IntType i = 0; i < nl; ++i) {
#if defined(PETSC_USE_REAL_SINGLE)
        fingerprint += reinterpret_cast<uint32_t*>(p_x1)[i];
        fingerprint += reinterpret_cast<uint32_t*>(p_x2)[i];
        fingerprint += reinterpret_cast<uint32_t*>(p_x2)[i];
#else
        fingerprint += reinterpret_cast<uint64_t*>(p_x1)[i];
        fingerprint += reinterpret_cast<uint64_t*>(p_x2)[i];
        fingerprint += reinterpret_cast<uint64_t*>(p_x2)[i];
#endif
      }
      ierr = VecRestoreArray(this->m_X1, &p_x1); CHKERRQ(ierr);
      ierr = VecRestoreArray(this->m_X2, &p_x2); CHKERRQ(ierr);
      ierr = VecRestoreArray(this->m_X3, &p_x3); CHKERRQ(ierr);
    
      ScalarType maxval, minval, nvx1, nvx2, nvx3;
      std::stringstream ss;
      
      ss  << str << " hash: " << std::hex << fingerprint;
      ierr = DbgMsg(ss.str()); CHKERRQ(ierr);
      ss.str(std::string()); ss.clear();

      ierr = this->Norm(nvx1, nvx2, nvx3); CHKERRQ(ierr);
      ss  << str << " norm: (" << std::scientific
          << nvx1 << "," << nvx2 << "," << nvx3 <<")";
      ierr = DbgMsg(ss.str()); CHKERRQ(ierr);
      ss.str(std::string()); ss.clear();
      
      ierr = VecMax(this->m_X1, NULL, &maxval); CHKERRQ(ierr);
      ierr = VecMin(this->m_X1, NULL, &minval); CHKERRQ(ierr);
      ss  << str << " min/max: [" << std::scientific
          << minval << "," << maxval << "]";
      ierr = DbgMsg(ss.str()); CHKERRQ(ierr);
      ss.str(std::string()); ss.clear();
      ierr = VecMax(this->m_X2, NULL, &maxval); CHKERRQ(ierr);
      ierr = VecMin(this->m_X2, NULL, &minval); CHKERRQ(ierr);
      ss  << std::string(str.size(),' ') << "          [" << std::scientific
          << minval << "," << maxval << "] ";
      ierr = DbgMsg(ss.str()); CHKERRQ(ierr);
      ss.str(std::string()); ss.clear();
      ierr = VecMax(this->m_X3, NULL, &maxval); CHKERRQ(ierr);
      ierr = VecMin(this->m_X3, NULL, &minval); CHKERRQ(ierr);
      ss  << std::string(str.size(), ' ') << "          [" << std::scientific
          << minval << "," << maxval << "]";
      ierr = DbgMsg(ss.str()); CHKERRQ(ierr);
      ss.str(std::string()); ss.clear();
  }
  
  PetscFunctionReturn(ierr);
}



}  // namespace reg




#endif  // _VECFIELD_CPP_
