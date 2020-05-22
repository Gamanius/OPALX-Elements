//
// Class IrregularDomain
//   Defines a common abstract interface for different types of boundaries.
//
// Copyright (c) 2010 - 2013, Yves Ineichen, ETH Zürich,
//               2013 - 2015, Tülin Kaman, Paul Scherrer Institut, Villigen PSI, Switzerland
// All rights reserved
//
// Implemented as part of the PhD thesis
// "Toward massively parallel multi-objective optimization with application to
// particle accelerators" (https://doi.org/10.3929/ethz-a-009792359)
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
#ifndef IRREGULAR_DOMAIN_H
#define IRREGULAR_DOMAIN_H

#include <vector>
#include <string>
#include "Algorithms/PBunchDefs.h"
#include "Algorithms/Quaternion.h"

/// enumeration corresponding to different interpolation methods at the boundary
enum {
    CONSTANT,
    LINEAR,
    QUADRATIC
};


class IrregularDomain {

public:

    /** method to compute the intersection points with the boundary geometry (stored in some appropriate data structure)
     * \param hr updated mesh spacings
     */
    virtual void compute(Vector_t hr) = 0;
    virtual void compute(Vector_t hr, NDIndex<3> localId) = 0;
    /** method to get the number of gridpoints in a given z plane
     * \param z coordinate of the z plane
     * \return int number of grid nodes in the given z plane
     */
    virtual int getNumXY(int z) = 0;

    /// method to calculate the stencil at a boundary points
    /// \param x index of the current element in the matrix
    /// \param y index of the current element in the matrix
    /// \param z index of the current element in the matrix
    /// \param W stencil value of the element in the west of idx: (x-1)
    /// \param E stencil value of the element in the east of idx: (x+1)
    /// \param S stencil value of the element in the south of idx: (y-1)
    /// \param N stencil value of the element in the north of idx: (y+1)
    /// \param F stencil value of the element in front of idx: (z-1)
    /// \param B stencil value of the element in the back of idx: (z+1)
    /// \param C stencil value of the element in the center
    virtual void getBoundaryStencil(int x, int y, int z, double &W, double &E, double &S, double &N, double &F, double &B, double &C, double &scaleFactor) = 0;

    /// method to calculate the stencil at a boundary points
    /// \param idx index of the current element in the matrix
    /// \param W stencil value of the element in the west of idx: (x-1)
    /// \param E stencil value of the element in the east of idx: (x+1)
    /// \param S stencil value of the element in the south of idx: (y-1)
    /// \param N stencil value of the element in the north of idx: (y+1)
    /// \param F stencil value of the element in front of idx: (z-1)
    /// \param B stencil value of the element in the back of idx: (z+1)
    /// \param C stencil value of the element in the center
    virtual void getBoundaryStencil(int idx, double &W, double &E, double &S, double &N, double &F, double &B, double &C, double &scaleFactor) = 0;

    /// method to calculate the neighbours in the matrix of the current index (x,y,z)
    /// \param x index of the current element in the matrix
    /// \param y index of the current element in the matrix
    /// \param z index of the current element in the matrix
    /// \param W stencil index of the element in the west of idx: (x-1)
    /// \param E stencil index of the element in the east of idx: (x+1)
    /// \param S stencil index of the element in the south of idx: (y-1)
    /// \param N stencil index of the element in the north of idx: (y+1)
    /// \param F stencil index of the element in front of idx: (z-1)
    /// \param B stencil index of the element in the back of idx: (z+1)
    virtual void getNeighbours(int x, int y, int z, int &W, int &E, int &S, int &N, int &F, int &B) = 0;
    virtual void getNeighbours(int idx, int &W, int &E, int &S, int &N, int &F, int &B) = 0;

    /// method that identifies a specialized boundary geometry
    /// \return std::string containing a description of the boundary geometry used
    virtual std::string getType() = 0;

    /// method that checks if a given point lies inside the boundary
    /// \param x index of the current element in the matrix
    /// \param y index of the current element in the matrix
    /// \param z index of the current element in the matrix
    /// \return boolean indicating if the point lies inside the boundary
    virtual bool isInside(int x, int y, int z) = 0;

    Vector_t getNr() { return nr; }
    Vector_t getHr() { return hr; }
    void setNr(Vector_t nri) { nr = nri; }
    void setHr(Vector_t hri) { hr = hri; }

    void setMinMaxZ(double minz, double maxz) { zMin_m=minz; zMax_m=maxz; }
    double getMinZ() { return zMin_m; }
    double getMaxZ() { return zMax_m; }

    void setGlobalMeanR(Vector_t rmean) { rMean_m = rmean;}
    Vector_t getGlobalMeanR() { return rMean_m; }

    void setGlobalToLocalQuaternion(Quaternion_t globalToLocalQuaternion){
        globalToLocalQuaternion_m = globalToLocalQuaternion;}
    Quaternion_t getGlobalToLocalQuaternion() { return globalToLocalQuaternion_m;}

    virtual double getXRangeMin() = 0;
    virtual double getXRangeMax() = 0;
    virtual double getYRangeMin() = 0;
    virtual double getYRangeMax() = 0;
    virtual double getZRangeMin() = 0;
    virtual double getZRangeMax() = 0;

    virtual int getIdx(int x, int y, int z) = 0;
    virtual bool hasGeometryChanged() = 0;
    virtual ~IrregularDomain() {};

    // FIXME The function body should be implemented in derived classes.
    virtual void resizeMesh(Vector_t& origin, Vector_t& hr,
                            const Vector_t& /*rmin*/, const Vector_t& /*rmax*/, double /*dh*/) {
        double xmin = getXRangeMin();
        double xmax = getXRangeMax();
        double ymin = getYRangeMin();
        double ymax = getYRangeMax();
        double zmin = getZRangeMin();
        double zmax = getZRangeMax();

        origin = Vector_t(xmin, ymin , zmin);
        Vector_t mymax = Vector_t(xmax, ymax , zmax);

        for (int i = 0; i < 3; i++)
            hr[i] = (mymax[i] - origin[i]) / nr[i];
    };

protected:

    // a irregular domain is always defined on a grid
    /// number of mesh points in each direction
    Vector_t nr;
    /// mesh-spacings in each direction
    Vector_t hr;

    /// min/max of bunch in floor coordinates
    double zMin_m;
    double zMax_m;

    /// mean position of bunch (m)
    Vector_t rMean_m;
    Quaternion_t globalToLocalQuaternion_m;
};

#endif

// vi: set et ts=4 sw=4 sts=4:
// Local Variables:
// mode:c
// c-basic-offset: 4
// indent-tabs-mode: nil
// require-final-newline: nil
// End:
