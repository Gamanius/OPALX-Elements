//
// Class BoxCornerDomain
//   :FIXME: add brief description
//
// Copyright (c) 2008,        Yves Ineichen, ETH Zürich,
//               2013 - 2015, Tülin Kaman, Paul Scherrer Institut, Villigen PSI, Switzerland
//               2017 - 2020, Paul Scherrer Institut, Villigen PSI, Switzerland
// All rights reserved
//
// Implemented as part of the master thesis
// "A Parallel Multigrid Solver for Beam Dynamics"
// and the paper
// "A fast parallel Poisson solver on irregular domains applied to beam dynamics simulations"
// (https://doi.org/10.1016/j.jcp.2010.02.022)
//
// This file is part of OPAL.
//
// OPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// You should have received a copy of the GNU General Public License
// along with OPAL. If not, see <https://www.gnu.org/licenses/>.
//
#ifndef BOXCORNER_DOMAIN_H
#define BOXCORNER_DOMAIN_H

#include <map>
#include <string>
#include <cmath>
#include <iostream>  // Neeeded for stream I/O
#include <fstream>   // Needed for file I/O
#include "IrregularDomain.h"


/*

    A and B are the half apperture of the box


                                     / (A,B)
                                    /
                                   /
                                  /
    L1                         /
------------      --------------+ (-A,B)
           | L2 |             |
        C|      |             |
           |------|             |      /
         .....                  |     /
(0,0)---.......-----------------+    /
         .....                  |   /
   z                            |  /
   |                            | /
--------------------------------+/ (-A,-B)

            Length_m

Test in which of the 3 parts of the geometry we are in.

    if((z < L1) || (z > (L1 + L2)))
        b = B;
    else
        b = B-C;


A  = max_m(0)
B  = max_m(1)
L1 = min_m(2)
L2 = max_m(2) - min_m(2)
*/

class BoxCornerDomain : public IrregularDomain {

public:
    using IrregularDomain::StencilIndex_t;
    using IrregularDomain::StencilValue_t;

    /**
     * \param A depth of the box
     * \param B maximal height of the box
     * \param C height of the corner
     * \param length of the structure
     * \param L1 length of the first part of the structure
     * \param L2 length of the corner
     */
    BoxCornerDomain(double A, double B, double C, double length,
                    double L1, double L2, Vector_t nr, Vector_t hr,
                    std::string interpl);
    ~BoxCornerDomain();

    /// as a function of z, determine the hight (B) of the geometry
    inline double getB(double z) {
      if((z < min_m(2)) || (z > max_m(2)))
            return max_m(1);
        else
            return max_m(1) - C_m;
    }

    /// queries if a given (x,y,z) coordinate lies inside the domain
    inline bool isInside(int x, int y, int z) {
        const double xx = (x - (nr_m[0] - 1) / 2.0) * hr_m[0];
        const double yy = (y - (nr_m[1] - 1) / 2.0) * hr_m[1];
        const double b = getB(z * hr_m[2]);
        return (xx < getXRangeMax() && yy < b && z != 0 && z != nr_m[2] - 1);
    }

    void compute(Vector_t hr, NDIndex<3> localId);

private:

    //XXX: since the Y coorindate is dependent on the Z value we need (int,
    //int) -> intersection. To simplify things (for now) we use the same
    //structure for X...
    /// Map from a ([(x or y], z) to a list of intersection values with
    /// boundary.
    typedef std::multimap< std::pair<int, int>, double > BoxCornerPointList;

    /// all intersection points with grid lines in X direction
    BoxCornerPointList IntersectXDir;

    /// all intersection points with grid lines in Y direction
    BoxCornerPointList IntersectYDir;

    /// because the geometry can change in the y direction
    double actBMin_m;

    double actBMax_m;

    /// length of the structure
    double length_m;

    /// height of the corner
    double C_m;



    inline double getXIntersection(double cx, int /*z*/) {
        return (cx < 0) ? min_m(0) : max_m(0);
    }

    inline double getYIntersection(double cy, int z) {
        return (cy < 0) ? min_m(1) : getB(z * hr_m[2]);
    }

    /// conversion from (x,y,z) to index in xyz plane
    inline int toCoordIdx(int x, int y, int z) {
        return (z * nr_m[1] + y) * nr_m[0] + x;
    }

    /// conversion from (x,y,z) to index on the 3D grid
    int indexAccess(int x, int y, int z) {
        return idxMap_m[toCoordIdx(x, y, z)];
    }

    /// different interpolation methods for boundary points
    void constantInterpolation(int x, int y, int z, StencilValue_t& value,
                               double &scaleFactor) override;

    void linearInterpolation(int x, int y, int z, StencilValue_t& value,
                             double &scaleFactor) override;

    void quadraticInterpolation(int x, int y, int z, StencilValue_t& value,
                                double &scaleFactor) override;

};

#endif

// vi: set et ts=4 sw=4 sts=4:
// Local Variables:
// mode:c
// c-basic-offset: 4
// indent-tabs-mode: nil
// require-final-newline: nil
// End:
