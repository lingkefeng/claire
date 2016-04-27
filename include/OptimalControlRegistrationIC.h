/**
 *  Description: we invert for an incompressible diffeomorphism;
 *  we only have to reimplement some of the functionality of the
 *  base class
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

#ifndef _OPTIMALCONTROLREGISTRATIONIC_H_
#define _OPTIMALCONTROLREGISTRATIONIC_H_

#include "OptimalControlRegistration.h"

namespace reg
{


class OptimalControlRegistrationIC : public OptimalControlRegistration
{

public:

    typedef OptimalControlRegistrationIC Self;
    typedef OptimalControlRegistration SuperClass;
    typedef ScalarType FFTScaType[2];

    OptimalControlRegistrationIC();
    OptimalControlRegistrationIC(RegOpt*);
    ~OptimalControlRegistrationIC();

protected:

    /*! init class variables (called by constructor) */
    PetscErrorCode Initialize(void);

    /*! clear memory (called by destructor) */
    PetscErrorCode ClearMemory(void);

    /*! compute body force */
    PetscErrorCode ComputeBodyForce(void);

    /*! compute incremental body force */
    PetscErrorCode ComputeIncBodyForce(void);

    /*! apply the projection operator to the
        body force and the incremental body force */
    PetscErrorCode ApplyProjection(VecField*);

    /*! reimplementation (accounting for incompressiblity) */
    PetscErrorCode SolveAdjointEquationSL(void);

    /*! reimplementation (accounting for incompressiblity) */
    PetscErrorCode SolveIncAdjointEquationGNSL(void);

private:

    FFTScaType *m_x1hat;
    FFTScaType *m_x2hat;
    FFTScaType *m_x3hat;

    FFTScaType *m_Kx1hat;
    FFTScaType *m_Kx2hat;
    FFTScaType *m_Kx3hat;

};


} // end of name space


#endif // _OPTIMALCONTROLREGISTRATIONIC_H_