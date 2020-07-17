//
// Class EllipticDomain
//   This class provides an elliptic beam pipe. The mesh adapts to the bunch size
//   in the longitudinal direction. At the intersection of the mesh with the
//   beam pipe, three stencil interpolation methods are available.
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
#ifndef ELLIPTICAL_DOMAIN_H
#define ELLIPTICAL_DOMAIN_H

#include <vector>
#include <map>
#include <string>
#include <cmath>
#include "IrregularDomain.h"
#include "Structure/BoundaryGeometry.h"
#include "Utilities/OpalException.h"

class EllipticDomain : public IrregularDomain {

public:
    EllipticDomain(BoundaryGeometry *bgeom, IntVector_t nr,
                   Vector_t hr, std::string interpl);

    ~EllipticDomain();

    int getNumXY() const override;

    /// queries if a given (x,y,z) coordinate lies inside the domain
    bool isInside(int x, int y, int z) const {
        double xx = getXRangeMin() + hr_m[0] * (x + 0.5);
        double yy = getYRangeMin() + hr_m[1] * (y + 0.5);

        bool isInsideEllipse = (xx * xx / (getXRangeMax() * getXRangeMax()) +
                                yy * yy / (getYRangeMax() * getYRangeMax()) < 1);

        return (isInsideEllipse && z >= 0 && z < nr_m[2]);
    }

    /// calculates intersection
    void compute(Vector_t hr, NDIndex<3> localId);

    void resizeMesh(Vector_t& origin, Vector_t& hr, const Vector_t& rmin,
                    const Vector_t& rmax, double dh) override;

private:

    /// Map from a single coordinate (x or y) to a list of intersection values with
    /// boundary.
    typedef std::multimap<int, double> EllipticPointList_t;

    /// all intersection points with grid lines in X direction
    EllipticPointList_t intersectXDir_m;

    /// all intersection points with grid lines in Y direction
    EllipticPointList_t intersectYDir_m;

    /// number of nodes in the xy plane (for this case: independent of the z coordinate)
    int nxy_m;

    /// conversion from (x,y) to index in xy plane
    int toCoordIdx(int x, int y) const { return y * nr_m[0] + x; }

    /// conversion from (x,y,z) to index on the 3D grid
    int indexAccess(int x, int y, int z) const {
        return idxMap_m.at(toCoordIdx(x, y)) + z * nxy_m;
    }

    int coordAccess(int idx) const {
        int ixy = idx % nxy_m;
        return coordMap_m.at(ixy);
    }

    /// different interpolation methods for boundary points
    void constantInterpolation(int x, int y, int z, StencilValue_t& value,
                               double &scaleFactor) const override;

    void linearInterpolation(int x, int y, int z, StencilValue_t& value,
                             double &scaleFactor) const override;

    void quadraticInterpolation(int x, int y, int z, StencilValue_t& value,
                                double &scaleFactor) const override;

    /// function to handle the open boundary condition in longitudinal direction
    void robinBoundaryStencil(int z, double &F, double &B, double &C) const;
};

#endif //#ifdef ELLIPTICAL_DOMAIN_H

// vi: set et ts=4 sw=4 sts=4:
// Local Variables:
// mode:c
// c-basic-offset: 4
// indent-tabs-mode: nil
// require-final-newline: nil
// End:
