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


#include <string>

#include "RegToolsOpt.hpp"
#include "RegUtils.hpp"
#include "PreProcReg.hpp"
#include "VecField.hpp"
#include "ReadWriteReg.hpp"
#include "SynProbRegistration.hpp"
#include "RegistrationInterface.hpp"

PetscErrorCode RunPostProcessing(reg::RegToolsOpt*);
PetscErrorCode ResampleVecField(reg::RegToolsOpt*);
PetscErrorCode ResampleScaField(reg::RegToolsOpt*);
PetscErrorCode ComputeDefFields(reg::RegToolsOpt*);
PetscErrorCode ComputeGrad(reg::RegToolsOpt*);
PetscErrorCode ComputeResidual(reg::RegToolsOpt*);
PetscErrorCode ComputeError(reg::RegToolsOpt*);
PetscErrorCode ComputeSynVel(reg::RegToolsOpt*);
PetscErrorCode SolveForwardProblem(reg::RegToolsOpt*);
PetscErrorCode CheckAdjointSolve(reg::RegToolsOpt*);
PetscErrorCode CheckForwardSolve(reg::RegToolsOpt*);
PetscErrorCode CheckDetDefGradSolve(reg::RegToolsOpt*);
PetscErrorCode ConvertData(reg::RegToolsOpt*);
PetscErrorCode ApplySmoothing(reg::RegToolsOpt*);




/********************************************************************
 * @brief main function for registration tools
 *******************************************************************/
int main(int argc, char **argv) {
    PetscErrorCode ierr = 0;

    reg::RegToolsOpt* regopt = NULL;

    // initialize petsc (user is not allowed to set petsc options)
    ierr = PetscInitialize(0, reinterpret_cast<char***>(NULL),
                              reinterpret_cast<char*>(NULL),
                              reinterpret_cast<char*>(NULL)); CHKERRQ(ierr);
    PetscFunctionBegin;

    // allocate class for controlling everything
    try {regopt = new reg::RegToolsOpt(argc, argv);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    if (regopt->GetFlags().computedeffields) {
        ierr = ComputeDefFields(regopt); CHKERRQ(ierr);
    } else if (regopt->GetFlags().computegrad) {
        ierr = ComputeGrad(regopt); CHKERRQ(ierr);
    } else if (regopt->GetFlags().resample) {
        if (regopt->GetFlags().readvecfield) {
            ierr = ResampleVecField(regopt); CHKERRQ(ierr);
        }
        if (regopt->GetFlags().readscafield) {
            ierr = ResampleScaField(regopt); CHKERRQ(ierr);
        }
    } else if (regopt->GetFlags().tscafield) {
        ierr = SolveForwardProblem(regopt); CHKERRQ(ierr);
    } else if (regopt->GetFlags().tlabelmap) {
        ierr = SolveForwardProblem(regopt); CHKERRQ(ierr);
    } else if (regopt->GetFlags().computeresidual) {
        ierr = ComputeResidual(regopt); CHKERRQ(ierr);
    } else if (regopt->GetFlags().computeerror) {
        ierr = ComputeError(regopt); CHKERRQ(ierr);
    } else if (regopt->GetFlags().computesynvel) {
        ierr = ComputeSynVel(regopt); CHKERRQ(ierr);
    } else if (regopt->GetFlags().checkfwdsolve) {
        ierr = CheckForwardSolve(regopt); CHKERRQ(ierr);
    } else if (regopt->GetFlags().checkadjsolve) {
        ierr = CheckAdjointSolve(regopt); CHKERRQ(ierr);
    } else if (regopt->GetFlags().checkdetdefgradsolve) {
        ierr = CheckDetDefGradSolve(regopt); CHKERRQ(ierr);
    } else if (regopt->GetFlags().convert) {
        ierr = ConvertData(regopt); CHKERRQ(ierr);
    } else if (regopt->GetFlags().applysmoothing) {
        ierr = ApplySmoothing(regopt); CHKERRQ(ierr);
    }

    // clean up
    if (regopt != NULL) {delete regopt; regopt = NULL;}

    // clean up petsc
    ierr = PetscFinalize(); CHKERRQ(ierr);

    return 0;
}




/********************************************************************
 * @brief compute gradient of scalar field
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode ComputeGrad(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    std::string filename, fnx1, fnx2, fnx3;
    std::stringstream ss;
    int rank;
    double timers[5] = {0, 0, 0, 0, 0};
    ScalarType *p_m = NULL, *p_gm1 = NULL, *p_gm2 = NULL, *p_gm3 = NULL;
    Vec m = NULL;
    std::bitset<3> XYZ; XYZ[0] = 1; XYZ[1] = 1; XYZ[2] = 1;
    reg::VecField* grad = NULL;
    reg::ReadWriteReg* readwrite = NULL;

    PetscFunctionBegin;

    regopt->Enter(__func__);

    // allocate class for io
    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

    if (regopt->GetFlags().readscafield) {
        // read velocity components
        filename = regopt->GetScaFieldFN(0);
        ierr = readwrite->Read(&m, filename); CHKERRQ(ierr);
        ierr = reg::Assert(m != NULL, "null pointer"); CHKERRQ(ierr);

        if (!regopt->SetupDone()) {ierr = regopt->DoSetup(); CHKERRQ(ierr);}

        try {grad = new reg::VecField(regopt);}
        catch (std::bad_alloc&) {
            ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
        }

        // computing gradient of m
        ierr = VecGetArray(m, &p_m); CHKERRQ(ierr);
        ierr = grad->GetArrays(p_gm1, p_gm2, p_gm3); CHKERRQ(ierr);
        accfft_grad(p_gm1, p_gm2, p_gm3, p_m, regopt->GetFFT().plan, &XYZ, timers);
        ierr = grad->RestoreArrays(p_gm1, p_gm2, p_gm3); CHKERRQ(ierr);
        ierr = VecRestoreArray(m, &p_m); CHKERRQ(ierr);

        // write to file
        fnx1 = regopt->GetVecFieldFN(0, 1);
        fnx2 = regopt->GetVecFieldFN(1, 1);
        fnx3 = regopt->GetVecFieldFN(2, 1);
        ierr = readwrite->Write(grad, fnx1, fnx2, fnx3); CHKERRQ(ierr);

    } else if (regopt->GetFlags().readvecfield) {
    }

    if (m != NULL) {ierr = VecDestroy(&m); CHKERRQ(ierr);}
    if (grad != NULL) {delete grad; grad = NULL;}
    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}

    regopt->Exit(__func__);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief post process image registration results
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode RunPostProcessing(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    std::string ifolder, xfolder, filename, ext;
    Vec mT = NULL, mR = NULL, vx1 = NULL, vx2 = NULL, vx3 = NULL;
    reg::VecField *v = NULL;
    reg::ReadWriteReg* readwrite = NULL;
    reg::RegistrationInterface* registration = NULL;

    PetscFunctionBegin;

    // allocate class for io
    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }
    ierr = reg::Msg("processing results"); CHKERRQ(ierr);

    ext = regopt->GetReadWriteFlags().extension;
    ifolder = regopt->GetReadWriteFlags().ifolder;
    ierr = reg::Assert(ifolder.empty() != true, "input folder needs to be provided"); CHKERRQ(ierr);

    // read template image
    filename = ifolder + "template-image" + ext;
    ierr = readwrite->Read(&mT, filename); CHKERRQ(ierr);
    ierr = reg::Assert(mT != NULL, "null pointer"); CHKERRQ(ierr);

    if ( !regopt->SetupDone() ) { ierr = regopt->DoSetup(); CHKERRQ(ierr); }

    // read reference image
    filename = ifolder + "reference-image" + ext;
    ierr = readwrite->Read(&mR, filename); CHKERRQ(ierr);
    ierr = reg::Assert(mR != NULL, "null pointer"); CHKERRQ(ierr);

    if (!regopt->SetupDone()) {ierr = regopt->DoSetup(); CHKERRQ(ierr);}

    // allocate container for velocity field
    try { v = new reg::VecField(regopt); }
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // read velocity components
    filename = ifolder + "velocity-field-x1" + ext;
    ierr = readwrite->Read(&vx1, filename); CHKERRQ(ierr);
    ierr = VecCopy(vx1, v->m_X1); CHKERRQ(ierr);

    filename = ifolder + "velocity-field-x2" + ext;
    ierr = readwrite->Read(&vx2, filename); CHKERRQ(ierr);
    ierr = VecCopy(vx2, v->m_X2); CHKERRQ(ierr);

    filename = ifolder + "velocity-field-x3" + ext;
    ierr = readwrite->Read(&vx3, filename); CHKERRQ(ierr);
    ierr = VecCopy(vx3, v->m_X3); CHKERRQ(ierr);

    // allocate class for registration interface
    try {registration = new reg::RegistrationInterface(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // set all we need
    ierr = registration->SetReadWrite(readwrite); CHKERRQ(ierr);
    ierr = registration->SetReferenceImage(mR); CHKERRQ(ierr);
    ierr = registration->SetTemplateImage(mT); CHKERRQ(ierr);
    ierr = registration->SetInitialGuess(v); CHKERRQ(ierr);

    // run post processing
    ierr = registration->RunPostProcessing(); CHKERRQ(ierr);

    if (mT != NULL) {ierr = VecDestroy(&mT); CHKERRQ(ierr); mT = NULL;}
    if (mR != NULL) {ierr = VecDestroy(&mR); CHKERRQ(ierr); mR = NULL;}

    if (vx1 != NULL) {ierr = VecDestroy(&vx1); CHKERRQ(ierr); vx1 = NULL;}
    if (vx2 != NULL) {ierr = VecDestroy(&vx2); CHKERRQ(ierr); vx2 = NULL;}
    if (vx3 != NULL) {ierr = VecDestroy(&vx3); CHKERRQ(ierr); vx3 = NULL;}

    if (v != NULL) {delete v; v = NULL;}

    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}
    if (registration != NULL) {delete registration; registration = NULL;}

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief post process image registration results
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode ComputeDefFields(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    std::string ifolder, xfolder, filename, ext;
    Vec vxi = NULL;
    reg::VecField* v = NULL;
    reg::ReadWriteReg* readwrite = NULL;
    reg::SynProbRegistration* synprob = NULL;
    reg::RegistrationInterface* registration = NULL;

    PetscFunctionBegin;

    // allocate class for io
    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    ext = regopt->GetReadWriteFlags().extension;
    ifolder = regopt->GetReadWriteFlags().ifolder;
    if (!ifolder.empty()) {
        // read velocity components
        filename = ifolder + "velocity-field-x1" + ext;
        ierr = readwrite->Read(&vxi, filename); CHKERRQ(ierr);
        if (!regopt->SetupDone()) {ierr = regopt->DoSetup(); CHKERRQ(ierr);}

        // allocate container for velocity field
        try {v = new reg::VecField(regopt);}
        catch (std::bad_alloc&) {
            ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
        }
        ierr = VecCopy(vxi, v->m_X1); CHKERRQ(ierr);
        if (vxi != NULL) {ierr = VecDestroy(&vxi); CHKERRQ(ierr); vxi = NULL;}

        filename = ifolder + "velocity-field-x2" + ext;
        ierr = readwrite->Read(&vxi, filename); CHKERRQ(ierr);
        ierr = VecCopy(vxi, v->m_X2); CHKERRQ(ierr);
        if (vxi != NULL) {ierr = VecDestroy(&vxi); CHKERRQ(ierr); vxi = NULL;}

        filename = ifolder + "velocity-field-x3" + ext;
        ierr = readwrite->Read(&vxi, filename); CHKERRQ(ierr);
        ierr = VecCopy(vxi, v->m_X3); CHKERRQ(ierr);
        if (vxi != NULL) {ierr = VecDestroy(&vxi); CHKERRQ(ierr); vxi = NULL;}
    } else {
        // allocate container for velocity field
        if (!regopt->SetupDone()) {ierr = regopt->DoSetup(); CHKERRQ(ierr);}
        try {v = new reg::VecField(regopt);}
        catch (std::bad_alloc&) {
            ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
        }
        try {synprob = new reg::SynProbRegistration(regopt);}
        catch (std::bad_alloc&) {
            ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
        }
        // set up smooth problem
        ierr = synprob->ComputeSmoothVectorField(v, 2); CHKERRQ(ierr);
    }

    // allocate class for registration interface
    try {registration = new reg::RegistrationInterface(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // set all we need
    ierr = registration->SetReadWrite(readwrite); CHKERRQ(ierr);
    ierr = registration->SetInitialGuess(v); CHKERRQ(ierr);

    // run post processing
    ierr = registration->ComputeDefFields(); CHKERRQ(ierr);

    if (v != NULL) {delete v; v = NULL;}
    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}
    if (registration != NULL) {delete registration; registration = NULL;}
    if (synprob != NULL) {delete synprob; synprob = NULL;}

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief resample scalar field
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode ResampleScaField(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    std::string filename;
    std::stringstream ss;
    int rank;
    IntType nl, ng, nx[3], nxl[3];
    ScalarType gridscale, value, hd, hdl;
    bool pro, res;
    Vec m = NULL, ml = NULL;
    reg::PreProcReg* preproc = NULL;
    reg::ReadWriteReg* readwrite = NULL;

    PetscFunctionBegin;

    regopt->Enter(__func__);

    // allocate class for io
    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

    // read velocity components
    filename = regopt->GetScaFieldFN(0);
    ierr = readwrite->Read(&m, filename); CHKERRQ(ierr);
    ierr = reg::Assert(m != NULL, "null pointer"); CHKERRQ(ierr);

    if (!regopt->SetupDone()) {ierr = regopt->DoSetup(); CHKERRQ(ierr);}

    // compute grid size
    gridscale = regopt->GetResamplingPara().gridscale;

    for (int i = 0; i < 3; ++i) {
        nx[i] = regopt->GetDomainPara().nx[i];
    }

    pro = false; res = false;
    if (gridscale != 1.0) {
        if (gridscale == -1.0) {
            for (int i = 0; i < 3; ++i) {
                nxl[i] = regopt->GetResamplingPara().nx[i];
                ierr = reg::Assert(nxl[i] > 0, "input error"); CHKERRQ(ierr);
                if (nxl[i] > nx[i]) {
                    pro = true;
                } else if (nxl[i] < nx[i]) {
                    res = true;
                }
            }
        } else {
            for (int i = 0; i < 3; ++i) {
                value = gridscale*static_cast<ScalarType>(regopt->GetDomainPara().nx[i]);
                nxl[i] = static_cast<IntType>(ceil(value));
            }
            if (gridscale > 1) {
                pro = true;
            } else if (gridscale < 1) {
                res = true;
            }
        }
    }

    if (pro || res) {
        // allocate container for velocity field
        try {preproc = new reg::PreProcReg(regopt);}
        catch (std::bad_alloc&) {
            ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
        }
        preproc->ResetGridChangeOps(true);

        ierr = regopt->GetSizes(nxl, nl, ng); CHKERRQ(ierr);

        ss << "resampling scalar field  ("
           << nx[0] << "," << nx[1] << "," << nx[2] << ")"
           << " -> (" << nxl[0] << "," << nxl[1] << "," << nxl[2] << ")";
        ierr = reg::Msg(ss.str()); CHKERRQ(ierr);
        ss.str(std::string()); ss.clear();

        // allocate array
        ierr = reg::VecCreate(ml, nl, ng); CHKERRQ(ierr);

        // restrict of prolong the vector field
        if (pro) {
            ierr = preproc->Prolong(&ml, m, nxl, nx); CHKERRQ(ierr);
        } else if (res) {
            ierr = preproc->Restrict(&ml, m, nxl, nx); CHKERRQ(ierr);
        } else {
            ierr = reg::ThrowError("either prolong or restrict"); CHKERRQ(ierr);
        }

        // reset io
        if (readwrite != NULL) {delete readwrite; readwrite = NULL;}
        for (int i = 0; i < 3; ++i) {
            regopt->SetNumGridPoints(i, nxl[i]);
        }
        ierr = regopt->DoSetup(false); CHKERRQ(ierr);

        try {readwrite = new reg::ReadWriteReg(regopt);}
        catch (std::bad_alloc&) {
            ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
        }

        // write resampled scalar field to file
        filename = regopt->GetScaFieldFN(1);
        std::cout << filename << std::endl;
        ierr = readwrite->Write(ml, filename); CHKERRQ(ierr);
    } else {
        // simply write field to file
        filename = regopt->GetScaFieldFN(1);
        ierr = readwrite->Write(m, filename); CHKERRQ(ierr);
    }

    hd  = (2.0*PETSC_PI/nx[0])*(2.0*PETSC_PI/nx[1])*(2.0*PETSC_PI/nx[2]);
    hdl = (2.0*PETSC_PI/nxl[0])*(2.0*PETSC_PI/nxl[1])*(2.0*PETSC_PI/nxl[2]);

    ierr = VecTDot(m, m, &value); CHKERRQ(ierr);
    ss << "norm (" << nx[0] << " " << nx[1] << " " << nx[2] << "): " << value*hd;
    ierr = reg::Msg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    ierr = VecTDot(ml, ml, &value); CHKERRQ(ierr);
    ss << "norm (" << nxl[0] << " " << nxl[1] << " " << nxl[2] << "): " << value*hdl;
    ierr = reg::Msg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    if (m != NULL) {ierr = VecDestroy(&m); CHKERRQ(ierr);}
    if (ml != NULL) {ierr = VecDestroy(&ml); CHKERRQ(ierr);}

    if (preproc != NULL) {delete preproc; preproc = NULL;}
    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}

    regopt->Exit(__func__);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief resample vector field
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode ResampleVecField(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    std::string filename, fnx1, fnx2, fnx3;
    std::stringstream ss;
    IntType nl, ng, nx[3], nxl[3];
    ScalarType scale, hd, hdl, value;
    Vec vx1 = NULL, vx2 = NULL, vx3 = NULL;
    reg::VecField *v = NULL, *vl = NULL;
    reg::PreProcReg* preproc = NULL;
    reg::ReadWriteReg* readwrite = NULL;

    PetscFunctionBegin;

    // allocate class for io
    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // read velocity components
    filename = regopt->GetVecFieldFN(0, 0);
    ierr = readwrite->Read(&vx1, filename); CHKERRQ(ierr);

    filename = regopt->GetVecFieldFN(1, 0);
    ierr = readwrite->Read(&vx2, filename); CHKERRQ(ierr);

    filename = regopt->GetVecFieldFN(2, 0);
    ierr = readwrite->Read(&vx3, filename); CHKERRQ(ierr);

    if (!regopt->SetupDone()) {ierr = regopt->DoSetup(); CHKERRQ(ierr);}

    // allocate container for velocity field
    try {v = new reg::VecField(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }
    ierr = VecCopy(vx1, v->m_X1); CHKERRQ(ierr);
    ierr = VecCopy(vx2, v->m_X2); CHKERRQ(ierr);
    ierr = VecCopy(vx3, v->m_X3); CHKERRQ(ierr);

    // allocate container for velocity field
    try {preproc = new reg::PreProcReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }
    preproc->ResetGridChangeOps(true);

    // compute grid size
    scale = regopt->GetResamplingPara().gridscale;
    for (int i=0; i < 3; ++i) {
        nx[i]  = regopt->GetDomainPara().nx[i];
        nxl[i] = scale*regopt->GetDomainPara().nx[i];
    }
    ierr = regopt->GetSizes(nxl, nl, ng); CHKERRQ(ierr);

    ss << "resampling vector field  (" << nx[0] << "," << nx[1] << "," << nx[2] << ")"
       << " -> (" << nxl[0] << "," << nxl[1] << "," << nxl[2] << ")";
    ierr = reg::Msg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    // allocate container for velocity field
    try {vl = new reg::VecField(nl, ng);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // restrict of prolong the vector field
    if (scale > 1.0) {
        ierr = preproc->Prolong(vl, v, nxl, nx); CHKERRQ(ierr);
    } else {
        ierr = preproc->Restrict(vl, v, nxl, nx); CHKERRQ(ierr);
    }

    hd  = (2.0*PETSC_PI/nx[0])*(2.0*PETSC_PI/nx[1])*(2.0*PETSC_PI/nx[2]);
    hdl = (2.0*PETSC_PI/nxl[0])*(2.0*PETSC_PI/nxl[1])*(2.0*PETSC_PI/nxl[2]);

    ierr = VecTDot(v->m_X1, v->m_X1, &value); CHKERRQ(ierr);
    ss << std::scientific << "norm x1 (" << nx[0] << " " << nx[1] << " " << nx[2] << "): " << value*hd;
    ierr = reg::Msg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    ierr = VecTDot(vl->m_X1, vl->m_X1, &value); CHKERRQ(ierr);
    ss << std::scientific << "norm x1 (" << nxl[0] << " " << nxl[1] << " " << nxl[2] << "): " << value*hdl;
    ierr = reg::Msg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();


    ierr = VecTDot(v->m_X2, v->m_X2, &value); CHKERRQ(ierr);
    ss << std::scientific << "norm x1 (" << nx[0] << " " << nx[1] << " " << nx[2] << "): " << value*hd;
    ierr = reg::Msg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    ierr = VecTDot(vl->m_X2, vl->m_X2, &value); CHKERRQ(ierr);
    ss << std::scientific << "norm x1 (" << nxl[0] << " " << nxl[1] << " " << nxl[2] << "): " << value*hdl;
    ierr = reg::Msg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();


    ierr = VecTDot(v->m_X3, v->m_X3, &value); CHKERRQ(ierr);
    ss << std::scientific << "norm x1 (" << nx[0] << " " << nx[1] << " " << nx[2] << "): " << value*hd;
    ierr = reg::Msg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    ierr = VecTDot(vl->m_X3, vl->m_X3, &value); CHKERRQ(ierr);
    ss << std::scientific << "norm x1 (" << nxl[0] << " " << nxl[1] << " " << nxl[2] << "): " << value*hdl;
    ierr = reg::Msg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    // reset io
    if (readwrite != NULL) {
        delete readwrite; readwrite = NULL;
    }
    for (int i = 0; i < 3; ++i) {
        regopt->SetNumGridPoints(i, nxl[i]);
    }
    ierr = regopt->DoSetup(false); CHKERRQ(ierr);
    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // get output file name (based on input file name)
    fnx1 = regopt->GetVecFieldFN(0, 1);
    fnx2 = regopt->GetVecFieldFN(1, 1);
    fnx3 = regopt->GetVecFieldFN(2, 1);

    // write to file
    ierr = readwrite->Write(vl, fnx1, fnx2, fnx3); CHKERRQ(ierr);

    if (v != NULL) {delete v; v = NULL; }
    if (vl != NULL) {delete vl; vl = NULL;}

    if (vx1 != NULL) {ierr = VecDestroy(&vx1); CHKERRQ(ierr); vx1 = NULL;}
    if (vx2 != NULL) {ierr = VecDestroy(&vx2); CHKERRQ(ierr); vx2 = NULL;}
    if (vx3 != NULL) {ierr = VecDestroy(&vx3); CHKERRQ(ierr); vx3 = NULL;}

    if (preproc != NULL) {delete preproc; preproc = NULL;}
    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief solve forward problem
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode SolveForwardProblem(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    IntType nl;
    std::string fn;
    std::stringstream ss;
    reg::VecField* v = NULL;
    Vec m0 = NULL, m1 = NULL, vxi = NULL;
    ScalarType *p_m1 = NULL, *p_m0 = NULL;
    reg::ReadWriteReg* readwrite = NULL;
    reg::PreProcReg* preproc = NULL;
    reg::RegistrationInterface* registration = NULL;
    PetscFunctionBegin;

    regopt->Enter(__func__);

    // allocate class for io
    try { readwrite = new reg::ReadWriteReg(regopt); }
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // read velocity components
    fn = regopt->GetScaFieldFN(0);
    ierr = readwrite->Read(&m0, fn); CHKERRQ(ierr);
    ierr = reg::Assert(m0 != NULL, "null pointer"); CHKERRQ(ierr);
    if (!regopt->SetupDone()) {
        ierr = regopt->DoSetup(); CHKERRQ(ierr);
    }
    ierr = VecDuplicate(m0, &m1); CHKERRQ(ierr);

    nl = regopt->GetDomainPara().nl;

    // if we consider a lable map, we want to truncate the values
    if (regopt->GetFlags().tlabelmap) {
        // map to integer
        ierr = VecGetArray(m0, &p_m0); CHKERRQ(ierr);
        for (IntType i = 0; i < nl; ++i) {
            if (p_m0[i] > 0.5) {
                p_m0[i] = 1.0;
            } else {
                p_m0[i] = 0.0;
            }
        }
        ierr = VecRestoreArray(m0, &p_m0); CHKERRQ(ierr);

        try {preproc = new reg::PreProcReg(regopt);}
        catch (std::bad_alloc&) {
            ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
        }
        ierr = preproc->ApplySmoothing(m1, m0); CHKERRQ(ierr);
        ierr = VecCopy(m1, m0); CHKERRQ(ierr);
    } else {
        ierr = reg::Rescale(m0, 0.0, 1.0); CHKERRQ(ierr);
    }

    try {v = new reg::VecField(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // read the individual components of the vector fields
    fn = regopt->GetVecFieldFN(0, 0);
    ierr = readwrite->Read(&vxi, fn); CHKERRQ(ierr);
    ierr = VecCopy(vxi, v->m_X1); CHKERRQ(ierr);
    if (vxi != NULL) {ierr = VecDestroy(&vxi); CHKERRQ(ierr); vxi = NULL;}

    fn = regopt->GetVecFieldFN(1, 0);
    ierr = readwrite->Read(&vxi, fn); CHKERRQ(ierr);
    ierr = VecCopy(vxi, v->m_X2); CHKERRQ(ierr);
    if (vxi != NULL) {ierr = VecDestroy(&vxi); CHKERRQ(ierr); vxi = NULL;}

    fn = regopt->GetVecFieldFN(2, 0);
    ierr = readwrite->Read(&vxi, fn); CHKERRQ(ierr);
    ierr = VecCopy(vxi, v->m_X3); CHKERRQ(ierr);
    if (vxi != NULL) {ierr = VecDestroy(&vxi); CHKERRQ(ierr); vxi = NULL;}

    // allocate class for registration interface
    try {registration = new reg::RegistrationInterface(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // set all we need
    ierr = registration->SetReadWrite(readwrite); CHKERRQ(ierr);
    ierr = registration->SetInitialGuess(v); CHKERRQ(ierr);

    // run post processing
    ierr = registration->SolveForwardProblem(m1, m0); CHKERRQ(ierr);

    // if we consider a lable map, we want to truncate the values
    if (regopt->GetFlags().tlabelmap) {
        // map to integer
        ierr = VecGetArray(m1, &p_m1); CHKERRQ(ierr);
        for (IntType i = 0; i < nl; ++i) {
            if (p_m1[i] > 0.5) {
                p_m1[i] = 1.0;
            } else {
                p_m1[i] = 0.0;
            }
        }
        ierr = VecRestoreArray(m1, &p_m1); CHKERRQ(ierr);
    }

    // write resampled scalar field to file
    fn = regopt->GetScaFieldFN(1);
    ierr = readwrite->Write(m1, fn); CHKERRQ(ierr);

    if (v != NULL) {delete v; v = NULL;}
    if (m0 != NULL) {ierr = VecDestroy(&m0); CHKERRQ(ierr); m0 = NULL;}
    if (m1 != NULL) {ierr = VecDestroy(&m1); CHKERRQ(ierr); m1 = NULL;}
    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}
    if (preproc != NULL) {delete preproc; preproc = NULL;}
    if (registration != NULL) {delete registration; registration = NULL;}

    regopt->Exit(__func__);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief compute error
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode ComputeError(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    std::string fn;
    std::stringstream ss;
    Vec mR = NULL, mT = NULL;
    ScalarType value, minval, maxval, ell2norm, ell8norm;
    reg::ReadWriteReg* readwrite = NULL;
    PetscFunctionBegin;

    regopt->Enter(__func__);

    // allocate class for io
    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // read reference image
    fn = regopt->GetScaFieldFN(2);
    ierr = readwrite->Read(&mR, fn); CHKERRQ(ierr);
    ierr = reg::Assert(mR != NULL, "null pointer"); CHKERRQ(ierr);
    if (!regopt->SetupDone()) {
        ierr = regopt->DoSetup(); CHKERRQ(ierr);
    }

    // read template image
    fn = regopt->GetScaFieldFN(3);
    ierr = readwrite->Read(&mT, fn); CHKERRQ(ierr);
    ierr = reg::Assert(mT != NULL, "null pointer"); CHKERRQ(ierr);

    ierr = VecNorm(mR, NORM_2, &ell2norm); CHKERRQ(ierr);
    ell2norm = ell2norm <= 0.0 ? 1.0 : ell2norm;

    ierr = VecNorm(mR, NORM_INFINITY, &ell8norm); CHKERRQ(ierr);
    ell8norm = ell8norm <= 0.0 ? 1.0 : ell8norm;

    ierr = VecMin(mR, NULL, &minval); CHKERRQ(ierr);
    ierr = VecMax(mR, NULL, &maxval); CHKERRQ(ierr);
    ss << std::scientific << "mr (min, max) = (" << minval << "," << maxval << ")";
    ierr = reg::DbgMsg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    ierr = VecMin(mT, NULL, &minval); CHKERRQ(ierr);
    ierr = VecMax(mT, NULL, &maxval); CHKERRQ(ierr);
    ss << std::scientific << "mt (min, max) = (" << minval << "," << maxval << ")";
    ierr = reg::DbgMsg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    ierr = VecAXPY(mR, -1.0, mT); CHKERRQ(ierr);
    ierr = VecNorm(mR, NORM_2, &value); CHKERRQ(ierr);
    ss << std::scientific << "ell2-norm " << value/ell2norm << " (" << value << ")";
    ierr = reg::DbgMsg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    ierr = VecNorm(mR, NORM_INFINITY, &value); CHKERRQ(ierr);
    ss << std::scientific << "ell8-norm " << value/ell8norm << " (" << value << ")";
    ierr = reg::DbgMsg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}
    if (mR != NULL) {ierr = VecDestroy(&mR); CHKERRQ(ierr); mR = NULL;}
    if (mT != NULL) {ierr = VecDestroy(&mT); CHKERRQ(ierr); mT = NULL;}

    regopt->Exit(__func__);

    PetscFunctionReturn(ierr);
}





/********************************************************************
 * @brief compute residual
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode ComputeResidual(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    IntType nl;
    std::string fn;
    std::stringstream ss;
    int rank;
    Vec mR = NULL, mT = NULL;
    ScalarType *p_mr = NULL, *p_mt = NULL;
    reg::ReadWriteReg* readwrite = NULL;
    PetscFunctionBegin;

    regopt->Enter(__func__);

    // allocate class for io
    try { readwrite = new reg::ReadWriteReg(regopt); }
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    MPI_Comm_rank(PETSC_COMM_WORLD, &rank);

    // read reference image
    fn = regopt->GetScaFieldFN(2);
    ierr = readwrite->Read(&mR, fn); CHKERRQ(ierr);
    ierr = reg::Assert(mR != NULL, "null pointer"); CHKERRQ(ierr);

    if (!regopt->SetupDone()) {
        ierr = regopt->DoSetup(); CHKERRQ(ierr);
    }

    // read template image
    fn = regopt->GetScaFieldFN(3);
    ierr = readwrite->Read(&mT, fn); CHKERRQ(ierr);
    ierr = reg::Assert(mT != NULL, "null pointer"); CHKERRQ(ierr);

    ierr = reg::Rescale(mR, 0, 1); CHKERRQ(ierr);
    ierr = reg::Rescale(mT, 0, 1); CHKERRQ(ierr);

    nl = regopt->GetDomainPara().nl;

    ierr = VecGetArray(mR, &p_mr); CHKERRQ(ierr);
    ierr = VecGetArray(mT, &p_mt); CHKERRQ(ierr);
#pragma omp parallel
{
#pragma omp for
    for (IntType i = 0; i < nl; ++i) {  // for all grid points
        p_mt[i] = 1.0 - PetscAbs(p_mt[i] - p_mr[i]);
    }
}  // pragma omp
    ierr = VecRestoreArray(mR, &p_mr); CHKERRQ(ierr);
    ierr = VecRestoreArray(mT, &p_mt); CHKERRQ(ierr);

    // write resampled scalar field to file
    fn = regopt->GetScaFieldFN(1);
    ierr = readwrite->Write(mT, fn); CHKERRQ(ierr);

    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}
    if (mT != NULL) {ierr = VecDestroy(&mT); CHKERRQ(ierr); mT = NULL;}
    if (mR != NULL) {ierr = VecDestroy(&mR); CHKERRQ(ierr); mR = NULL;}

    regopt->Exit(__func__);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief compute synthetic velocity field
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode ComputeSynVel(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    int problem = 3;
    reg::VecField* v = NULL;
    reg::ReadWriteReg* readwrite = NULL;
    ScalarType *p_vx1 = NULL, *p_vx2 = NULL, *p_vx3 = NULL;
    ScalarType hx[3];
    std::string fnx1, fnx2, fnx3;
    PetscFunctionBegin;

    regopt->Enter(__func__);

    ierr = regopt->DoSetup(); CHKERRQ(ierr);

    // allocate class for io
    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // allocate class for io
    try {v = new reg::VecField(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    for (int i = 0; i < 3; ++i) {
        hx[i] = regopt->GetDomainPara().hx[i];
    }

    ierr = v->GetArrays(p_vx1, p_vx2, p_vx3); CHKERRQ(ierr);
#pragma omp parallel
{
    ScalarType x1, x2, x3;
    IntType i1, i2, i3, i;
#pragma omp for
    for (i1 = 0; i1 < regopt->GetDomainPara().isize[0]; ++i1) {  // x1
        for (i2 = 0; i2 < regopt->GetDomainPara().isize[1]; ++i2) {  // x2
            for (i3 = 0; i3 < regopt->GetDomainPara().isize[2]; ++i3) {  // x3
                // compute coordinates (nodal grid)
                x1 = hx[0]*static_cast<ScalarType>(i1 + regopt->GetDomainPara().istart[0]);
                x2 = hx[1]*static_cast<ScalarType>(i2 + regopt->GetDomainPara().istart[1]);
                x3 = hx[2]*static_cast<ScalarType>(i3 + regopt->GetDomainPara().istart[2]);

                // compute linear / flat index
                i = reg::GetLinearIndex(i1, i2, i3, regopt->GetDomainPara().isize);
                // compute the velocity field
                if (problem == 0) {
                    ScalarType v0 = 0.5;
                    p_vx1[i] = v0*sin(x3)*cos(x2)*sin(x2);
                    p_vx2[i] = v0*sin(x1)*cos(x3)*sin(x3);
                    p_vx3[i] = v0*sin(x2)*cos(x1)*sin(x1);
                } else if (problem == 1) {
                    p_vx1[i] = sin(2.0*x1)*cos(2.0*x2)*sin(2.0*x3);
                    p_vx2[i] = sin(2.0*x1)*cos(2.0*x2)*sin(2.0*x3);
                    p_vx3[i] = sin(2.0*x1)*cos(2.0*x2)*sin(2.0*x3);
                } else if (problem == 2) {
                    p_vx1[i] = cos(x1)*sin(x2);
                    p_vx2[i] = cos(x2)*sin(x1);
                    p_vx3[i] = cos(x1)*sin(x3);
                } else if (problem == 3) {
                    p_vx1[i] = cos(x2)*cos(x3);
                    p_vx2[i] = sin(x3)*sin(x1);
                    p_vx3[i] = cos(x1)*cos(x2);
                } else if (problem == 4) {
                    ScalarType v0 = 0.5;
                    p_vx1[i] = v0;
                    p_vx2[i] = v0;
                    p_vx3[i] = v0;
                }
            }  // i1
        }  // i2
    }  // i3
}  // pragma omp parallel
    ierr = v->RestoreArrays(p_vx1, p_vx2, p_vx3); CHKERRQ(ierr);

    // write computed vectorfield to file
    fnx1 = "velocity-field-x1" + regopt->GetReadWriteFlags().extension;
    fnx2 = "velocity-field-x2" + regopt->GetReadWriteFlags().extension;
    fnx3 = "velocity-field-x3" + regopt->GetReadWriteFlags().extension;
    ierr = readwrite->Write(v, fnx1, fnx2, fnx3); CHKERRQ(ierr);

    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}
    if (v != NULL) {delete v; v = NULL;}

    regopt->Exit(__func__);

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief check the forward solver
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode CheckForwardSolve(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    IntType nc, nl, ng;
    Vec m0 = NULL, m1 = NULL, m0tilde = NULL;
    reg::VecField *v = NULL;
    reg::RegistrationInterface* registration = NULL;
    reg::SynProbRegistration* synprob = NULL;
    reg::ReadWriteReg* readwrite = NULL;
    ScalarType errval, normval, minval, maxval;
    std::stringstream ss;
    PetscFunctionBegin;

    regopt->Enter(__func__);

    nc = 1;

    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    if (regopt->GetFlags().readscafield) {
        if (regopt->GetVerbosity() > 1) {
            ierr = reg::DbgMsg("reading m0"); CHKERRQ(ierr);
        }
        ierr = readwrite->Read(&m0, regopt->GetScaFieldFN(0)); CHKERRQ(ierr);
        ierr = reg::Assert(m0 != NULL, "null pointer"); CHKERRQ(ierr);
    } else {
        ierr = regopt->DoSetup(); CHKERRQ(ierr);
    }

    regopt->SetNumImageComponents(nc);
    regopt->DisableSmoothing();

    nc = regopt->GetDomainPara().nc;
    nl = regopt->GetDomainPara().nl;
    ng = regopt->GetDomainPara().ng;

    // allocation
    try {v = new reg::VecField(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }
    try {registration = new reg::RegistrationInterface(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }
    ierr = registration->SetReadWrite(readwrite); CHKERRQ(ierr);

    try {synprob = new reg::SynProbRegistration(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // allocate the data
    if (m0 == NULL) {
        ierr = reg::VecCreate(m0, nc*nl, nc*ng); CHKERRQ(ierr);
        //ierr = synprob->ComputeSmoothScalarField(m0, 0); CHKERRQ(ierr);
        ierr = synprob->ComputeSmoothScalarField(m0, 10); CHKERRQ(ierr);
    }

    ierr = reg::VecCreate(m1, nc*nl, nc*ng); CHKERRQ(ierr);
    ierr = reg::VecCreate(m0tilde, nc*nl, nc*ng); CHKERRQ(ierr);

    // set up smooth problem
    ierr = synprob->ComputeSmoothVectorField(v, 2); CHKERRQ(ierr);

    // set initial guess and solve forward problem
    ierr = registration->SetInitialGuess(v, true); CHKERRQ(ierr);
    ierr = registration->SolveForwardProblem(m1, m0); CHKERRQ(ierr);

    ierr = v->Scale(-1.0); CHKERRQ(ierr);
    ierr = registration->SetInitialGuess(v, true); CHKERRQ(ierr);
    ierr = registration->SolveForwardProblem(m0tilde, m1); CHKERRQ(ierr);

    if (regopt->GetReadWriteFlags().results) {
        ierr = readwrite->WriteMC(m0, "initial-condition" + regopt->GetReadWriteFlags().extension); CHKERRQ(ierr);
        ierr = readwrite->WriteMC(m1, "final-condition" + regopt->GetReadWriteFlags().extension); CHKERRQ(ierr);
        ierr = readwrite->WriteMC(m0tilde, "backward-solve" + regopt->GetReadWriteFlags().extension); CHKERRQ(ierr);
    }
    ierr = VecNorm(m0, NORM_2, &normval); CHKERRQ(ierr);
    ierr = VecMax(m0, NULL, &maxval); CHKERRQ(ierr);
    ierr = VecMin(m0, NULL, &minval); CHKERRQ(ierr);
    ss  << "m(t=0)                    (min,max,norm)=("
        << std::scientific << minval << "," << maxval << "," << normval << ")";
    ierr = reg::DbgMsg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    ierr = VecNorm(m1, NORM_2, &normval); CHKERRQ(ierr);
    ierr = VecMax(m1, NULL, &maxval); CHKERRQ(ierr);
    ierr = VecMin(m1, NULL, &minval); CHKERRQ(ierr);
    ss  << "m(t=1) = m(t=0) o v       (min,max,norm)=("
        << std::scientific << minval << "," << maxval << "," << normval << ")";
    ierr = reg::DbgMsg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    ierr = VecNorm(m0tilde, NORM_2, &normval); CHKERRQ(ierr);
    ierr = VecMax(m0tilde, NULL, &maxval); CHKERRQ(ierr);
    ierr = VecMin(m0tilde, NULL, &minval); CHKERRQ(ierr);
    ss  << "m(t=0) = m(t=1) o (-v)    (min,max,norm)=("
        << std::scientific << minval << "," << maxval << "," << normval << ")";
    ierr = reg::DbgMsg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();


    ierr = VecAXPY(m0tilde, -1.0, m0); CHKERRQ(ierr);
    ierr = VecNorm(m0tilde, NORM_2, &errval); CHKERRQ(ierr);
    ierr = VecNorm(m0, NORM_2, &normval); CHKERRQ(ierr);
    ss  << "error " << std::scientific
        << errval/normval << " (" << errval << ")";
    ierr = reg::DbgMsg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    regopt->Exit(__func__);

    if (v != NULL) {delete v; v = NULL;}
    if (m0 != NULL) {ierr = VecDestroy(&m0); CHKERRQ(ierr); m0 = NULL;}
    if (m1 != NULL) {ierr = VecDestroy(&m1); CHKERRQ(ierr); m1 = NULL;}
    if (synprob != NULL) {delete synprob; synprob = NULL;}
    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}
    if (registration != NULL) {delete registration; registration = NULL;}

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief check the adjoint solver
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode CheckAdjointSolve(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    IntType nc, nl, ng;
    Vec l0 = NULL, m1 = NULL, mR = NULL;
    reg::VecField *v = NULL;
    reg::RegistrationInterface* registration = NULL;
    reg::SynProbRegistration* synprob = NULL;
    reg::ReadWriteReg* readwrite = NULL;
    PetscFunctionBegin;

    regopt->Enter(__func__);

    nc = 2;

    ierr = regopt->DoSetup(); CHKERRQ(ierr);

    regopt->SetNumImageComponents(nc);
    regopt->DisableSmoothing();

    nc = regopt->GetDomainPara().nc;
    nl = regopt->GetDomainPara().nl;
    ng = regopt->GetDomainPara().ng;

    // allocation
    try {v = new reg::VecField(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }
    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }
    try {registration = new reg::RegistrationInterface(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }
    ierr = registration->SetReadWrite(readwrite); CHKERRQ(ierr);

    try {synprob = new reg::SynProbRegistration(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    ierr = reg::VecCreate(l0, nc*nl, nc*ng); CHKERRQ(ierr);
    ierr = reg::VecCreate(m1, nc*nl, nc*ng); CHKERRQ(ierr);
    ierr = reg::VecCreate(mR, nc*nl, nc*ng); CHKERRQ(ierr);
    ierr = VecSet(m1, 0.0); CHKERRQ(ierr);
    ierr = VecSet(mR, 0.0); CHKERRQ(ierr);
    ierr = VecSet(l0, 0.0); CHKERRQ(ierr);

    ierr = synprob->ComputeSmoothScalarField(mR, 0); CHKERRQ(ierr);
    ierr = synprob->ComputeSmoothVectorField(v, 2); CHKERRQ(ierr);
    ierr = registration->SetInitialGuess(v, true); CHKERRQ(ierr);
    ierr = registration->SetReferenceImage(mR); CHKERRQ(ierr);
    ierr = registration->SolveAdjointProblem(l0, m1); CHKERRQ(ierr);
    ierr = readwrite->WriteMC(l0, "initial-adjoint-variable.nc"); CHKERRQ(ierr);

    regopt->Exit(__func__);

    if (v != NULL) {delete v; v = NULL;}
    if (l0 != NULL) {ierr = VecDestroy(&l0); CHKERRQ(ierr); l0 = NULL;}
    if (m1 != NULL) {ierr = VecDestroy(&m1); CHKERRQ(ierr); m1 = NULL;}
    if (mR != NULL) {ierr = VecDestroy(&mR); CHKERRQ(ierr); mR = NULL;}
    if (synprob != NULL) {delete synprob; synprob = NULL;}
    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}
    if (registration != NULL) {delete registration; registration = NULL;}

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief check the jacobian solver
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode CheckDetDefGradSolve(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    IntType nl, ng;
    Vec detj = NULL, invdetj = NULL;
    reg::VecField *v = NULL;
    reg::RegistrationInterface* registration = NULL;
    reg::SynProbRegistration* synprob = NULL;
    reg::ReadWriteReg* readwrite = NULL;
    ScalarType minval, maxval, normval, errval;
    std::stringstream ss;
    PetscFunctionBegin;

    regopt->Enter(__func__);

    ierr = regopt->DoSetup(); CHKERRQ(ierr);

    regopt->DisableSmoothing();
    nl = regopt->GetDomainPara().nl;
    ng = regopt->GetDomainPara().ng;

    // allocation
    try {v = new reg::VecField(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }
    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }
    try {registration = new reg::RegistrationInterface(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }
    ierr = registration->SetReadWrite(readwrite); CHKERRQ(ierr);

    try {synprob = new reg::SynProbRegistration(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    ierr = reg::VecCreate(detj, nl, ng); CHKERRQ(ierr);
    ierr = reg::VecCreate(invdetj, nl, ng); CHKERRQ(ierr);

    ierr = synprob->ComputeSmoothVectorField(v, 2); CHKERRQ(ierr);
    ierr = registration->SetInitialGuess(v, true); CHKERRQ(ierr);
    ierr = registration->ComputeDetDefGrad(detj); CHKERRQ(ierr);

    regopt->ComputeInvDetDefGrad(true);
    ierr = registration->ComputeDetDefGrad(invdetj); CHKERRQ(ierr);

    ierr = VecNorm(detj, NORM_2, &normval); CHKERRQ(ierr);
    ierr = VecMax(detj, NULL, &maxval); CHKERRQ(ierr);
    ierr = VecMin(detj, NULL, &minval); CHKERRQ(ierr);
    ss  << "det(grad(y))    (min,max,norm)=("
        << std::scientific << minval << "," << maxval << "," << normval << ")";
    ierr = reg::DbgMsg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    ierr = VecNorm(invdetj, NORM_2, &normval); CHKERRQ(ierr);
    ierr = VecMax(invdetj, NULL, &maxval); CHKERRQ(ierr);
    ierr = VecMin(invdetj, NULL, &minval); CHKERRQ(ierr);
    ss  << "det(grad(y))    (min,max,norm)=("
        << std::scientific << minval << "," << maxval << "," << normval << ")";
    ierr = reg::DbgMsg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();

    if (regopt->GetReadWriteFlags().results) {
        ierr = readwrite->WriteMC(invdetj, "invdetj" + regopt->GetReadWriteFlags().extension); CHKERRQ(ierr);
        ierr = readwrite->WriteMC(detj, "detj" + regopt->GetReadWriteFlags().extension); CHKERRQ(ierr);
    }

    ierr = VecReciprocal(invdetj); CHKERRQ(ierr);

    ierr = VecAXPY(invdetj, -1.0, detj); CHKERRQ(ierr);
    ierr = VecNorm(invdetj, NORM_2, &errval); CHKERRQ(ierr);
    ierr = VecNorm(detj, NORM_2, &normval); CHKERRQ(ierr);
    ss  << "error " << std::scientific
        << errval/normval << " (" << errval << ")";
    ierr = reg::DbgMsg(ss.str()); CHKERRQ(ierr);
    ss.str(std::string()); ss.clear();


    regopt->Exit(__func__);

    if (v != NULL) {delete v; v = NULL;}
    if (detj != NULL) {ierr = VecDestroy(&detj); CHKERRQ(ierr); detj = NULL;}
    if (invdetj != NULL) {ierr = VecDestroy(&invdetj); CHKERRQ(ierr); invdetj = NULL;}
    if (synprob != NULL) {delete synprob; synprob = NULL;}
    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}
    if (registration != NULL) {delete registration; registration = NULL;}

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief convert the data
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode ConvertData(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    std::string fn, path, filename, extension;
    Vec m;
    reg::ReadWriteReg* readwrite = NULL;
    PetscFunctionBegin;

    regopt->Enter(__func__);

    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // read reference image
    fn = regopt->GetScaFieldFN(0);
    ierr = readwrite->Read(&m, fn); CHKERRQ(ierr);
    ierr = reg::Assert(m != NULL, "null pointer"); CHKERRQ(ierr);
    //if (!regopt->SetupDone()) {ierr = regopt->DoSetup(); CHKERRQ(ierr);}

    std::cout << fn << std::endl;
    ierr = reg::GetFileName(path, filename, extension, fn); CHKERRQ(ierr);
    std::cout << fn << std::endl;
    fn = path + "/" + filename + "_converted" + regopt->GetReadWriteFlags().extension;
    std::cout << fn << std::endl;
    ierr = readwrite->Write(m, fn); CHKERRQ(ierr);

    regopt->Exit(__func__);

    if (m != NULL) {ierr = VecDestroy(&m); CHKERRQ(ierr); m = NULL;}
    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}

    PetscFunctionReturn(ierr);
}




/********************************************************************
 * @brief apply smoothing to data
 * @param[in] regopt container for user defined options
 *******************************************************************/
PetscErrorCode ApplySmoothing(reg::RegToolsOpt* regopt) {
    PetscErrorCode ierr = 0;
    std::string fn, path, filename, extension;
    Vec m;
    reg::ReadWriteReg* readwrite = NULL;
    reg::PreProcReg* preproc = NULL;
    PetscFunctionBegin;

    regopt->Enter(__func__);

    try {readwrite = new reg::ReadWriteReg(regopt);}
    catch (std::bad_alloc&) {
        ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
    }

    // read reference image
    fn = regopt->GetScaFieldFN(0);
    ierr = readwrite->Read(&m, fn); CHKERRQ(ierr);
    ierr = reg::Assert(m != NULL, "null pointer"); CHKERRQ(ierr);
    if (!regopt->SetupDone()) {ierr = regopt->DoSetup(); CHKERRQ(ierr);}


    // allocate preprocessing class
    if (preproc == NULL) {
        try {preproc = new reg::PreProcReg(regopt);}
        catch (std::bad_alloc&) {
            ierr = reg::ThrowError("allocation failed"); CHKERRQ(ierr);
        }
    }
    // apply smoothing
    ierr = preproc->ApplySmoothing(m, m); CHKERRQ(ierr);

    ierr = reg::GetFileName(path, filename, extension, fn); CHKERRQ(ierr);
    fn = path + "/" + filename + "_test" + regopt->GetReadWriteFlags().extension;
    std::cout << fn << std::endl;
    ierr = readwrite->Write(m, fn); CHKERRQ(ierr);

    regopt->Exit(__func__);

    if (m != NULL) {ierr = VecDestroy(&m); CHKERRQ(ierr); m = NULL;}
    if (readwrite != NULL) {delete readwrite; readwrite = NULL;}
    if (preproc != NULL) {delete preproc; preproc = NULL;}

    PetscFunctionReturn(ierr);
}
