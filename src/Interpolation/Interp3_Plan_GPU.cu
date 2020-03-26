#include <interp3_gpu_mpi.hpp>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <cuda.h>
#include <cufft.h>
#include <cuda_runtime_api.h>
#include <cmath>

#include <algorithm>

//#define VERBOSE1

#ifndef ACCFFT_CHECKCUDA_H
#define ACCFFT_CHECKCUDA_H
inline cudaError_t checkCuda_accfft(cudaError_t result)
{
#if defined(DEBUG) || defined(_DEBUG)
  if (result != cudaSuccess) {
    fprintf(stderr, "CUDA Runtime Error: %s\n", cudaGetErrorString(result));
    assert(result == cudaSuccess);
  }
#endif
  return result;
}
inline cufftResult checkCuda_accfft(cufftResult result)
{
#if defined(DEBUG) || defined(_DEBUG)
  if (result != CUFFT_SUCCESS) {
    fprintf(stderr, "CUDA Runtime Error: %s\n", result);
    assert(result == CUFFT_SUCCESS);
  }
#endif
  return result;
}
#endif


struct is_equal {
    int id;
    is_equal(int comp_id) : id(comp_id) {};
    __host__ __device__ 
    bool operator()(const int &x) {
        return x == id;
    }
};

template <typename Iterator>
class strided_range
{
    public:

    typedef typename thrust::iterator_difference<Iterator>::type difference_type;

    struct stride_functor : public thrust::unary_function<difference_type,difference_type>
    {
        difference_type stride;

        stride_functor(difference_type stride)
            : stride(stride) {}

        __host__ __device__
        difference_type operator()(const difference_type& i) const
        { 
            return stride * i;
        }
    };

    typedef typename thrust::counting_iterator<difference_type>                   CountingIterator;
    typedef typename thrust::transform_iterator<stride_functor, CountingIterator> TransformIterator;
    typedef typename thrust::permutation_iterator<Iterator,TransformIterator>     PermutationIterator;

    // type of the strided_range iterator
    typedef PermutationIterator iterator;

    // construct strided_range for the range [first,last)
    strided_range(Iterator first, Iterator last, difference_type stride)
        : first(first), last(last), stride(stride) {}
   
    iterator begin(void) const
    {
        return PermutationIterator(first, TransformIterator(CountingIterator(0), stride_functor(stride)));
    }

    iterator end(void) const
    {
        return begin() + ((last - first) + (stride - 1)) / stride;
    }
    
    protected:
    Iterator first;
    Iterator last;
    difference_type stride;
};

class Trip_GPU{
  public:
    Trip_GPU(){};
    double x;
    double y;
    double z;
    int ind;
    int N[3];
    double h[3];

};

static bool ValueCmp(Trip_GPU const & a, Trip_GPU const & b)
{
    return a.z + a.y/a.h[1]*a.N[2] + a.x/a.h[0]* a.N[1]*a.N[2]<b.z + b.y/b.h[1]*b.N[2] + b.x/b.h[0]* b.N[1]*b.N[2] ;
}

#ifdef SORT_QUERIES
static void sort_queries(std::vector<Real>* query_outside,std::vector<int>* f_index,int* N_reg,Real* h,MPI_Comm c_comm){

  int nprocs, procid;
  MPI_Comm_rank(c_comm, &procid);
  MPI_Comm_size(c_comm, &nprocs);
  for(int proc=0;proc<nprocs;++proc){
    int qsize=query_outside[proc].size()/COORD_DIM;
    Trip_GPU* trip=new Trip_GPU[qsize];

    for(int i=0;i<qsize;++i){
      trip[i].x=query_outside[proc][i*COORD_DIM+0];
      trip[i].y=query_outside[proc][i*COORD_DIM+1];
      trip[i].z=query_outside[proc][i*COORD_DIM+2];
      trip[i].ind=f_index[proc][i];
      trip[i].N[0]=N_reg[0];
      trip[i].N[1]=N_reg[1];
      trip[i].N[2]=N_reg[2];
      trip[i].h[0]=h[0];
      trip[i].h[1]=h[1];
      trip[i].h[2]=h[2];
    }

    std::sort(trip, trip + qsize, ValueCmp);

    query_outside[proc].clear();
    f_index[proc].clear();

    for(int i=0;i<qsize;++i){
      query_outside[proc].push_back(trip[i].x);
      query_outside[proc].push_back(trip[i].y);
      query_outside[proc].push_back(trip[i].z);
      f_index[proc].push_back(trip[i].ind);
    }
    delete[] trip;
  }
  return;
}
#endif


Interp3_Plan_GPU::Interp3_Plan_GPU (size_t g_alloc_max) {
  this->g_alloc_max=g_alloc_max;
  this->allocate_baked=false;
  this->scatter_baked=false;
}


void Interp3_Plan_GPU::allocate (int N_pts, int data_dof)
{
  int nprocs, procid;
  MPI_Comm_rank(MPI_COMM_WORLD, &procid);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  query_points=(Real*) malloc(N_pts*COORD_DIM*sizeof(Real));

  f_index_procs_others_offset=(int*)malloc(nprocs*sizeof(int)); // offset in the all_query_points array
  f_index_procs_self_offset  =(int*)malloc(nprocs*sizeof(int)); // offset in the query_outside array
  f_index_procs_self_sizes   =(int*)malloc(nprocs*sizeof(int)); // sizes of the number of interpolations that need to be sent to procs
  f_index_procs_others_sizes =(int*)malloc(nprocs*sizeof(int)); // sizes of the number of interpolations that need to be received from procs

  s_request= new MPI_Request[nprocs];
  request= new MPI_Request[nprocs];
    
  f_index = new thrust::device_vector<int> [nprocs];
  query_outside=new thrust::device_vector<Real> [nprocs];
    
  // on CPU, N_pts = nl (number of local points), data_dof = 1 (Scalar field) / 3 (vector field)
  // The reshuffled semi-final interpolated values are stored here
  //f_cubic_unordered=(Real*) malloc(N_pts*sizeof(Real)*data_dof); 
  cudaMalloc((void**)&f_cubic_unordered_d, N_pts*sizeof(Real)*data_dof);


  //double time=0;
  //time=-MPI_Wtime();

// Allocate memory for the ghost padded regular grid values
//#ifdef INTERP_PINNED
//  //cudaMallocHost((void**)&this->ghost_reg_grid_vals_d,g_alloc_max*data_dof);
//  cudaMalloc((void**)&this->ghost_reg_grid_vals_d, g_alloc_max*data_dof);
//#else
//  cudaMalloc((void**)&this->ghost_reg_grid_vals_d, g_alloc_max*data_dof);
//#endif

  //time+=MPI_Wtime();
  //if(procid==0)
  //  std::cout<<"malloc time="<<time<<std::endl;

  stype= new MPI_Datatype[nprocs];
  rtype= new MPI_Datatype[nprocs];
  this->data_dof=data_dof;
  this->allocate_baked=true;
}

Interp3_Plan_GPU::~Interp3_Plan_GPU ()
{
  int nprocs, procid;
  MPI_Comm_rank(MPI_COMM_WORLD, &procid);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  if(this->allocate_baked){
   // free(query_points);

    free(f_index_procs_others_offset);
    free(f_index_procs_self_offset  );
    free(f_index_procs_self_sizes   );
    free(f_index_procs_others_sizes );

    delete(s_request);
    delete(request);
    //vectors
    for(int proc=0;proc<nprocs;++proc)
    {
      thrust::device_vector<int>().swap(f_index[proc]);
      thrust::device_vector<Real>().swap(query_outside[proc]);
    }

    //free(f_cubic_unordered);
    cudaFree(f_cubic_unordered_d);

  }

  if(this->scatter_baked) {
    //free(all_query_points);

#ifdef INTERP_PINNED
    cudaFreeHost(all_f_cubic_d);
    cudaFreeHost(xq1);
    cudaFreeHost(xq2);
    cudaFreeHost(xq3);
    cudaFreeHost(all_query_points_d);
#else
    cudaFree(all_f_cubic_d);
    cudaFree(xq1);
    cudaFree(xq2);
    cudaFree(xq3);
    cudaFree(all_query_points_d);
#endif

    for(int i=0;i<nprocs;++i) {
      MPI_Type_free(&stype[i]);
      MPI_Type_free(&rtype[i]);
    }

  }

  if(this->allocate_baked) {
    delete(stype);
    delete(rtype);
  }
  return;
}

void rescale_xyz(const int g_size,  int* N_reg, int* N_reg_g, int* istart, const int N_pts, Real* query_points);


/*
 * Phase 1 of the parallel interpolation: This function computes which query_points needs to be sent to
 * other processors and which ones can be interpolated locally. Then a sparse alltoall is performed and
 * all the necessary information is sent/received including the coordinates for the query_points.
 * At the end, each process has the coordinates for interpolation of its own data and those of the others.
 *
 * IMPORTANT: This function must be called just once for a specific query_points. The reason is because of the
 * optimizations performed which assumes that the query_points do not change. For repeated interpolation you should
 * just call this function once, and instead repeatedly call Interp3_Plan::interpolate function.
 */
void Interp3_Plan_GPU::scatter( int data_dof,
                                int* N_reg,  // global grid dimensions
                                int * isize, // local grid dimensions
                                int* istart, // local grid start indices
                                const int N_pts, // local grid point count
                                const int g_size, // ghost layer width
                                Real* query_points_in_x, // input query points
                                Real* query_points_in_y, // input query points
                                Real* query_points_in_z, // input query points
                                int* c_dims,  // process cartesian grid dimensions
                                MPI_Comm c_comm,  // MPI Comm
                                double * timings) 
{
  int nprocs, procid;
  MPI_Comm_rank(c_comm, &procid);
  MPI_Comm_size(c_comm, &nprocs);

  if(this->allocate_baked==false){
    std::cout<<"ERROR Interp3_Plan_GPU Scatter called before calling allocate.\n";
    return;
  }
  if(this->scatter_baked==true) {
    for(int proc=0;proc<nprocs;++proc) {
      thrust::device_vector<int>().swap(f_index[proc]);
      thrust::device_vector<Real>().swap(query_outside[proc]);
    }
  }
  all_query_points_allocation=0;
 {
    //int N_reg_g[3], isize_g[3];
    N_reg_g[0]=N_reg[0]+2*g_size;
    N_reg_g[1]=N_reg[1]+2*g_size;
    N_reg_g[2]=N_reg[2]+2*g_size;

    isize_g[0]=isize[0]+2*g_size;
    isize_g[1]=isize[1]+2*g_size;
    isize_g[2]=isize[2]+2*g_size;

    Real h[3]; // original grid size along each axis
    h[0]=1./N_reg[0];
    h[1]=1./N_reg[1];
    h[2]=1./N_reg[2];
    
    // We copy query_points_in to query_points to aviod overwriting the input coordinates
    // query_points is a temporary pointer which will be freed at the end of this routine
    //Real* query_points=(Real*) malloc(N_pts*COORD_DIM*sizeof(Real));
    //memcpy(query_points,query_points_in,N_pts*COORD_DIM*sizeof(Real));
    
    Real *query_points_x, *query_points_y, *query_points_z;
    cudaMalloc((void**)&query_points_x, sizeof(Real)*N_pts);
    cudaMalloc((void**)&query_points_y, sizeof(Real)*N_pts);
    cudaMalloc((void**)&query_points_z, sizeof(Real)*N_pts);

    cudaMemcpy(query_points_x, query_points_in_x, sizeof(Real)*N_pts, cudaMemcpyDeviceToDevice);
    cudaMemcpy(query_points_y, query_points_in_y, sizeof(Real)*N_pts, cudaMemcpyDeviceToDevice);
    cudaMemcpy(query_points_z, query_points_in_z, sizeof(Real)*N_pts, cudaMemcpyDeviceToDevice);

  
  //for (int i=0; i < N_pts; i++) {
  //      printf("%d,%0.2f,%0.2f,%0.2f\n", i, query_points[i*3+0], query_points[i*3+1], query_points[i*3+2]);
  //}
    
    ZeitGeist_define(scatter_enforce_periodicity_kernel);
    ZeitGeist_tick(scatter_enforce_periodicity_kernel);
    // Enforce periodicity // write kernel for this
    timings[3]+=-MPI_Wtime();
    enforcePeriodicity(query_points_x, query_points_y, query_points_z, h, N_pts);
    timings[3]+=+MPI_Wtime();
    ZeitGeist_tock(scatter_enforce_periodicity_kernel);
    
    //for(int i=0;i<N_pts;i++){
    //  while(query_points[i*COORD_DIM+0]<=-h[0]) {query_points[i*COORD_DIM+0]=query_points[i*COORD_DIM+0]+1;}
    //  while(query_points[i*COORD_DIM+1]<=-h[1]) {query_points[i*COORD_DIM+1]=query_points[i*COORD_DIM+1]+1;}
    //  while(query_points[i*COORD_DIM+2]<=-h[2]) {query_points[i*COORD_DIM+2]=query_points[i*COORD_DIM+2]+1;}

    //  while(query_points[i*COORD_DIM+0]>=1) {query_points[i*COORD_DIM+0]=query_points[i*COORD_DIM+0]-1;}
    //  while(query_points[i*COORD_DIM+1]>=1) {query_points[i*COORD_DIM+1]=query_points[i*COORD_DIM+1]-1;}
    //  while(query_points[i*COORD_DIM+2]>=1) {query_points[i*COORD_DIM+2]=query_points[i*COORD_DIM+2]-1;}
    //} 
    thrust::device_ptr<ScalarType> query_points_x_ptr = thrust::device_pointer_cast<ScalarType>(query_points_x);
    thrust::device_ptr<ScalarType> query_points_y_ptr = thrust::device_pointer_cast<ScalarType>(query_points_y);
    thrust::device_ptr<ScalarType> query_points_z_ptr = thrust::device_pointer_cast<ScalarType>(query_points_z);

    // Compute the start and end coordinates that this processor owns
    Real iX0[3],iX1[3];
    for (int j=0;j<3;j++) {
      iX0[j]=istart[j]*h[j];
      iX1[j]=iX0[j]+(isize[j]-1)*h[j];
    }

    // Now march through the query points and split them into nprocs parts.
    // These are stored in query_outside which is an array of vectors of size nprocs.
    // That is query_outside[i] is a vector that contains the query points that need to
    // be sent to process i. Obviously for the case of query_outside[procid], we do not
    // need to send it to any other processor, as we own the necessary information locally,
    // and interpolation can be done locally.


    // This is needed for one-to-one correspondence with output f. This is becaues we are reshuffling
    // the data according to which processor it land onto, and we need to somehow keep the original
    // index to write the interpolation data back to the right location in the output.

    // This is necessary because when we want to compute dproc0 and dproc1 we have to divide by
    // the max isize. If the proc grid is unbalanced, the last proc's isize will be different
    // than others. With this approach we always use the right isize0 for all procs.
    int isize0=std::ceil(N_reg[0]*1./c_dims[0]);
    int isize1=std::ceil(N_reg[1]*1./c_dims[1]);
    //for(int i=0;i<N_pts;i++){
    //  // The if condition check whether the query points fall into the locally owned domain or not
    //  if(
    //      iX0[0]-h[0]<=query_points[i*COORD_DIM+0] && query_points[i*COORD_DIM+0]<=iX1[0]+h[0] &&
    //      iX0[1]-h[1]<=query_points[i*COORD_DIM+1] && query_points[i*COORD_DIM+1]<=iX1[1]+h[1] &&
    //      iX0[2]-h[2]<=query_points[i*COORD_DIM+2] && query_points[i*COORD_DIM+2]<=iX1[2]+h[2]
    //    ){
    //    query_outside[procid].push_back(query_points[i*COORD_DIM+0]);
    //    query_outside[procid].push_back(query_points[i*COORD_DIM+1]);
    //    query_outside[procid].push_back(query_points[i*COORD_DIM+2]);
    //    f_index[procid].push_back(i);
    //    Q_local++;
    //    //PCOUT<<"j=0 else ---------- i="<<i<<std::endl;
    //    continue;
    //  }
    //  else{
    //    // If the point does not reside in the processor's domain then we have to
    //    // first compute which processor owns the point. After computing that
    //    // we add the query point to the corresponding vector.
    //    int dproc0=(int)(query_points[i*COORD_DIM+0]/h[0])/isize0;
    //    int dproc1=(int)(query_points[i*COORD_DIM+1]/h[1])/isize1;
    //    int proc=dproc0*c_dims[1]+dproc1; // Compute which proc has to do the interpolation
    //    //if(proc>=nprocs) std::cout<<"dp0="<<dproc0<<" dp1="<<dproc1<<" proc="<<proc<<" q[0]="<<query_points[i*COORD_DIM+0]<<" h[0]="<<h[0]<< " div="<<(query_points[i*COORD_DIM+0]/h[0])<<" "<<isize[0]<<std::endl;
    //    //PCOUT<<"proc="<<proc<<std::endl;
    //    query_outside[proc].push_back(query_points[i*COORD_DIM+0]);
    //    query_outside[proc].push_back(query_points[i*COORD_DIM+1]);
    //    query_outside[proc].push_back(query_points[i*COORD_DIM+2]);
    //    f_index[proc].push_back(i);
    //    Q_outside++;
    //    //PCOUT<<"j=0 else ---------- i="<<i<<std::endl;
    //    continue;
    //  }

    //}
    
    // number of coordinates to be sent to each proc
    int coords_in_proc;
    typedef thrust::device_vector<ScalarType>::iterator Iterator;
    // which_proc is an array denoting which point each processor belongs to
    int* which_proc;
    cudaMalloc((void**)&which_proc, sizeof(int)*N_pts);
    
    ZeitGeist_define(scatter_check_domain_kernel);
    ZeitGeist_tick(scatter_check_domain_kernel);
    timings[3]+=-MPI_Wtime();
    checkDomain(which_proc, query_points_x, query_points_y, query_points_z, iX0, iX1, h, N_pts, procid, isize0, isize1, c_dims[1]);
    ZeitGeist_tock(scatter_check_domain_kernel);

    thrust::device_ptr<int> which_proc_ptr = thrust::device_pointer_cast<int>(which_proc);
    
    ZeitGeist_define(query_memalloc);
    ZeitGeist_define(query_copy_if);
    ZeitGeist_define(query_get_count);


    ZeitGeist_define(scatter_create_mpi_buffer);
    ZeitGeist_tick(scatter_create_mpi_buffer);
    // loop over all procs
    for (int proc=0; proc<nprocs; ++proc) {
        // count how many points belong to proc, will be useful in memory allocation
        ZeitGeist_tick(query_get_count);
        get_count(which_proc, N_pts, proc, &coords_in_proc);
        ZeitGeist_tock(query_get_count);
        
        if (coords_in_proc > 0) {
            // allocate the required memory for f_index[proc]
            ZeitGeist_tick(query_memalloc);
            f_index[proc].reserve(coords_in_proc);
            f_index[proc].resize(coords_in_proc);
            ZeitGeist_tock(query_memalloc);
            // get indices of coordinates which belong to this proc and store in f_index[proc]

            ZeitGeist_tick(query_copy_if);
            thrust::copy_if(thrust::device, thrust::make_counting_iterator(0), thrust::make_counting_iterator(N_pts), which_proc_ptr, f_index[proc].begin(), is_equal(proc));
            ZeitGeist_tock(query_copy_if);
          
            ZeitGeist_tick(query_memalloc);
            query_outside[proc].reserve(3*coords_in_proc);
            query_outside[proc].resize(3*coords_in_proc);
            ZeitGeist_tock(query_memalloc);

            ZeitGeist_tick(query_copy_if);
            strided_range<Iterator> strided_x(query_outside[proc].begin(),   query_outside[proc].end() + coords_in_proc+1-COORD_DIM,   COORD_DIM);
            strided_range<Iterator> strided_y(query_outside[proc].begin()+1, query_outside[proc].end() + coords_in_proc+1-COORD_DIM+1, COORD_DIM);
            strided_range<Iterator> strided_z(query_outside[proc].begin()+2, query_outside[proc].end() + coords_in_proc+1-COORD_DIM+2, COORD_DIM);
            thrust::copy_if(thrust::device, query_points_x_ptr, query_points_x_ptr+N_pts, which_proc_ptr, strided_x.begin(), is_equal(proc));
            thrust::copy_if(thrust::device, query_points_y_ptr, query_points_y_ptr+N_pts, which_proc_ptr, strided_y.begin(), is_equal(proc));
            thrust::copy_if(thrust::device, query_points_z_ptr, query_points_z_ptr+N_pts, which_proc_ptr, strided_z.begin(), is_equal(proc));
            ZeitGeist_tock(query_copy_if);
        }
    }
    timings[3]+=+MPI_Wtime();
    ZeitGeist_tock(scatter_create_mpi_buffer);
    

    // Now sort the query points in zyx order
//#ifdef SORT_QUERIES
//    timings[3]+=-MPI_Wtime();
//    sort_queries(query_outside,f_index,N_reg,h,c_comm);
//    timings[3]+=+MPI_Wtime();
//    //if(procid==0) std::cout<<"Sorting Queries\n";
//#endif

    // Now we need to send the query_points that land onto other processor's domain.
    // This done using a sparse alltoallv.
    // Right now each process knows how much data to send to others, but does not know
    // how much data it should receive. This is a necessary information both for the MPI
    // command as well as memory allocation for received data.
    // So we first do an alltoall to get the f_index[proc].size from all processes.

    for (int proc=0;proc<nprocs;proc++) {
      if(!f_index[proc].empty())
        f_index_procs_self_sizes[proc]=f_index[proc].size();
      else
        f_index_procs_self_sizes[proc]=0;
    }
    ZeitGeist_define(scatter_comm_query_size);
    ZeitGeist_tick(scatter_comm_query_size);
    timings[0]+=-MPI_Wtime();
    MPI_Alltoall(f_index_procs_self_sizes,1, MPI_INT,
        f_index_procs_others_sizes,1, MPI_INT,
        c_comm);
    timings[0]+=+MPI_Wtime();
    ZeitGeist_tock(scatter_comm_query_size);


    // Now we need to allocate memory for the receiving buffer of all query
    // points including ours. This is simply done by looping through
    // f_index_procs_others_sizes and adding up all the sizes.
    // Note that we would also need to know the offsets.
    f_index_procs_others_offset[0]=0;
    f_index_procs_self_offset[0]=0;
    for (int proc=0;proc<nprocs;++proc) {
      // The reason we multiply by COORD_DIM is that we have three coordinates per interpolation request
      all_query_points_allocation+=f_index_procs_others_sizes[proc]*COORD_DIM;
      if(proc>0){
        f_index_procs_others_offset[proc]=f_index_procs_others_offset[proc-1]+f_index_procs_others_sizes[proc-1];
        f_index_procs_self_offset[proc]=f_index_procs_self_offset[proc-1]+f_index_procs_self_sizes[proc-1];
      }
    }
    total_query_points=all_query_points_allocation/COORD_DIM;

    // This if condition is to allow multiple calls to scatter fucntion with different query points
    // without having to create a new plan
    //if(this->scatter_baked==true){
    //  free(this->all_query_points);
    //  free(this->all_f_cubic);
    //}
    
    //all_query_points_d=(Real*) malloc(all_query_points_allocation*sizeof(Real));
    //all_f_cubic=(Real*)malloc(total_query_points*sizeof(Real)*data_dof);

  // This if condition is to allow multiple calls to scatter fucntion with different query points
  // without having to create a new plan
  if(this->scatter_baked==true) {
#ifdef INTERP_PINNED
    cudaFreeHost(this->all_query_points_d);
    cudaFreeHost(this->xq1);
    cudaFreeHost(this->xq2);
    cudaFreeHost(this->xq3);
    cudaFreeHost(this->all_f_cubic_d);
    cudaMallocHost((void**)&all_query_points_d,all_query_points_allocation*sizeof(Real) );
    cudaMallocHost((void**)&xq1, total_query_points*sizeof(Real));
    cudaMallocHost((void**)&xq2, total_query_points*sizeof(Real));
    cudaMallocHost((void**)&xq3, total_query_points*sizeof(Real));
    cudaMallocHost((void**)&all_f_cubic_d, total_query_points*sizeof(Real)*data_dof);
#else
    // freeing the cuda memory is required everytime scatter is called because the distribution of query points might not be uniform across all GPUs
    cudaFree(this->all_query_points_d);
    cudaFree(this->xq1);
    cudaFree(this->xq2);
    cudaFree(this->xq3);
    cudaFree(this->all_f_cubic_d);
    cudaMalloc((void**)&all_f_cubic_d, total_query_points*sizeof(Real)*data_dof);
    cudaMalloc((void**)&xq1, total_query_points*sizeof(Real));
    cudaMalloc((void**)&xq2, total_query_points*sizeof(Real));
    cudaMalloc((void**)&xq3, total_query_points*sizeof(Real));
    cudaMalloc((void**)&all_query_points_d,all_query_points_allocation*sizeof(Real) );
#endif
  }
  else {
    // if this the first time scatter is being called (scatter_baked = False) then, only allocate the memory on GPU
#ifdef INTERP_PINNED
    cudaMallocHost((void**)&all_query_points_d,all_query_points_allocation*sizeof(Real) );
    cudaMallocHost((void**)&xq1, total_query_points*sizeof(Real));
    cudaMallocHost((void**)&xq2, total_query_points*sizeof(Real));
    cudaMallocHost((void**)&xq3, total_query_points*sizeof(Real));
    cudaMallocHost((void**)&all_f_cubic_d, total_query_points*sizeof(Real)*data_dof);
#else
    cudaMalloc((void**)&all_query_points_d,all_query_points_allocation*sizeof(Real) );
    cudaMalloc((void**)&xq1, total_query_points*sizeof(Real));
    cudaMalloc((void**)&xq2, total_query_points*sizeof(Real));
    cudaMalloc((void**)&xq3, total_query_points*sizeof(Real));
    cudaMalloc((void**)&all_f_cubic_d, total_query_points*sizeof(Real)*data_dof);
#endif
  }


    // Now perform the allotall to send/recv query_points
    ZeitGeist_define(scatter_comm_query_points_sendrcv);
    ZeitGeist_tick(scatter_comm_query_points_sendrcv);
    timings[0]+=-MPI_Wtime();
    int dst_r,dst_s;
    for (int i=0;i<nprocs;++i) {
      dst_r=i;//(procid+i)%nprocs;
      dst_s=i;//(procid-i+nprocs)%nprocs;
      s_request[dst_s]=MPI_REQUEST_NULL;
      request[dst_r]=MPI_REQUEST_NULL;
      int roffset=f_index_procs_others_offset[dst_r]*COORD_DIM; // notice that COORD_DIM is needed because query_points are 3 times f
      //int soffset=f_index_procs_self_offset[dst_s]*COORD_DIM;
      if(f_index_procs_others_sizes[dst_r]!=0) {
        MPI_Irecv(&all_query_points_d[roffset], f_index_procs_others_sizes[dst_r]*COORD_DIM,MPI_T, dst_r, 0, c_comm, &request[dst_r]);
      }

      if(!query_outside[dst_s].empty()) {
        ScalarType* src_ptr = thrust::raw_pointer_cast(query_outside[dst_s].data());
        MPI_Isend(src_ptr, f_index_procs_self_sizes[dst_s]*COORD_DIM, MPI_T, dst_s, 0, c_comm, &s_request[dst_s]);
      }
    }
    
    // Wait for all the communication to finish
    MPI_Status ierr;
    for (int proc=0;proc<nprocs;++proc) {
      if(request[proc]!=MPI_REQUEST_NULL)
        MPI_Wait(&request[proc], &ierr);
      if(s_request[proc]!=MPI_REQUEST_NULL)
        MPI_Wait(&s_request[proc], &ierr);
    }
    ZeitGeist_tock(scatter_comm_query_points_sendrcv);
    timings[0]+=+MPI_Wtime();
  

    // Now perform the interpolation on all query points including those that need to
    // be sent to other processors and store them into all_f_cubic
    //free(query_points);
    cudaFree(query_points_x);
    cudaFree(query_points_y);
    cudaFree(query_points_z);
  }

  for(int i=0;i<nprocs;++i){
    MPI_Type_vector(data_dof,f_index_procs_self_sizes[i],N_pts, MPI_T, &rtype[i]);
    MPI_Type_vector(data_dof,f_index_procs_others_sizes[i],total_query_points, MPI_T, &stype[i]);
    MPI_Type_commit(&stype[i]);
    MPI_Type_commit(&rtype[i]);
  }

  //rescale_xyz(g_size,N_reg,N_reg_g,istart,total_query_points,all_query_points); //SNAFU

  
//  for (int i=0; i < total_query_points; i++) {
//        printf("%d,%0.4f,%0.4f,%0.4f\n", i, all_query_points[i*3+0], all_query_points[i*3+1], all_query_points[i*3+2]);
//  }

  int proc_coord[2];
  proc_coord[0] = static_cast<int>(istart[0]/isize[0]);
  proc_coord[1] = static_cast<int>(istart[1]/isize[1]);

  // transfer query points "all_query_points" from host to device
  ZeitGeist_define(scatter_query_points_normalize_kernel);
  ZeitGeist_tick(scatter_query_points_normalize_kernel);
  timings[3]+=-MPI_Wtime();
  //cudaMemcpy(all_query_points_d,all_query_points,all_query_points_allocation*sizeof(Real),cudaMemcpyHostToDevice);
  //PetscSynchronizedPrintf(PETSC_COMM_WORLD, "[%d] total_query_points = %d, proc_i = %d, proc_j = %d\n", procid, total_query_points, proc_coord[0], proc_coord[1]);
  //PetscSynchronizedFlush(PETSC_COMM_WORLD, PETSC_STDOUT);
  normalizeQueryPoints(xq1, xq2, xq3, all_query_points_d, total_query_points, isize, N_reg, proc_coord, g_size);
  timings[3]+=+MPI_Wtime();
  ZeitGeist_tock(scatter_query_points_normalize_kernel);

  this->scatter_baked=true;
  return;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Phase 2 of the parallel interpolation: This function must be called after the scatter function is called.
 * It performs local interpolation for all the points that the processor has for itself, as well as the interpolations
 * that it has to send to other processors. After the local interpolation is performed, a sparse
 * alltoall is performed so that all the interpolated results are sent/received.
 *
 */

void Interp3_Plan_GPU::interpolate( Real* ghost_reg_grid_vals_d, // ghost padded regular grid values on GPU
                                    int data_dof,              // degree of freedom for data (vector field=3, scalarfield=1)
                                    int* N_reg,                // size of global grid points 
                                    int* isize,                // size of the local grid owned by the process
                                    int* istart,               // start point of the local grid owned by the process
                                    int* isize_g,              // size of the local grid (including ghost points)
                                    const int nlghost,         // number of local grid points (including ghost points) owned by process
                                    const int N_pts,           // number of local points owned by the process
                                    const int g_size,          // ghost layer width
                                    Real* query_values_d,      // interpolation result on GPU
                                    int* c_dims,               // dimensions of the communicator plan
                                    MPI_Comm c_comm,           // MPI communicator
                                    double * timings,          // time variable to store interpolation time
                                    float *tmp1,               // temporary memory for interpolation prefilter
                                    float* tmp2,               // temporary memory for interpolation prefilter
                                    cudaTextureObject_t yi_tex,// texture object for interpolation
                                    int iporder,               // interpolation order
                                    ScalarType* interp_time)   // interpolation time
{
  int nprocs, procid;
  MPI_Comm_rank(c_comm, &procid);
  MPI_Comm_size(c_comm, &nprocs);
  if(this->allocate_baked==false){
    std::cout<<"ERROR Interp3_Plan_GPU interpolate called before calling allocate.\n";
    return;
  }
  if(this->scatter_baked==false){
    std::cout<<"ERROR Interp3_Plan_GPU interpolate called before calling scatter.\n";
    return;
  }

  // copy the ghost padded regular grid values from Host to Device
  //timings[2]+=-MPI_Wtime();
  //cudaMemcpy(ghost_reg_grid_vals_d, ghost_reg_grid_vals, nlghost*data_dof*sizeof(Real), cudaMemcpyHostToDevice);
  //cudaMemcpy(ghost_reg_grid_vals_d, ghost_reg_grid_vals, nlghost*data_dof*sizeof(Real), cudaMemcpyDeviceToDevice);
  //cudaCheckLastError();
  //timings[2]+=+MPI_Wtime();

#if defined(VERBOSE1) 
  printf("\ng_alloc_max = %zu", g_alloc_max);
  printf("\ndata_dof = %d", data_dof);
  printf("\nisize_g[0] = %d", isize_g[0]);
  printf("\nisize_g[1] = %d", isize_g[1]);
  printf("\nisize_g[2] = %d", isize_g[2]);
  printf("\nipoder = %d", iporder);
  printf("\ntotal_query_points = %d", total_query_points);
  printf("\nnlghost = %d", nlghost);
#endif
    
  // compute the interpolation on the GPU
  ZeitGeist_define(interp_kernel);
  ZeitGeist_tick(interp_kernel);
  timings[1]+=-MPI_Wtime();
  if (data_dof == 3)
    gpuInterpVec3D(&ghost_reg_grid_vals_d[0*nlghost], 
                   &ghost_reg_grid_vals_d[1*nlghost], 
                   &ghost_reg_grid_vals_d[2*nlghost], 
                   xq1, xq2, xq3, 
                   &all_f_cubic_d[0*total_query_points], 
                   &all_f_cubic_d[1*total_query_points], 
                   &all_f_cubic_d[2*total_query_points], 
                   tmp1, tmp2, isize_g, static_cast<long int>(total_query_points), yi_tex, iporder, interp_time);
  else 
    gpuInterp3D(ghost_reg_grid_vals_d, 
                xq1, xq2, xq3, 
                all_f_cubic_d, 
                tmp1, tmp2, isize_g, static_cast<long int>(total_query_points), yi_tex, 
                iporder, interp_time);
  ZeitGeist_tock(interp_kernel);
  timings[1]+=+MPI_Wtime();
  
  PetscSynchronizedPrintf(PETSC_COMM_WORLD, "[%d] query points = %d\n", procid, total_query_points);
  PetscSynchronizedFlush(PETSC_COMM_WORLD, PETSC_STDOUT);

  //gpu_interp3_ghost_xyz_p(ghost_reg_grid_vals_d, data_dof, N_reg, isize,istart,total_query_points, g_size, all_query_points_d, all_f_cubic_d,true);
    
  // copy the interpolated results from the device to the host
  //timings[2]+=-MPI_Wtime();
  //cudaMemcpy(all_f_cubic,all_f_cubic_d, total_query_points*sizeof(Real)*data_dof ,cudaMemcpyDeviceToHost); // no need to do with cuda-aware mpi
  //cudaCheckLastError();
  //timings[2]+=+MPI_Wtime();

  // Now we have to do an alltoall to distribute the interpolated data from all_f_cubic_d to f_cubic_unordered_d
  ZeitGeist_define(interp_comm_values_sendrcv);
  ZeitGeist_tick(interp_comm_values_sendrcv);
  timings[0]+=-MPI_Wtime();
  int dst_r,dst_s;
  for (int i=0;i<nprocs;++i){
    dst_r=i;//(procid+i)%nprocs;
    dst_s=i;//(procid-i+nprocs)%nprocs;
    s_request[dst_s]=MPI_REQUEST_NULL;
    request[dst_r]=MPI_REQUEST_NULL;
    // Notice that this is the adjoint of the first comm part
    // because now you are sending others f and receiving your part of f
    int soffset=f_index_procs_others_offset[dst_r];
    int roffset=f_index_procs_self_offset[dst_s];
    if(f_index_procs_self_sizes[dst_r]!=0)
      MPI_Irecv(&f_cubic_unordered_d[roffset],1,rtype[i], dst_r,
          0, c_comm, &request[dst_r]); 
    if(f_index_procs_others_sizes[dst_s]!=0)
      MPI_Isend(&all_f_cubic_d[soffset],1,stype[i],dst_s,
          0, c_comm, &s_request[dst_s]);
  }

  MPI_Status ierr;
  for (int proc=0;proc<nprocs;++proc){
    if(request[proc]!=MPI_REQUEST_NULL)
      MPI_Wait(&request[proc], &ierr);
    if(s_request[proc]!=MPI_REQUEST_NULL)
      MPI_Wait(&s_request[proc], &ierr);
  }
  
  ZeitGeist_tock(interp_comm_values_sendrcv);
  timings[0]+=+MPI_Wtime();

  
  timings[3]+=-MPI_Wtime();
  ZeitGeist_define(interp_values_copy_kernel);
  ZeitGeist_tick(interp_values_copy_kernel);
  int* f_index_ptr;
  // Now copy back f_cubic_unordered_d to query_values_d in the correct f_index
  for(int dof=0;dof<data_dof;++dof) {
    for(int proc=0;proc<nprocs;++proc) {
      if(!f_index[proc].empty()) {
          //for (int i=0; i<f_index[proc].size(); ++i) {
          //int ind=f_index[proc][i];
          //query_values_d[ind+dof*N_pts]=f_cubic_unordered_d[f_index_procs_self_offset[proc]+i+dof*N_pts];
          //}
          //PetscSynchronizedPrintf(PETSC_COMM_WORLD, "[%d] proc = %d, f_index[proc].size()=%d\n", procid, proc, f_index[proc].size());
          //PetscSynchronizedFlush(PETSC_COMM_WORLD, PETSC_STDOUT);
          f_index[proc] = f_index[proc];
          f_index_ptr = thrust::raw_pointer_cast( f_index[proc].data() );
          copyQueryValues(&query_values_d[dof*N_pts],
                          &f_cubic_unordered_d[f_index_procs_self_offset[proc]+dof*N_pts], 
                          f_index_ptr, 
                          f_index[proc].size());
      }
    }
  }
  ZeitGeist_tock(interp_values_copy_kernel);
  timings[3]+=+MPI_Wtime();

  return;
}
