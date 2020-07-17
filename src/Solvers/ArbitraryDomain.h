//
// Class ArbitraryDomain
//   Interface to iterative solver and boundary geometry
//   for space charge calculation
//
// Copyright (c) 2008,        Yves Ineichen, ETH Zürich,
//               2013 - 2015, Tülin Kaman, Paul Scherrer Institut, Villigen PSI, Switzerland
//                      2016, Daniel Winklehner, Massachusetts Institute of Technology
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
#ifndef ARBITRARY_DOMAIN_H
#define ARBITRARY_DOMAIN_H

#include <mpi.h>
#include <hdf5.h>
#include "H5hut.h"

#include <map>
#include <string>
#include <tuple>
#include <vector>
#include "IrregularDomain.h"

class BoundaryGeometry;

class ArbitraryDomain : public IrregularDomain {

public:
    using IrregularDomain::StencilIndex_t;
    using IrregularDomain::StencilValue_t;

    ArbitraryDomain(BoundaryGeometry *bgeom, Vector_t nr, Vector_t hr,
                    std::string interpl);

    ArbitraryDomain(BoundaryGeometry *bgeom, Vector_t nr, Vector_t hr,
                    Vector_t globalMeanR, Quaternion_t globalToLocalQuaternion,
                    std::string interpl);

    ~ArbitraryDomain();

    /// queries if a given (x,y,z) coordinate lies inside the domain
    bool isInside(int idx, int idy, int idz);
    /// returns number of nodes in xy plane
    int getNumXY(int idz);
    // calculates intersection with rotated and shifted geometry
    void compute(Vector_t hr, NDIndex<3> localId);

    int getStartId() {return startId;}

private:
    BoundaryGeometry *bgeom_m;

    /** PointList maps from an (x,z) resp. (y,z) pair to double values
     * (=intersections with boundary)
     */
    typedef std::multimap< std::tuple<int, int, int>, double > PointList;

    /// all intersection points with gridlines in X direction
    PointList IntersectHiX, IntersectLoX;

    /// all intersection points with gridlines in Y direction
    PointList IntersectHiY, IntersectLoY;

    /// all intersection points with gridlines in Z direction
    PointList IntersectHiZ, IntersectLoZ;

    // meanR to shift from global to local frame
    Vector_t globalMeanR_m;
    //    Quaternion_t globalToLocalQuaternion_m;  because defined in parent class
    Quaternion_t localToGlobalQuaternion_m;

    int startId;

    // Here we store the number of nodes in a xy layer for a given z coordinate
    std::map<int, int> numXY;

    // Mapping all cells that are inside the geometry
    std::map<int, bool> IsInsideMap;

    Vector_t geomCentroid_m;
    Vector_t globalInsideP0_m;

    // Conversion from (x,y,z) to index in xyz plane
    inline int toCoordIdx(int idx, int idy, int idz);
    // Conversion from (x,y,z) to index on the 3D grid
    int getIdx(int idx, int idy, int idz);
    // Conversion from a 3D index to (x,y,z)
    inline void getCoord(int idxyz, int &x, int &y, int &z);

    inline void crossProduct(double A[], double B[], double C[]);

    inline double dotProduct(double v1[], double v2[]) {
        return (v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]);
    }

    // Different interpolation methods for boundary points
    void constantInterpolation(int idx, int idy, int idz, StencilValue_t& value,
                               double &scaleFactor) override;

    void linearInterpolation(int idx, int idy, int idz, StencilValue_t& value,
                             double &scaleFactor) override;

    // Rotate positive axes with quaternion -DW
    inline void rotateWithQuaternion(Vector_t &v, Quaternion_t const quaternion);

    inline void rotateXAxisWithQuaternion(Vector_t &v, Quaternion_t const quaternion);
    inline void rotateYAxisWithQuaternion(Vector_t &v, Quaternion_t const quaternion);
    inline void rotateZAxisWithQuaternion(Vector_t &v, Quaternion_t const quaternion);
};

#endif

// vi: set et ts=4 sw=4 sts=4:
// Local Variables:
// mode:c
// c-basic-offset: 4
// indent-tabs-mode: nil
// require-final-newline: nil
// End:
