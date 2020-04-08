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
#include "interp3.hpp"
#if defined(REG_HAS_MPI_CUDA) || defined(REG_HAS_CUDA)
#include "interp3_gpu_mpi.hpp"
#endif
#include "zeitgeist.hpp"
#define CONSTANT

void TestFunction(ScalarType &val, const ScalarType x1, const ScalarType x2, const ScalarType x3) {
  val =  ( PetscSinReal(8*x1)*PetscSinReal(8*x1)
        + PetscSinReal(2*x2)*PetscSinReal(2*x2)
        + PetscSinReal(4*x3)*PetscSinReal(4*x3) )/3.0;
}

inline void printOnMaster(std::string msg) {
    PetscPrintf(PETSC_COMM_WORLD, msg.c_str());
}

template<typename T>
void printVector(T* x, int n, std::string name) {
    std::string msg;
    msg = name +  ": ";
    for (int i=0; i<n; i++) {
        msg += (std::to_string(x[i]) + ",");
    }
    msg += "\n";
    PetscPrintf(PETSC_COMM_WORLD, msg.c_str());
}

template<typename T>
void printScalar(T x, std::string name) {
    std::string msg = name + ": " + std::to_string(x) + "\n";
    PetscPrintf(PETSC_COMM_WORLD, msg.c_str());
}

void TestError(ScalarType *ref, ScalarType *eval, IntType nl, double *err, double *maxval) {
  double error = 0;
  double max = 0;
  for (int i = 0; i < nl; ++i) {
    double local = static_cast<double>(ref[i]) - static_cast<double>(eval[i]);
    double lmax = std::abs(eval[i]);
    if (lmax > max) max = lmax;
    error += local*local;
  }
  *err = sqrt(error/static_cast<double>(nl));
  *maxval = max;
}


namespace reg {
namespace UnitTest {
  
PetscErrorCode TestInterpolation(RegOpt *m_Opt) {
  PetscErrorCode ierr = 0;
  PetscFunctionBegin;
  
  std::cout << "starting interpolation unit test" << std::endl;
  
  srand(time(0));
  
  int nx[3], istart[3], isize[3], nl;
  ScalarType hx[3], scale;
  
  nl = 1;
  for (int i = 0; i < 3; ++i) {
    nx[i]     = m_Opt->m_Domain.nx[i];
    isize[i]  = m_Opt->m_Domain.isize[i];
    istart[i] = m_Opt->m_Domain.istart[i];
    hx[i]     = m_Opt->m_Domain.hx[i];
    nl *= isize[i];
  }
    
  ScalarType *grid = new ScalarType[nl];
  ScalarType *q    = new ScalarType[nl*3];
  ScalarType *q1   = &q[0];
  ScalarType *q2   = &q[nl];
  ScalarType *q3   = &q[nl*2];
  ScalarType *ref  = new ScalarType[nl];
  ScalarType *eval = new ScalarType[nl];
  
  int iq = 10;

  for (int i1 = 0; i1 < isize[0]; ++i1) {  // x1
    for (int i2 = 0; i2 < isize[1]; ++i2) {  // x2
      for (int i3 = 0; i3 < isize[2]; ++i3) {  // x3
        // compute coordinates (nodal grid)
        double x1 = hx[0]*static_cast<double>(i1 + istart[0]);
        double x2 = hx[1]*static_cast<double>(i2 + istart[1]);
        double x3 = hx[2]*static_cast<double>(i3 + istart[2]);

        // compute linear / flat index
        int i = reg::GetLinearIndex(i1, i2, i3, m_Opt->m_Domain.isize);
        
        TestFunction(grid[i], x1, x2, x3);
        
       // x1 += hx[0]*0.5;
       // x2 += hx[1]*0.5;
       // x3 += hx[2]*0.5;
        
        x1 +=0* hx[0];
        x2 +=0* hx[1];
        x3 +=0* hx[2];
        
        if (x1 > 2.*M_PI) x1 -= 2.*M_PI;
        if (x2 > 2.*M_PI) x2 -= 2.*M_PI;
        if (x3 > 2.*M_PI) x3 -= 2.*M_PI;
    
        //x1 = static_cast<double>(rand())*2.*M_PI/static_cast<double>(RAND_MAX);
        //x2 = static_cast<double>(rand())*2.*M_PI/static_cast<double>(RAND_MAX);
        //x3 = static_cast<double>(rand())*2.*M_PI/static_cast<double>(RAND_MAX);
        
        TestFunction(ref[i], x1, x2, x3);
        eval[i] = 0.;
    
#ifdef REG_HAS_CUDA
        q1[i] = x1*nx[0]/(2.*M_PI);
        q2[i] = x2*nx[1]/(2.*M_PI);
        q3[i] = x3*nx[2]/(2.*M_PI);
#else
        q[3*i + 0] = x1;
        q[3*i + 1] = x2;
        q[3*i + 2] = x3;
#endif
      }  // i1
    }  // i2
  }  // i3

#if 0 // CUDA interpolation deactivated for multigpu (using cpu interpolation for multi gpu)
#ifdef REG_HAS_CUDA
  ScalarType *pg, *pq1, *pq2, *pq3, *pe;
  float *tmp1, *tmp2;
  
  cudaMalloc((void**)(&pg), sizeof(ScalarType)*nl);
  cudaMalloc((void**)(&pq1), sizeof(ScalarType)*nl);
  cudaMalloc((void**)(&pq2), sizeof(ScalarType)*nl);
  cudaMalloc((void**)(&pq3), sizeof(ScalarType)*nl);
  cudaMalloc((void**)(&pe), sizeof(ScalarType)*nl);
  cudaMalloc((void**)(&tmp1), sizeof(ScalarType)*nl);
  cudaMalloc((void**)(&tmp2), sizeof(ScalarType)*nl);
  
  cudaMemcpy(pg, grid, sizeof(ScalarType)*nl, cudaMemcpyHostToDevice);
  cudaMemcpy(pq1, q1, sizeof(ScalarType)*nl, cudaMemcpyHostToDevice);
  cudaMemcpy(pq2, q2, sizeof(ScalarType)*nl, cudaMemcpyHostToDevice);
  cudaMemcpy(pq3, q3, sizeof(ScalarType)*nl, cudaMemcpyHostToDevice);

  cudaTextureObject_t tex = gpuInitEmptyTexture(nx);
  float timer = 0;
  for (int i=0; i<10; ++i) {
    gpuInterp3D(pg, pq1, pq2, pq3, pe, tmp1, tmp2, nx, nl, tex, m_Opt->m_PDESolver.iporder, &timer);
    cudaDeviceSynchronize();
  }
  
  double error, max;
  error = 0;
  max = 0;
  
  ZeitGeist_define(INTERPOL);
  for (int i=0; i<100; ++i) {
    double err, m;
    ZeitGeist_tick(INTERPOL);
    gpuInterp3D(pg, pq1, pq2, pq3, pe, tmp1, tmp2, nx, nl, tex, m_Opt->m_PDESolver.iporder, &timer);
    //interp0(pg,pq1,pq2,pq3,pe,nx);
    cudaDeviceSynchronize();
    ZeitGeist_tock(INTERPOL);
    cudaMemcpy(eval, pe, sizeof(ScalarType)*nl, cudaMemcpyDeviceToHost);
    cudaDeviceSynchronize();
    TestError(ref, eval, nl, &err, &m);
    error += err;
    max += m;
  }
  
  error /= 100;
  max /= 100;
  
  printf("On Grid\n");
  std::cout << "\tthe interpolation has a RMS of " << error << std::endl;
  std::cout << "\tabs max of interpolation is " << max << std::endl;
  
  for (int i1 = 0; i1 < isize[0]; ++i1) {  // x1
      for (int i2 = 0; i2 < isize[1]; ++i2) {  // x2
        for (int i3 = 0; i3 < isize[2]; ++i3) {  // x3
          // compute linear / flat index
          int i = reg::GetLinearIndex(i1, i2, i3, m_Opt->m_Domain.isize);
          
          double x1 = hx[0]*static_cast<double>(i1 + istart[0]);
          double x2 = hx[1]*static_cast<double>(i2 + istart[1]);
          double x3 = hx[2]*static_cast<double>(i3 + istart[2]);
      
          x1 += hx[0]*static_cast<double>(rand()-RAND_MAX/2)/static_cast<double>(RAND_MAX/2);
          x2 += hx[1]*static_cast<double>(rand()-RAND_MAX/2)/static_cast<double>(RAND_MAX/2);
          x3 += hx[2]*static_cast<double>(rand()-RAND_MAX/2)/static_cast<double>(RAND_MAX/2);
          //double x1 = static_cast<double>(rand())*2.*M_PI/static_cast<double>(RAND_MAX);
          //double x2 = static_cast<double>(rand())*2.*M_PI/static_cast<double>(RAND_MAX);
          //double x3 = static_cast<double>(rand())*2.*M_PI/static_cast<double>(RAND_MAX);
          
          if (x1 > 2.*M_PI) x1 -= 2.*M_PI;
          if (x2 > 2.*M_PI) x2 -= 2.*M_PI;
          if (x3 > 2.*M_PI) x3 -= 2.*M_PI;
          if (x1 < 0.) x1 += 2.*M_PI;
          if (x2 < 0.) x2 += 2.*M_PI;
          if (x3 < 0.) x3 += 2.*M_PI;
          
          TestFunction(ref[i], x1, x2, x3);
          
          q1[i] = x1*nx[0]/(2.*M_PI);
          q2[i] = x2*nx[1]/(2.*M_PI);
          q3[i] = x3*nx[2]/(2.*M_PI);
        }  // i1
      }  // i2
    }  // i3
  cudaMemcpy(pq1, q1, sizeof(ScalarType)*nl, cudaMemcpyHostToDevice);
  cudaMemcpy(pq2, q2, sizeof(ScalarType)*nl, cudaMemcpyHostToDevice);
  cudaMemcpy(pq3, q3, sizeof(ScalarType)*nl, cudaMemcpyHostToDevice);
  
  for (int i=0; i<10; ++i) {
    gpuInterp3D(pg, pq1, pq2, pq3, pe, tmp1, tmp2, nx, nl, tex, m_Opt->m_PDESolver.iporder, &timer);
    cudaDeviceSynchronize();
  }
  
  error = 0;
  max = 0;
  
  ZeitGeist_define(INTERPOL_RAND);
  for (int i=0; i<100; ++i) {
    double err, m;
    ZeitGeist_tick(INTERPOL_RAND);
    gpuInterp3D(pg, pq1, pq2, pq3, pe, tmp1, tmp2, nx, nl, tex, m_Opt->m_PDESolver.iporder, &timer);
    //interp0(pg,pq1,pq2,pq3,pe,nx);
    cudaDeviceSynchronize();
    ZeitGeist_tock(INTERPOL_RAND);
    cudaMemcpy(eval, pe, sizeof(ScalarType)*nl, cudaMemcpyDeviceToHost);
    cudaDeviceSynchronize();
    TestError(ref, eval, nl, &err, &m);
    error += err;
    max += m;
  }
  
  error /= 100;
  max /= 100;
  
  printf("Random Eval\n");
  std::cout << "\tthe interpolation has a RMS of " << error << std::endl;
  std::cout << "\tabs max of interpolation is " << max << std::endl;

  cudaFree(pg);
  cudaFree(q1);
  cudaFree(q2);
  cudaFree(q3);
  cudaFree(pe);
  cudaFree(tmp1);
  cudaFree(tmp2);
  cudaDestroyTextureObject(tex);
#endif
#else
  std::cout << "unit test not implemented" << std::endl;
#endif

  printf("Timings\n");
  for (auto zg : ZeitGeist::zgMap()) {
      char txt[120];
      sprintf(txt, "  %16s: %5lix, %10lf s",zg.first.c_str(), zg.second.Count(), zg.second.Total_s());
      reg::Msg(txt);
  }
  
  delete[] grid;
  delete[] q;
  delete[] ref;
  delete[] eval;
  
  PetscFunctionReturn(ierr);
}

/********************************************************************************************************************/
PetscErrorCode TestInterpolationMultiGPU(RegOpt *m_Opt) {
    PetscErrorCode ierr = 0;
    PetscFunctionBegin;
  
  
  printOnMaster("starting interpolation unit test\n");
  
  srand(time(0));
  
  int nx[3], istart[3], isize[3];
  int nl = 1, ng = 1;
  ScalarType hx[3], scale;
  double error=0, max=0;
  ScalarType m_GPUtime;
  int nghost = 3;
  int isize_g[3], istart_g[3];
  int nlghost = 1;
 
  //ierr = PetscOptionsSetValue(NULL,"-cuda_view","true"); CHKERRQ(ierr);
  
  Vec q = NULL; ScalarType* p_q; // query points
  Vec f = NULL; ScalarType* p_f; // on grid function
  Vec fout = NULL; ScalarType *p_fout;  // output function values 
  Interp3_Plan_GPU* interp_plan = NULL;
  ScalarType* p_fghost = NULL;  // ghost padded data

  for (int i = 0; i < 3; ++i) {
    nx[i]     = m_Opt->m_Domain.nx[i];
    isize[i]  = m_Opt->m_Domain.isize[i];
    istart[i] = m_Opt->m_Domain.istart[i];
    hx[i]     = m_Opt->m_Domain.hx[i];
    nl       *= isize[i];
    ng       *= nx[i];
  }

  ScalarType *ref  = new ScalarType[nl];
  ierr = VecCreate(q, 3*nl, 3*ng); CHKERRQ(ierr);
  ierr = VecCreate(f, nl, ng);     CHKERRQ(ierr);
  ierr = VecCreate(fout, nl, ng);  CHKERRQ(ierr);
 
  ierr = VecGetArray(q, &p_q); CHKERRQ(ierr);
  ierr = VecGetArray(f, &p_f); CHKERRQ(ierr);

  for (int i1 = 0; i1 < isize[0]; ++i1) {  // x1
    for (int i2 = 0; i2 < isize[1]; ++i2) {  // x2
      for (int i3 = 0; i3 < isize[2]; ++i3) {  // x3
        // compute coordinates (nodal grid)
        double x1 = hx[0]*static_cast<double>(i1 + istart[0]);
        double x2 = hx[1]*static_cast<double>(i2 + istart[1]);
        double x3 = hx[2]*static_cast<double>(i3 + istart[2]);

        // compute linear / flat index
        int i = reg::GetLinearIndex(i1, i2, i3, m_Opt->m_Domain.isize);
        
        TestFunction(p_f[i], x1, x2, x3);
        
        x1 += hx[0]*1.5;
        x2 += hx[1]*0.5;
        x3 += hx[2]*0.1;
        
       // x1 += hx[0]*static_cast<double>(rand()-RAND_MAX/2)/static_cast<double>(RAND_MAX/2);
       // x2 += hx[1]*static_cast<double>(rand()-RAND_MAX/2)/static_cast<double>(RAND_MAX/2);
       // x3 += hx[2]*static_cast<double>(rand()-RAND_MAX/2)/static_cast<double>(RAND_MAX/2);
        
        if (x1 > 2.*M_PI) x1 -= 2.*M_PI;
        if (x2 > 2.*M_PI) x2 -= 2.*M_PI;
        if (x3 > 2.*M_PI) x3 -= 2.*M_PI;
    
        //x1 = static_cast<double>(rand())*2.*M_PI/static_cast<double>(RAND_MAX);
        //x2 = static_cast<double>(rand())*2.*M_PI/static_cast<double>(RAND_MAX);
        //x3 = static_cast<double>(rand())*2.*M_PI/static_cast<double>(RAND_MAX);
        
        TestFunction(ref[i], x1, x2, x3);
         
        // scaling by 2*pi is needed by the scatter function
        p_q[3*i + 0] = x1/(2.*M_PI);
        p_q[3*i + 1] = x2/(2.*M_PI);
        p_q[3*i + 2] = x3/(2.*M_PI);
      }  // i1
    }  // i2
  }  // i3
    
    ierr = VecRestoreArray(f, &p_f); CHKERRQ(ierr);
    ierr = VecRestoreArray(q, &p_q); CHKERRQ(ierr);

    // get ghost dimensions
    size_t g_alloc_max = ghost_xyz_local_size(m_Opt, nghost, isize_g, istart_g);
    for (int i=0; i<3; ++i) {
        nlghost *= isize_g[i];
    }
    
  cudaTextureObject_t tex = gpuInitEmptyTexture(isize_g);   
  
  int dofs[2] = {1, 3};
  if (interp_plan == NULL){
      
      try{ interp_plan = new Interp3_Plan_GPU(g_alloc_max); }
      catch (std::bad_alloc&){
          ierr=reg::ThrowError("allocation failed"); CHKERRQ(ierr);
      }
      interp_plan->allocate(nl,dofs,2);
  }
 
  double timers[4] = {0,0,0,0};
  ierr = VecGetArray(q, &p_q); CHKERRQ(ierr);
  interp_plan->scatter(nx, isize, istart, nl, nghost, p_q, m_Opt->m_CartGridDims, m_Opt->m_Domain.mpicomm, timers);
  ierr = VecRestoreArray(q, &p_q); CHKERRQ(ierr);
    
  p_fghost = reinterpret_cast<ScalarType*>(accfft_alloc(g_alloc_max));
  ierr = VecGetArray(f, &p_f);  CHKERRQ(ierr);
  get_ghost_xyz(m_Opt, nghost, isize_g, p_f, p_fghost); 
  ierr = VecRestoreArray(f, &p_f); CHKERRQ(ierr);

  ScalarType* m_tmpInterpol1 = NULL, *m_tmpInterpol2 = NULL;


  ierr = VecGetArray(fout, &p_fout); CHKERRQ(ierr);
    
  interp_plan->interpolate( p_fghost, 
                            nx,
                            isize,
                            istart,
                            isize_g, 
                            nlghost,
                            nl, 
                            nghost, 
                            p_fout,
                            m_Opt->m_CartGridDims,
                            m_Opt->m_Domain.mpicomm, 
                            timers, 
                            m_tmpInterpol1, 
                            m_tmpInterpol2, 
                            tex, 
                            3, 
                            &m_GPUtime, 0);
    
  TestError(ref, p_fout, nl, &error, &max);

  ierr = VecRestoreArray(fout, &p_fout); CHKERRQ(ierr);
  
  double global_error;
  double global_max;
  MPI_Reduce(&error, &global_error, 1, MPI_DOUBLE, MPI_MAX, 0, PETSC_COMM_WORLD);
  MPI_Reduce(&max, &global_max, 1, MPI_DOUBLE, MPI_MAX, 0, PETSC_COMM_WORLD);

  printOnMaster("interp error = " + std::to_string(global_error) + "\n" + "max val = " + std::to_string(global_max) + "\n");

  cudaDestroyTextureObject(tex);
  ierr = VecDestroy(&q); CHKERRQ(ierr); q = NULL;
  ierr = VecDestroy(&f); CHKERRQ(ierr); f = NULL;
  ierr = VecDestroy(&fout); CHKERRQ(ierr); fout = NULL;
  
  delete[] ref;
  
  if (interp_plan != NULL) {
    delete interp_plan;
    interp_plan = NULL;
  }

  if (p_fghost != NULL) {
    accfft_free(p_fghost); 
    p_fghost = NULL;
  }

  PetscFunctionReturn(ierr);

}


} // namespace UnitTest


} // namespace reg

#endif // _TESTINTERPOLATION_CPP_
