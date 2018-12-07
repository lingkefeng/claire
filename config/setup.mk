# developer flags (ignore)
DBGCODE=no
PEDANTIC=yes

RM = rm -f
MKDIRS = mkdir -p

ifeq ($(DBGCODE),yes)
	#CXXFLAGS = -g -debug
	CXXFLAGS = -g
else
	#CXXFLAGS = -O3 -ansi
	CXXFLAGS = -O3 -ansi -g
endif


ifeq ($(USEINTEL),yes)
	CXXFLAGS += -xhost -parallel
	#CXXFLAGS += -openmp
	CXXFLAGS += -qopenmp
else
	CXXFLAGS += -fopenmp
#	CXXFLAGS += -mavx2
#	CXXFLAGS += -march=corei7-avx
	CXXFLAGS += -march=native
endif
CXXFLAGS += -std=c++11

ifeq ($(USEKNL),yes)
	CXXFLAGS += -DKNL
endif

ifeq ($(USEHASWELL),yes)
	ifeq ($(USESINGLE),yes)
		CXXFLAGS += -DHASWELL
	endif
endif


ifeq ($(PEDANTIC),yes)
	ifeq ($(USEINTEL),no)
		#CXXFLAGS += -Wall
	endif
	CXXFLAGS += -Warray-bounds -Wchar-subscripts -Wcomment
	CXXFLAGS += -Wenum-compare -Wformat -Wuninitialized
	CXXFLAGS += -Wmaybe-uninitialized -Wmain -Wnarrowing
	CXXFLAGS += -Wnonnull -Wparentheses #-Wpointer-sign
	CXXFLAGS += -Wreorder -Wreturn-type -Wsign-compare
	CXXFLAGS += -Wsequence-point -Wtrigraphs -Wunused-function
	CXXFLAGS += -Wwrite-strings #-Wunused-variable -Wunused-but-set-variable 
endif

ifeq ($(USEPNETCDF),yes)
	CXXFLAGS += -DREG_HAS_PNETCDF
endif

ifeq ($(USENIFTI),yes)
	CXXFLAGS += -DREG_HAS_NIFTI
endif

ifeq ($(USECUDA),yes)
	CXXFLAGS += -DREG_HAS_CUDA
	CXXFLAGS += -DREG_FFT_CUDA
	ifeq ($(USECUDADBG),yes)
		CXXFLAGS += -DREG_DBG_CUDA
	endif
endif

ifeq ($(USECUDA),yes)
	BINDIR = ./bingpu
	OBJDIR = ./objgpu
else
	BINDIR = ./bin
	OBJDIR = ./obj
endif
SRCDIR = ./src
INCDIR = ./include
APPDIR = ./apps
EXSRCDIR = ./3rdparty

#GIT_VERSION := $(shell git describe --abbrev=4 --dirty --always --tags)
GIT_VERSION := $(shell git describe --abbrev=4 --always --tags)
CXXFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"

CLAIRE_INC = -I$(INCDIR) -I$(EXSRCDIR)

ifeq ($(USECUDA),yes)
    # CUDA includes
    CUDA_INC = -I$(CUDA_DIR)/include -I$(INCDIR) -I$(EXSRCDIR)
endif

ifeq ($(USECUDA),yes)
	ifeq ($(USESINGLE),yes)
		ifeq ($(DBGCODE),yes)
				CLAIRE_INC += -isystem$(PETSC_DIR)/include -isystem$(PETSC_DIR)/$(PETSC_ARCH_CUDA_SINGLE_DBG)/include -I$(MPI_DIR)
				CUDA_INC += -I$(PETSC_DIR)/include -I$(PETSC_DIR)/$(PETSC_ARCH_CUDA_SINGLE_DBG)/include -I$(MPI_DIR)
		else
				CLAIRE_INC += -isystem$(PETSC_DIR)/include -isystem$(PETSC_DIR)/$(PETSC_ARCH_CUDA_SINGLE)/include -I$(MPI_DIR)
				CUDA_INC += -I$(PETSC_DIR)/include -I$(PETSC_DIR)/$(PETSC_ARCH_CUDA_SINGLE)/include -I$(MPI_DIR)
		endif
	else
		ifeq ($(DBGCODE),yes)
				CLAIRE_INC += -isystem$(PETSC_DIR)/include -isystem$(PETSC_DIR)/$(PETSC_ARCH_CUDA_DOUBLE_DBG)/include -I$(MPI_DIR)
				CUDA_INC += -I$(PETSC_DIR)/include -I$(PETSC_DIR)/$(PETSC_ARCH_CUDA_DOUBLE_DBG)/include -I$(MPI_DIR)
		else
				CLAIRE_INC += -isystem$(PETSC_DIR)/include -isystem$(PETSC_DIR)/$(PETSC_ARCH_CUDA_DOUBLE)/include -I$(MPI_DIR)
				CUDA_INC += -I$(PETSC_DIR)/include -I$(PETSC_DIR)/$(PETSC_ARCH_CUDA_DOUBLE)/include -I$(MPI_DIR)
		endif
	endif
else
ifeq ($(DBGCODE),yes)
	ifeq ($(USESINGLE),yes)
		CLAIRE_INC += -isystem$(PETSC_DIR)/include -isystem$(PETSC_DIR)/$(PETSC_ARCH_DBG_SINGLE)/include
	else 
		CLAIRE_INC += -isystem$(PETSC_DIR)/include -isystem$(PETSC_DIR)/$(PETSC_ARCH_DBG_DOUBLE)/include
	endif
else
	ifeq ($(USESINGLE),yes)
		CLAIRE_INC += -isystem$(PETSC_DIR)/include -isystem$(PETSC_DIR)/$(PETSC_ARCH_SINGLE)/include
	else
		CLAIRE_INC += -isystem$(PETSC_DIR)/include -isystem$(PETSC_DIR)/$(PETSC_ARCH_DOUBLE)/include
	endif
endif
endif


CLAIRE_INC += -I$(ACCFFT_DIR)/include
CLAIRE_INC += -I$(FFTW_DIR)/include
CLAIRE_INC += -I$(MORTON_DIR)
CLAIRE_INC += -I./3rdparty
# CUDA INCLUDE in CLAIRE
ifeq ($(USECUDA),yes)
		CUDA_INC += -I$(ACCFFT_DIR)/include
		CUDA_INC += -I$(FFTW_DIR)/include
    CLAIRE_INC += -I$(CUDA_DIR)/include
endif

# CUDA flags
CUDA_FLAGS=-c -Xcompiler "$(CXXFLAGS)" -std=c++11 -O3 -Xcompiler -fPIC -Wno-deprecated-gpu-targets
CUDA_FLAGS+=-gencode arch=compute_60,code=sm_60
CUDA_FLAGS+=-gencode arch=compute_70,code=sm_70


ifeq ($(USENIFTI),yes)
	CLAIRE_INC += -I$(NIFTI_DIR)/include/nifti
	CUDA_INC += -I$(NIFTI_DIR)/include/nifti
endif

ifeq ($(USEPNETCDF),yes)
	CLAIRE_INC += -I$(PNETCDF_DIR)/include
	CUDA_INC += -I$(PNETCDF_DIR)/include
endif

ifeq ($(USECUDA),yes)
	ifeq ($(USESINGLE),yes)
		ifeq ($(DBGCODE),yes)
				LDFLAGS += -L$(PETSC_DIR)/lib -L$(PETSC_DIR)/$(PETSC_ARCH_CUDA_SINGLE_DBG)/lib
		else
				LDFLAGS += -L$(PETSC_DIR)/lib -L$(PETSC_DIR)/$(PETSC_ARCH_CUDA_SINGLE)/lib
		endif
	else
		ifeq ($(DBGCODE),yes)
				LDFLAGS += -L$(PETSC_DIR)/lib -L$(PETSC_DIR)/$(PETSC_ARCH_CUDA_DOUBLE_DBG)/lib
		else
				LDFLAGS += -L$(PETSC_DIR)/lib -L$(PETSC_DIR)/$(PETSC_ARCH_CUDA_DOUBLE)/lib
		endif
	endif
else

ifeq ($(DBGCODE),yes)
	ifeq ($(USESINGLE),yes)
		LDFLAGS += -L$(PETSC_DIR)/lib -L$(PETSC_DIR)/$(PETSC_ARCH_DBG_SINGLE)/lib
	else
		LDFLAGS += -L$(PETSC_DIR)/lib -L$(PETSC_DIR)/$(PETSC_ARCH_DBG_DOUBLE)/lib
	endif
else
	ifeq ($(USESINGLE),yes)
		LDFLAGS += -L$(PETSC_DIR)/lib -L$(PETSC_DIR)/$(PETSC_ARCH_SINGLE)/lib
	else
		LDFLAGS += -L$(PETSC_DIR)/lib -L$(PETSC_DIR)/$(PETSC_ARCH_DOUBLE)/lib
	endif
endif
endif
LDFLAGS += -lpetsc -lf2clapack -lf2cblas 

#CUDA LINKERS
ifeq ($(USECUDA),yes)
    LDFLAGS += -L$(CUDA_DIR)/lib64 -lcusparse -lcufft -lcublas -lcudart
    ifeq ($(USECUDADBG),yes)
			LDFLAGS += -lnvToolsExt
		endif
endif

ifeq ($(USENIFTI),yes)
	LDFLAGS += -L$(NIFTI_DIR)/lib -lnifticdf -lniftiio -lznz -L$(ZLIB_DIR)/lib -lz
endif

#LDFLAGS+= -lcrypto -lssl -ldl
ifeq ($(USEINTEL),yes)
	LDFLAGS += -limf
else
	LDFLAGS+= -ldl
endif


ifeq ($(USEINTELMPI),yes)
	LDFLAGS += -lmpi_mt
endif
LDFLAGS += -lm


# FFT LIBRARIES
LDFLAGS += -L$(ACCFFT_DIR)/lib -laccfft -laccfft_utils
ifeq ($(USECUDA),yes)
    LDFLAGS += -laccfft_gpu -laccfft_utils_gpu -lcudart -lcufft
endif
ifeq ($(USEPNETCDF),yes)
	LDFLAGS += -L$(PNETCDF_DIR)/lib -lpnetcdf
endif

ifeq ($(USESINGLE),yes)
	LDFLAGS += -L$(FFTW_DIR)/lib
endif
LDFLAGS += -L$(FFTW_DIR)/lib

ifeq ($(USESINGLE),yes)
	LDFLAGS += -lfftw3f_threads -lfftw3f
endif
LDFLAGS += -lfftw3_threads -lfftw3


BIN += $(BINDIR)/claire
ifeq ($(BUILDTOOLS),yes)
	BIN += $(BINDIR)/benchmark
	BIN += $(BINDIR)/clairetools
endif
ifeq ($(BUILDTEST),yes)
	BIN += $(BINDIR)/test
endif
