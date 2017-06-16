/*************************************************************************
 *  Copyright (c) 2017.
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
 ************************************************************************/

#ifndef _L2DISTANCE_H_
#define _L2DISTANCE_H_

#include "DistanceMeasure.hpp"




namespace reg {




class L2Distance : public DistanceMeasure {
 public:
    typedef DistanceMeasure SuperClass;
    typedef L2Distance Self;

    L2Distance(void);
    L2Distance(RegOpt*);
    virtual ~L2Distance(void);

    PetscErrorCode EvaluateFunctional(ScalarType*, Vec);

 protected:
    PetscErrorCode Initialize(void);
    PetscErrorCode ClearMemory(void);
};




}  // namespace reg




#endif