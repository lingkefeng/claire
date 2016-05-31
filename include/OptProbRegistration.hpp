/**
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

#ifndef _OPTIMIZATIONPROBLEMREGISTRATION_H_
#define _OPTIMIZATIONPROBLEMREGISTRATION_H_

#include "RegOpt.hpp"
#include "RegUtils.hpp"
#include "VecField.hpp"

namespace reg
{

// mat vec for two level preconditioner
PetscErrorCode TwoLevelPCMatVec(Mat,Vec,Vec);
PetscErrorCode PrecondMonitor(KSP,IntType,ScalarType,void*);


class OptProbRegistration
{

public:

    typedef OptProbRegistration Self;

    OptProbRegistration(void);
    OptProbRegistration(RegOpt*);
    //virtual ~OptProbRegistration(void) = 0;
    ~OptProbRegistration(void);

    inline RegOpt* GetOptions(void){return this->m_Opt;};

    inline ScalarType GetInitialGradNorm(){return this->m_InitGradNorm;};
    inline void SetInitialGradNorm(ScalarType value){this->m_InitGradNorm=value;};
    inline ScalarType GetInitialMismatchVal(){return this->m_InitMismatchVal;};
    inline void SetInitialMismatchVal(ScalarType value){this->m_InitMismatchVal=value;};
    inline ScalarType GetInitialObjVal(){return this->m_InitObjectiveVal;};
    inline void SetInitialObjVal(ScalarType value){this->m_InitObjectiveVal=value;};

    inline void SetKSPTolerance(ScalarType value){this->m_KSPTol = value;};
    inline void SetNumOuterIter(IntType value){this->m_NumOuterIter = value;};
    inline bool InFirstIteration(){return this->m_InFirstIteration;};
    inline void InFirstIteration(bool flag){this->m_InFirstIteration=flag;};

    /*! evaluate l2-distance between observed and predicted state */
    virtual PetscErrorCode EvaluateL2Distance(ScalarType*) = 0;

    /*! evaluate objective functional J(v) */
    virtual PetscErrorCode EvaluateObjective(ScalarType*,Vec) = 0;

    /*! evaluate gradient of Lagrangian L(v) */
    virtual PetscErrorCode EvaluateGradient(Vec,Vec) = 0;

    /*! apply Hessian matvec H\tilde{\vect{v}} */
    virtual PetscErrorCode HessianMatVec(Vec,Vec) = 0;

    /*! apply preconditioner for KKT system */
    virtual PetscErrorCode PrecondMatVec(Vec, Vec) = 0;

    /*! solve forward problem */
    virtual PetscErrorCode SolveForwardProblem(Vec) = 0;

    /*! finalize iteration */
    virtual PetscErrorCode FinalizeIteration(Vec) = 0;

    /*! finalize registration */
    virtual PetscErrorCode Finalize(Vec) = 0;

    /*! apply two level preconditioner */
    virtual PetscErrorCode TwoLevelPrecondMatVec(Vec,Vec) = 0;

    /*! apply two level preconditioner */
    virtual PetscErrorCode CheckBounds(Vec,bool&) = 0;

    /*! set registration options */
    PetscErrorCode SetOptions(RegOpt* opt);

    /*! check gradient (derivative check via tayler expansion) */
    PetscErrorCode DerivativeCheck(void);

    /*! check if hessian is symmetric */
    PetscErrorCode HessianSymmetryCheck(void);

protected:

    PetscErrorCode Initialize(void);
    virtual PetscErrorCode ClearMemory(void) = 0;

    inline ScalarType GetKSPTolerance(){return this->m_KSPTol;};

    RegOpt* m_Opt;

    bool m_InFirstIteration;

    ScalarType m_KSPTol;
    ScalarType m_InitGradNorm;
    ScalarType m_InitMismatchVal;
    ScalarType m_InitObjectiveVal;
    Mat m_PCMatVec;

    IntType m_NumOuterIter;

private:


};

} // end of namespace


#endif // _OPTIMIZATIONPROBLEMREGISTRATION_H_

