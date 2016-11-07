/*************************************************************************
 *  Copyright (c) 2016.
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
 *  along with XXX. If not, see <http://www.gnu.org/licenses/>.
 ************************************************************************/

#ifndef _SYNPROBREGISTRATION_CPP_
#define _SYNPROBREGISTRATION_CPP_

#include "SynProbRegistration.hpp"




namespace reg {




/********************************************************************
 * @brief default constructor
 *******************************************************************/
#undef __FUNCT__
#define __FUNCT__ "SynProbRegistration"
SynProbRegistration::SynProbRegistration() {
    this->Initialize();
}




/********************************************************************
 * @brief default constructor
 *******************************************************************/
#undef __FUNCT__
#define __FUNCT__ "SynProbRegistration"
SynProbRegistration::SynProbRegistration(RegOpt* opt) {
    this->Initialize();
    this->m_Opt = opt;
}




/********************************************************************
 * @brief default destructor
 *******************************************************************/
#undef __FUNCT__
#define __FUNCT__ "~SynProbRegistration"
SynProbRegistration::~SynProbRegistration() {
    this->ClearMemory();
}




/********************************************************************
 * @brief initialize class variables
 *******************************************************************/
#undef __FUNCT__
#define __FUNCT__ "Initialize"
PetscErrorCode SynProbRegistration::Initialize() {
    this->m_Opt = NULL;
    PetscFunctionReturn(0);
}




/********************************************************************
 * @brief clear memory
 *******************************************************************/
#undef __FUNCT__
#define __FUNCT__ "ClearMemory"
PetscErrorCode SynProbRegistration::ClearMemory() {
    PetscFunctionReturn(0);
}




/********************************************************************
 * @brief clear memory
 *******************************************************************/
#undef __FUNCT__
#define __FUNCT__ "ComputeSmoothScalarField"
PetscErrorCode SynProbRegistration::ComputeSmoothScalarField(Vec m, int id) {
    PetscErrorCode ierr = 0;
    IntType isize[3], istart[3];
    ScalarType *p_m = NULL, value, hx[3];
    const ScalarType twopi = 2.0*PETSC_PI;
    const ScalarType sigma = 160.0;
    IntType nc, nl;
    PetscFunctionBegin;

    ierr = Assert(m != NULL, "null pointer"); CHKERRQ(ierr);

    for (int i = 0; i < 3; ++i) {
        hx[i] = this->m_Opt->GetDomainPara().hx[i];
        isize[i] = this->m_Opt->GetDomainPara().isize[i];
        istart[i] = this->m_Opt->GetDomainPara().istart[i];
    }

    nc = this->m_Opt->GetDomainPara().nc;
    nl = this->m_Opt->GetDomainPara().nlocal;

    ierr = VecGetArray(m, &p_m); CHKERRQ(ierr);
#pragma omp parallel
{
    IntType i1, i2, i3, i;
    ScalarType x1, x2, x3, s1, s2, s3, c, y;
#pragma omp for
    for (i1 = 0; i1 < isize[0]; ++i1) {  // x1
        for (i2 = 0; i2 < isize[1]; ++i2) {  // x2
            for (i3 = 0; i3 < isize[2]; ++i3) {  // x3
                // compute coordinates (nodal grid)
                x1 = hx[0]*static_cast<ScalarType>(i1 + istart[0]);
                x2 = hx[1]*static_cast<ScalarType>(i2 + istart[1]);
                x3 = hx[2]*static_cast<ScalarType>(i3 + istart[2]);

                // compute linear / flat index
                i = GetLinearIndex(i1, i2, i3, isize);

                if (id == 0) {
                    s1 = sin(x1);
                    s2 = sin(x2);
                    s3 = sin(x3);
                    value = (s1*s1 + s2*s2 + s3*s3)/3.0;
                } else if (id == 1) {
                   // first derivative of id 0 with respect to x1
                    value = 2.0*sin(x1)*cos(x1)/3.0;
                } else if (id == 2) {
                    // first derivative of id 0 with respect to x2
                    value = 2.0*sin(x2)*cos(x2)/3.0;
                } else if (id == 3) {
                    // first derivative of id 0 with respect to x3
                    value = 2.0*sin(x3)*cos(x3)/3.0;
                } else if (id == 4) {
                    // laplacian of id 0
                    s1 = 2.0*cos(x1)*cos(x1) - 2.0*sin(x1)*sin(x1);
                    s2 = 2.0*cos(x2)*cos(x2) - 2.0*sin(x2)*sin(x2);
                    s3 = 2.0*cos(x3)*cos(x3) - 2.0*sin(x3)*sin(x3);
                    value = (s1 + s2 + s3)/3.0;
                } else if (id == 5) {
                    // biharmonic of id 0
                    s1 = 8.0*sin(x1)*sin(x1) - 8.0*cos(x1)*cos(x1);
                    s2 = 8.0*sin(x2)*sin(x2) - 8.0*cos(x2)*cos(x2);
                    s3 = 8.0*sin(x3)*sin(x3) - 8.0*cos(x3)*cos(x3);
                    value = (s1 + s2 + s3)/3.0;
                } else if (id == 6) {
                    // rhs for poisson problem
                    c = sigma/(twopi*twopi);
                    s1 = (x1-PETSC_PI)*(x1-PETSC_PI);
                    s2 = (x2-PETSC_PI)*(x2-PETSC_PI);
                    s3 = (x3-PETSC_PI)*(x3-PETSC_PI);
                    y = - exp( -c * (s1 + s2 + s3) );
                    value = - ( -6.0*c + 4.0*c*c*(s1 + s2 + s3) )*y;
                } else if (id == 7) {

                    // analytic solution for rhs of poisson problem (id 6)
                    // rhs for poisson problem
                    c = sigma/(twopi*twopi);
                    s1 = (x1-PETSC_PI)*(x1-PETSC_PI);
                    s2 = (x2-PETSC_PI)*(x2-PETSC_PI);
                    s3 = (x3-PETSC_PI)*(x3-PETSC_PI);
                    y = -exp(-c * (s1 + s2 + s3) );
                    value = y + 1.0;
                } else if (id == 8) {
                    value  = 0.5 + 0.5*sin(x1)*sin(x2)*sin(x3);
                } else if (id == 9) {
                    value = sin(2.0*x1)*sin(2.0*x2)*sin(2.0*x3);
                    value *= value;
                } else {
                    value = 0.0;
                }

                p_m[i] = value;
            }  // i1
        }  // i2
    }  // i3
}  // pragma omp parallel

    // if the image has more than one component, just copy the
    // content of first image to all other
    if (nc != 1) {
        for (IntType k = 1; k < nc; ++k) {
            try {std::copy(p_m, p_m+nl, p_m+k*nl);}
            catch (std::exception&) {
                ierr = ThrowError("copy failed"); CHKERRQ(ierr);
            }
        }
    }

    ierr = VecRestoreArray(m, &p_m); CHKERRQ(ierr);

    PetscFunctionReturn(0);
}




/********************************************************************
 * @brief compute square
 *******************************************************************/
#undef __FUNCT__
#define __FUNCT__ "ComputeSquare"
PetscErrorCode SynProbRegistration::ComputeSquare(Vec m) {
    PetscErrorCode ierr = 0;
    IntType isize[3], istart[3];
    ScalarType *p_m = NULL, hx[3], x1, x2, x3;

    PetscFunctionBegin;

    for (int i = 0; i < 3; ++i) {
        hx[i] = this->m_Opt->GetDomainPara().hx[i];
        isize[i] = this->m_Opt->GetDomainPara().isize[i];
        istart[i] = this->m_Opt->GetDomainPara().istart[i];
    }

    ierr = VecGetArray(m,&p_m); CHKERRQ(ierr);

#pragma omp parallel
{
    IntType i, i1, i2, i3;
#pragma omp for
    for (i1 = 0; i1 < isize[0]; ++i1) {  // x1
        for (i2 = 0; i2 < isize[1]; ++i2) { // x2
            for (i3 = 0; i3 < isize[2]; ++i3) { // x3
                // compute coordinates (nodal grid)
                x1 = hx[0]*static_cast<ScalarType>(i1 + istart[0]);
                x2 = hx[1]*static_cast<ScalarType>(i2 + istart[1]);
                x3 = hx[2]*static_cast<ScalarType>(i3 + istart[2]);

                x1 -= PETSC_PI; x1 = fabs(x1);
                x2 -= PETSC_PI; x2 = fabs(x2);
                x3 -= PETSC_PI; x3 = fabs(x3);

                // compute linear / flat index
                i = GetLinearIndex(i1, i2, i3, isize);

                p_m[i] = (std::max(std::max(x1, x2), x3) < 2.0*PETSC_PI/4.0) ? 1.0 : 0.0;

            } // i1
        } // i2
    } // i3
} // pragma omp parallel

    ierr = VecRestoreArray(m, &p_m); CHKERRQ(ierr);

    PetscFunctionReturn(0);
}




/********************************************************************
 * @brief compute diamond
 *******************************************************************/
#undef __FUNCT__
#define __FUNCT__ "ComputeSphere"
PetscErrorCode SynProbRegistration::ComputeSphere(Vec m) {
    PetscErrorCode ierr = 0;
    IntType isize[3], istart[3];
    ScalarType *p_m = NULL, hx[3];

    PetscFunctionBegin;

    for (int i = 0; i < 3; ++i) {
        hx[i] = this->m_Opt->GetDomainPara().hx[i];
        isize[i] = this->m_Opt->GetDomainPara().isize[i];
        istart[i] = this->m_Opt->GetDomainPara().istart[i];
    }

    ierr = VecGetArray(m, &p_m); CHKERRQ(ierr);
#pragma omp parallel
{
    ScalarType x, x1, x2, x3, value;
    IntType i, i1, i2, i3;
#pragma omp for
    for (i1 = 0; i1 < isize[0]; ++i1) {  // i1
        for (i2 = 0; i2 < isize[1]; ++i2) {  // i2
            for (i3 = 0; i3 < isize[2]; ++i3) {  // i3
                // compute coordinates (nodal grid)
                x1 = hx[0]*static_cast<ScalarType>(i1 + istart[0]);
                x2 = hx[1]*static_cast<ScalarType>(i2 + istart[1]);
                x3 = hx[2]*static_cast<ScalarType>(i3 + istart[2]);

                // compute linear / flat index
                i = GetLinearIndex(i1,i2,i3,isize);

                x1 -= PETSC_PI;
                x2 -= PETSC_PI;
                x3 -= PETSC_PI;

                x = sqrt(x1*x1 + x2*x2 + x3*x3);

                value = x < PETSC_PI/2.0 ? 1.0 : 0.0;

                p_m[i] = value;
            }  // i1
        }  // i2
    }  // i3

} // pragma omp parallel

    ierr = VecRestoreArray(m,&p_m); CHKERRQ(ierr);

    PetscFunctionReturn(0);
}




/********************************************************************
 * @brief compute 3D c-like shape/hollow sphere
 *******************************************************************/
#undef __FUNCT__
#define __FUNCT__ "ComputeHollowSphere"
PetscErrorCode SynProbRegistration::ComputeHollowSphere(Vec m) {
    PetscErrorCode ierr;
    IntType isize[3],istart[3];
    ScalarType *p_m=NULL,hx[3];

    PetscFunctionBegin;

    for (int i = 0; i < 3; ++i) {
        hx[i] = this->m_Opt->GetDomainPara().hx[i];
        isize[i] = this->m_Opt->GetDomainPara().isize[i];
        istart[i] = this->m_Opt->GetDomainPara().istart[i];
    }

    ierr = VecGetArray(m, &p_m); CHKERRQ(ierr);

#pragma omp parallel
{
    ScalarType x, x1, x2, x3, value;
    IntType i, i1, i2, i3;
#pragma omp for
    for (i1 = 0; i1 < isize[0]; ++i1) {  // x1
        for (i2 = 0; i2 < isize[1]; ++i2) { // x2
            for (i3 = 0; i3 < isize[2]; ++i3) { // x3
                // compute coordinates (nodal grid)
                x1 = hx[0]*static_cast<ScalarType>(i1 + istart[0]);
                x2 = hx[1]*static_cast<ScalarType>(i2 + istart[1]);
                x3 = hx[2]*static_cast<ScalarType>(i3 + istart[2]);

                // compute linear / flat index
                i = GetLinearIndex(i1,i2,i3,isize);

                x1 -= PETSC_PI;
                x2 -= PETSC_PI;
                x3 -= PETSC_PI;

                x = sqrt(x1*x1 + x2*x2 + x3*x3);

                // draw outter sphere
                value = x < 1.2*PETSC_PI/2.0 ? 1.0 : 0.0;

                // opening
                x = sqrt((x1-1.8*PETSC_PI)*(x1-1.8*PETSC_PI) + x2*x2 + x3*x3);
                value = x < 3.0*PETSC_PI/2.0 ? 0.0 : value;

                // set inner circle to zero
                x = sqrt(x1*x1 + x2*x2 + x3*x3);
                value = x < 0.8*PETSC_PI/2.0 ? 0.0 : value;

                p_m[i] = value;
            }  // i1
        }  // i2
    }  // i3
}  // pragma omp parallel

    ierr = VecRestoreArray(m, &p_m); CHKERRQ(ierr);

    PetscFunctionReturn(0);
}





/********************************************************************
 * @brief compute exp sin field
 *******************************************************************/
#undef __FUNCT__
#define __FUNCT__ "ComputeExpSin"
PetscErrorCode SynProbRegistration::ComputeExpSin(Vec m) {
    PetscErrorCode ierr = 0;
    IntType isize[3], istart[3];
    const ScalarType twopi = 2.0*PETSC_PI;
    ScalarType *p_m = NULL, hx[3], sigma[3];

    PetscFunctionBegin;

    for (int i = 0; i < 3; ++i) {
        hx[i] = this->m_Opt->GetDomainPara().hx[i];
        isize[i] = this->m_Opt->GetDomainPara().isize[i];
        istart[i] = this->m_Opt->GetDomainPara().istart[i];
    }

    ierr = VecGetArray(m, &p_m); CHKERRQ(ierr);

    sigma[0] = twopi/8.0;
    sigma[1] = twopi/8.0;
    sigma[2] = twopi/8.0;

#pragma omp parallel
{
    ScalarType x1, x2, x3, s1, s2, s3, x1s, x2s, x3s;
    IntType i, i1, i2, i3;
#pragma omp for
    for (i1 = 0; i1 < isize[0]; ++i1) {  // x1
        for (i2 = 0; i2 < isize[1]; ++i2) { // x2
            for (i3 = 0; i3 < isize[2]; ++i3) { // x3
                // compute coordinates (nodal grid)
                x1 = hx[0]*static_cast<ScalarType>(i1 + istart[0]);
                x2 = hx[1]*static_cast<ScalarType>(i2 + istart[1]);
                x3 = hx[2]*static_cast<ScalarType>(i3 + istart[2]);

                // compute linear / flat index
                i = GetLinearIndex(i1, i2, i3, isize);

                s1 = sin(2.0*x1);
                s2 = sin(2.0*x2);
                s3 = sin(2.0*x3);

                x1s = x1 - twopi/2.0;
                x2s = x2 - twopi/2.0;
                x3s = x3 - twopi/2.0;
                x1s = x1s*x1s / (2.0*sigma[0]*sigma[0]);
                x2s = x2s*x2s / (2.0*sigma[1]*sigma[1]);
                x3s = x3s*x3s / (2.0*sigma[2]*sigma[2]);

                p_m[i] = exp( - (x1s + x2s + x3s) ) * (s1*s1 + s2*s2 + s3*s3);
            }  // i1
        }  // i2
    }  // i3
}  // pragma omp parallel

    ierr = VecRestoreArray(m, &p_m); CHKERRQ(ierr);

    PetscFunctionReturn(0);
}




/********************************************************************
 * @brief compute diamond
 *******************************************************************/
#undef __FUNCT__
#define __FUNCT__ "ComputeDiamond"
PetscErrorCode SynProbRegistration::ComputeDiamond(Vec m, int id) {
    PetscErrorCode ierr = 0;
    IntType isize[3], istart[3];
    ScalarType *p_m = NULL, hx[3];

    PetscFunctionBegin;

    for (int i = 0; i < 3; ++i) {
        hx[i] = this->m_Opt->GetDomainPara().hx[i];
        isize[i] = this->m_Opt->GetDomainPara().isize[i];
        istart[i] = this->m_Opt->GetDomainPara().istart[i];
    }

    ierr = VecGetArray(m, &p_m); CHKERRQ(ierr);
#pragma omp parallel
{
    ScalarType x1, x2, x3, x;
    IntType i1, i2, i3, i;
#pragma omp for
    for (i1 = 0; i1 < isize[0]; ++i1) {  // x1
        for (i2 = 0; i2 < isize[1]; ++i2) { // x2
            for (i3 = 0; i3 < isize[2]; ++i3) { // x3
                // compute coordinates (nodal grid)
                x1 = hx[0]*static_cast<ScalarType>(i1 + istart[0]);
                x2 = hx[1]*static_cast<ScalarType>(i2 + istart[1]);
                x3 = hx[2]*static_cast<ScalarType>(i3 + istart[2]);

                // compute linear / flat index
                i = GetLinearIndex(i1,i2,i3,isize);

                x1 = fabs(x1-PETSC_PI);
                x2 = fabs(x2-PETSC_PI);
                x3 = fabs(x3-PETSC_PI);

                x = 0;
                if (id == 1) {
                    x = x1 + x2 + x3;
                    if (x < sqrt(3.0)*PETSC_PI/2.0) p_m[i] =  0.4;
                    if (x < sqrt(3.0)*PETSC_PI/3.0) p_m[i] =  0.6;
                    if (x < sqrt(3.0)*PETSC_PI/4.0) p_m[i] =  0.8;
                    if (x < sqrt(3.0)*PETSC_PI/8.0) p_m[i] =  1.0;
                } else {
                    x = std::max(std::max(x1,x2),x3);
                    if (x < PETSC_PI/2.0) p_m[i] =  0.4;
                    if (x < PETSC_PI/3.0) p_m[i] =  0.6;
                    if (x < PETSC_PI/4.0) p_m[i] =  0.8;
                    if (x < PETSC_PI/8.0) p_m[i] =  1.0;
                }
            }  // i1
        }  // i2
    }  // i3
}  // pragma omp parallel

    ierr = VecRestoreArray(m, &p_m); CHKERRQ(ierr);

    PetscFunctionReturn(0);
}




}  // namespace reg




#endif // _SYNPROBREGISTRATION_CPP_
