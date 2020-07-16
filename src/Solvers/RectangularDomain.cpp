//
// Class RectangularDomain
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
#ifdef HAVE_SAAMG_SOLVER

#include "Solvers/RectangularDomain.h"
#include "Utilities/OpalException.h"

RectangularDomain::RectangularDomain(double a, double b, Vector_t nr, Vector_t hr) {
    setRangeMin(Vector_t(-a, -b, getMinZ()));
    setRangeMax(Vector_t( a,  b, getMax/()));
    setNr(nr);
    setHr(hr);
    nxy_m = nr[0] * nr[1];
}

void RectangularDomain::compute(Vector_t hr, NDIndex<3> /*localId*/){
    setHr(hr);
    nxy_m = nr[0] * nr[1];
}

int RectangularDomain::getNumXY(int /*z*/) {
    return nxy_m;
}

void RectangularDomain::getBoundaryStencil(int x, int y, int z, StencilValue_t& value,
                                           double &scaleFactor)
{
    //scaleFactor = 1.0;

    //W = -1/hr[0]*1/hr[0];
    //E = -1/hr[0]*1/hr[0];
    //N = -1/hr[1]*1/hr[1];
    //S = -1/hr[1]*1/hr[1];
    //F = -1/hr[2]*1/hr[2];
    //B = -1/hr[2]*1/hr[2];
    //C = 2/hr[0]*1/hr[0] + 2/hr[1]*1/hr[1] + 2/hr[2]*1/hr[2];

    scaleFactor  = hr[0] * hr[1] * hr[2];
    value.west   = -hr[1] * hr[2] / hr[0];
    value.east   = -hr[1] * hr[2] / hr[0];
    value.north  = -hr[0] * hr[2] / hr[1];
    value.south  = -hr[0] * hr[2] / hr[1];
    value.front  = -hr[0] * hr[1] / hr[2];
    value.back   = -hr[0] * hr[1] / hr[2];
    value.center = 2 * hr[1] * hr[2] / hr[0] + 2 * hr[0] * hr[2] / hr[1] + 2 * hr[0] * hr[1] / hr[2];

    if(!isInside(x + 1, y, z))
        value.east = 0.0; //we are a right boundary point

    if(!isInside(x - 1, y, z))
        value.west = 0.0; //we are a left boundary point

    if(!isInside(x, y + 1, z))
        value.north = 0.0; //we are a upper boundary point

    if(!isInside(x, y - 1, z))
        value.south = 0.0; //we are a lower boundary point

    //dirichlet
    if(z == 0)
        value.front = 0.0;
    if(z == nr[2] - 1)
        value.back = 0.0;

    if(false /*z == 0 || z == nr[2]-1*/) {

        //case where we are on the Robin BC in Z-direction
        //where we distinguish two cases
        //IFF: this values should not matter because they
        //never make it into the discretization matrix
        if(z == 0)
            value.front = 0.0;
        else
            value.back = 0.0;

        //add contribution of Robin discretization to center point
        //d the distance between the center of the bunch and the boundary
        //double cx = (x-(nr[0]-1)/2)*hr[0];
        //double cy = (y-(nr[1]-1)/2)*hr[1];
        //double cz = hr[2]*(nr[2]-1);
        //double d = sqrt(cx*cx+cy*cy+cz*cz);
        double d = hr[2] * (nr[2] - 1) / 2;
        //cout << cx << " " << cy << " " << cz << " " << 2/(d*hr[2]) << endl;
        value.center += 2 / (d * hr[2]);
        //C += 2/((hr[2]*(nr[2]-1)/2.0) * hr[2]);

        //scale all stencil-points in z-plane with 0.5 (Neumann discretization)
        value.west   /= 2.0;
        value.east   /= 2.0;
        value.north  /= 2.0;
        value.south  /= 2.0;
        value.center /= 2.0;
        scaleFactor  *= 0.5;
    }


    //simple check if center value of stencil is positive
#ifdef DEBUG
    if(value.center <= 0)
        throw OpalException("RectangularDomain::getBoundaryStencil",
                            "Stencil C is <= 0! This case should never occure!");
#endif
}

/*
void MGPoissonSolver::getNeighbours(const int idx, int& W, int& E, int& S, int& N, int& F, int& B, int& numOutOfDomain)
{

    int ixy, iz, iy, ix;
    int nx = nr_m[0];
    int ny = nr_m[1];
    int nz = nr_m[2];
    int nxy = nx*ny;
    ixy = idx % nxy;

    ix = ixy % nx;
    iy = (ixy - ix) / nx;
    iz = (idx - ixy) / nxy;
    numOutOfDomain = 0;

    if (iz == 0)
    {F = -1; numOutOfDomain++;}
    else
        F = idx - nxy;
    if (iz == nz - 1)
    {B = -1; numOutOfDomain++;}
    else
        B = idx + nxy;

    if (ix == 0)
    {W = -1; numOutOfDomain++;}
    else
        W = ixy - 1;
    if (ix == nx - 1)
    {E = -1; numOutOfDomain++;}
    else
        E = ixy + 1;

    if (iy == 0)
    {S = -1; numOutOfDomain++;}
    else
        S = ixy - nx;
    if (iy == ny - 1)
    {N = -1; numOutOfDomain++;}
    else
        N = ixy + nx;

    int cor = iz * nxy;
    switch(W) {
        case -1: break;
        default: W += cor;
    }
    switch(E) {
        case -1: break;
        default: E += cor;
    }
    switch(S) {
        case -1: break;
        default: S += cor;
    }
    switch(N) {
        case -1: break;
        default: N += cor;
    }

}

//at the moment this stencil has neumann everywhere except on the z=0 plane (Dirichlet)
//this code is experimental/not-optimized/not-final at the moment
inline Epetra_CrsMatrix* MGPoissonSolver::stencil3DOneSidedDirichlet(Vector_t hr)
{
    Epetra_CrsMatrix* Matrix = new Epetra_CrsMatrix(Copy, *Map,  7);

    Vector_t hr2 = hr*hr;
    double a = 2*(1/hr2[0]+1/hr2[1]+1/hr2[2]);
    double b = -1/hr2[0];
    double d = -1/hr2[1];
    double f = -1/hr2[2];

    int NumMyElements = Map->NumMyElements();
    int* MyGlobalElements = Map->MyGlobalElements();

    int W, E, S, N, F, B, numout;
    std::vector<double> Values(6);
    std::vector<int> Indices(6);

    for (int i = 0 ; i < NumMyElements ; ++i)
    {
        getNeighbours(MyGlobalElements[i], W, E, S, N, F, B, numout);

        int NumEntries = 0;
        double diag = a;

        if(F != -1) {

            switch(numout) {

                case 3: {
                            if(W == -1) {
                                Indices[NumEntries] = E;
                                Values[NumEntries++] = 0.25*b;
                            }

                            if(E == -1) {
                                Indices[NumEntries] = W;
                                Values[NumEntries++] = 0.25*b;
                            }

                            if(N == -1) {
                                Indices[NumEntries] = S;
                                Values[NumEntries++] = 0.25*d;
                            }

                            if(S == -1) {
                                Indices[NumEntries] = N;
                                Values[NumEntries++] = 0.25*d;
                            }

                            if(B == -1) {
                                Indices[NumEntries] = F;
                                Values[NumEntries++] = 0.25*f;
                            }

                            if(F == -1) {
                                Indices[NumEntries] = B;
                                Values[NumEntries++] = 0.25*f;
                            }

                            diag *= 0.125;

                            if(NumEntries != 3)
                                cout << "ERROR: while calc corners in stencil" << endl;

                            break;

                        }
                case 2: {

                            if(W != -1 && E != -1) {
                                Indices[NumEntries] = W;
                                Values[NumEntries++] = 0.25*b;
                                Indices[NumEntries] = E;
                                Values[NumEntries++] = 0.25*b;
                            }
                            if(E == -1 && W != -1) {
                                Indices[NumEntries] = W;
                                Values[NumEntries++] = 0.5*b;
                            }
                            if(W == -1 && E != -1) {
                                Indices[NumEntries] = E;
                                Values[NumEntries++] = 0.5*b;
                            }


                            if(N != -1 && S != -1) {
                                Indices[NumEntries] = N;
                                Values[NumEntries++] = 0.25*d;
                                Indices[NumEntries] = S;
                                Values[NumEntries++] = 0.25*d;
                            }
                            if(S == -1 && N != -1) {
                                Indices[NumEntries] = N;
                                Values[NumEntries++] = 0.5*d;
                            }
                            if(N == -1 && S != -1) {
                                Indices[NumEntries] = S;
                                Values[NumEntries++] = 0.5*d;
                            }


                            if(B != -1 && F != -1) {
                                Indices[NumEntries] = B;
                                Values[NumEntries++] = 0.25*f;
                                Indices[NumEntries] = F;
                                Values[NumEntries++] = 0.25*f;
                            }
                            if(B == -1 && F != -1) {
                                Indices[NumEntries] = F;
                                Values[NumEntries++] = 0.5*f;
                            }
                            if(F == -1 && B != -1) {
                                Indices[NumEntries] = B;
                                Values[NumEntries++] = 0.5*f;
                            }

                            diag *= 0.25;

                            if(NumEntries != 4)
                                cout << "ERROR: while calc edge stencil elements" << endl;

                            break;
                        }
                case 1: {


                            if(W != -1 && E != -1) {
                                Indices[NumEntries] = W;
                                Values[NumEntries++] = 0.5*b;
                                Indices[NumEntries] = E;
                                Values[NumEntries++] = 0.5*b;
                            }
                            if(E == -1 && W != -1) {
                                Indices[NumEntries] = W;
                                Values[NumEntries++] = b;
                            }
                            if(W == -1 && E != -1) {
                                Indices[NumEntries] = E;
                                Values[NumEntries++] = b;
                            }


                            if(N != -1 && S != -1) {
                                Indices[NumEntries] = S;
                                Values[NumEntries++] = 0.5*d;
                                Indices[NumEntries] = N;
                                Values[NumEntries++] = 0.5*d;
                            }
                            if(S == -1 && N != -1) {
                                Indices[NumEntries] = N;
                                Values[NumEntries++] = d;
                            }
                            if(N == -1 && S != -1) {
                                Indices[NumEntries] = S;
                                Values[NumEntries++] = d;
                            }


                            if(B != -1 && F != -1) {
                                Indices[NumEntries] = F;
                                Values[NumEntries++] = 0.5*f;
                                Indices[NumEntries] = B;
                                Values[NumEntries++] = 0.5*f;
                            }
                            if(B == -1 && F != -1) {
                                Indices[NumEntries] = F;
                                Values[NumEntries++] = f;
                            }
                            if(F == -1 && B != -1) {
                                Indices[NumEntries] = B;
                                Values[NumEntries++] = f;
                            }

                            diag *= 0.5;

                            if(NumEntries != 5)
                                cout << "ERROR: calc surface elements of stencil" << endl;

                            break;
                        }

                case 0: {
                            if(W != -1) {
                                Indices[NumEntries] = W;
                                Values[NumEntries++] = b;
                            } if(E != -1) {
                                Indices[NumEntries] = E;
                                Values[NumEntries++] = b;
                            } if(S != -1) {
                                Indices[NumEntries] = S;
                                Values[NumEntries++] = d;
                            } if(N != -1) {
                                Indices[NumEntries] = N;
                                Values[NumEntries++] = d;
                            } if(F != -1) {
                                Indices[NumEntries] = F;
                                Values[NumEntries++] = f;
                            } if(B != -1) {
                                Indices[NumEntries] = B;
                                Values[NumEntries++] = f;
                            }

                            break;
                        }
                default: {
                             cout << "ERROR CREATING THE STENCIL: points out of domain (" << numout << ") is >3 OR <0" << endl;
                         }
            }
        } else {
            //F == -1 --> this is our dirichlet boundary surface
            switch(numout) {
                case 3: {

                            if(W == -1) {
                                Indices[NumEntries] = E;
                                Values[NumEntries++] = 0.5*b;
                            }

                            if(E == -1) {
                                Indices[NumEntries] = W;
                                Values[NumEntries++] = 0.5*b;
                            }

                            if(N == -1) {
                                Indices[NumEntries] = S;
                                Values[NumEntries++] = 0.5*d;
                            }

                            if(S == -1) {
                                Indices[NumEntries] = N;
                                Values[NumEntries++] = 0.5*d;
                            }

                            Indices[NumEntries] = B;
                            Values[NumEntries++] = 0.25*f;

                            if(NumEntries != 3)
                                cout << "ERROR: calc corner (below) elements of stencil" << endl;

                            diag *= 0.25;
                            break;

                        }
                case 2:{

                           if(W != -1 && E != -1) {
                               Indices[NumEntries] = W;
                               Values[NumEntries++] = 0.5*b;
                               Indices[NumEntries] = E;
                               Values[NumEntries++] = 0.5*b;
                           }
                           if(W == -1 && E != -1) {
                               Indices[NumEntries] = E;
                               Values[NumEntries++] = b;
                           }

                           if(E == -1 && W != -1) {
                               Indices[NumEntries] = W;
                               Values[NumEntries++] = b;
                           }

                           if(N != -1 && S != -1) {
                               Indices[NumEntries] = N;
                               Values[NumEntries++] = 0.5*d;
                               Indices[NumEntries] = S;
                               Values[NumEntries++] = 0.5*d;
                           }
                           if(N == -1 && S != -1) {
                               Indices[NumEntries] = S;
                               Values[NumEntries++] = d;
                           }

                           if(S == -1 && N != -1) {
                               Indices[NumEntries] = N;
                               Values[NumEntries++] = d;
                           }

                           Indices[NumEntries] = B;
                           Values[NumEntries++] = 0.25*f;

                           if(NumEntries != 4)
                               cout << "ERROR: calc edge (below) elements of stencil" << endl;

                           diag *= 0.25;
                           break;

                       }
                case 1:{

                           Indices[NumEntries] = E;
                           Values[NumEntries++] = b;

                           Indices[NumEntries] = W;
                           Values[NumEntries++] = b;

                           Indices[NumEntries] = S;
                           Values[NumEntries++] = d;

                           Indices[NumEntries] = N;
                           Values[NumEntries++] = d;

                           Indices[NumEntries] = B;
                           Values[NumEntries++] = f;

                           if(NumEntries != 5)
                               cout << "ERROR: calc surface (below) elements of stencil: " << NumEntries << endl;

                           break;

                       }
                default: {
                             cout << "error in dirichlet surface!" << endl;
                         }
            }
        }
        // put the off-diagonal entries
        Matrix->InsertGlobalValues(MyGlobalElements[i], NumEntries,
                &Values[0], &Indices[0]);

        // Put in the diagonal entry
        Matrix->InsertGlobalValues(MyGlobalElements[i], 1,
                &diag, MyGlobalElements + i);

    }
    Matrix->FillComplete();
    Matrix->OptimizeStorage();

    return(Matrix);
}

//stencil for longitudinal neumann and transversal dirichlet BC
//this code is experimental/not-optimized/not-final at the moment
inline Epetra_CrsMatrix* MGPoissonSolver::stencil3DLongitudinalNeumann(Vector_t hr)
{
    Epetra_CrsMatrix* Matrix = new Epetra_CrsMatrix(Copy, *Map,  7);

    Vector_t hr2 = hr*hr;
    double a = 2*(1/hr2[0]+1/hr2[1]+1/hr2[2]);
    double b = -1/hr2[0];
    double d = -1/hr2[1];
    double f = -1/hr2[2];

    int NumMyElements = Map->NumMyElements();
    int* MyGlobalElements = Map->MyGlobalElements();

    int W, E, S, N, F, B, numout;
    std::vector<double> Values(6);
    std::vector<int> Indices(6);

    for (int i = 0 ; i < NumMyElements ; ++i)
    {
        getNeighbours(MyGlobalElements[i], W, E, S, N, F, B, numout);

        int NumEntries = 0;
        double diag = a;
        //numout = 0;

        //transversal direction: if below == -1 || above == -1 ===> NEUMANN
        //else dirichlet:
        if(F != -1 && B != -1)
            numout = 0;

        //only on left longitudinal end open bc -- else dirichlet
        //if(F != -1)
        //  numout = 0;

        switch(numout) {

            case 3: {
                        if(W == -1) {
                            Indices[NumEntries] = E;
                            Values[NumEntries++] = 0.5*b;
                        }

                        if(E == -1) {
                            Indices[NumEntries] = W;
                            Values[NumEntries++] = 0.5*b;
                        }

                        if(N == -1) {
                            Indices[NumEntries] = S;
                            Values[NumEntries++] = 0.5*d;
                        }

                        if(S == -1) {
                            Indices[NumEntries] = N;
                            Values[NumEntries++] = 0.5*d;
                        }

                        if(B == -1) {
                            Indices[NumEntries] = F;
                            Values[NumEntries++] = f;
                        }

                        if(F == -1) {
                            Indices[NumEntries] = B;
                            Values[NumEntries++] = f;
                        }

                        diag *= 0.5;

                        if(NumEntries != 3)
                            cout << "ERROR: while calc corners in stencil" << endl;

                        break;

                    }
            case 2: { //edges

                        if(E != -1) {
                            Indices[NumEntries] = E;
                            Values[NumEntries++] = 0.5*b;
                        }
                        if(W != -1) {
                            Indices[NumEntries] = W;
                            Values[NumEntries++] = 0.5*b;
                        }

                        if(S != -1) {
                            Indices[NumEntries] = S;
                            Values[NumEntries++] = 0.5*d;
                        }
                        if(N != -1) {
                            Indices[NumEntries] = N;
                            Values[NumEntries++] = 0.5*d;
                        }

                        if(B == -1) {
                            Indices[NumEntries] = F;
                            Values[NumEntries++] = f;
                        }
                        if(F == -1) {
                            Indices[NumEntries] = B;
                            Values[NumEntries++] = f;
                        }

                        diag *= 0.5;

                        if(NumEntries != 4)
                            cout << "ERROR: while calc edge stencil elements" << endl;

                        break;
                    }
            case 1: {

                        if(E != -1) {
                            Indices[NumEntries] = E;
                            Values[NumEntries++] = 0.5*b;
                        }
                        if(W != -1) {
                            Indices[NumEntries] = W;
                            Values[NumEntries++] = 0.5*b;
                        }

                        if(S != -1) {
                            Indices[NumEntries] = S;
                            Values[NumEntries++] = 0.5*d;
                        }
                        if(N != -1) {
                            Indices[NumEntries] = N;
                            Values[NumEntries++] = 0.5*d;
                        }

                        if(B == -1) {
                            Indices[NumEntries] = F;
                            Values[NumEntries++] = f;
                        }
                        if(F == -1) {
                            Indices[NumEntries] = B;
                            Values[NumEntries++] = f;
                        }

                        diag *= 0.5;

                        if(NumEntries != 5)
                            cout << "ERROR: calc surface elements of stencil" << endl;

                        break;
                    }

            case 0: {  //interior points (& dirichlet)
                        if(W != -1) {
                            Indices[NumEntries] = W;
                            Values[NumEntries++] = b;
                        } if(E != -1) {
                            Indices[NumEntries] = E;
                            Values[NumEntries++] = b;
                        } if(S != -1) {
                            Indices[NumEntries] = S;
                            Values[NumEntries++] = d;
                        } if(N != -1) {
                            Indices[NumEntries] = N;
                            Values[NumEntries++] = d;
                        } if(F != -1) {
                            Indices[NumEntries] = F;
                            Values[NumEntries++] = f;
                        } if(B != -1) {
                            Indices[NumEntries] = B;
                            Values[NumEntries++] = f;
                        }

                        break;
                    }
            default: {
                         cout << "ERROR CREATING THE STENCIL: points out of domain (" << numout << ") is >3 OR <0" << endl;
                     }
        }
        // put the off-diagonal entries
        Matrix->InsertGlobalValues(MyGlobalElements[i], NumEntries,
                &Values[0], &Indices[0]);

        // Put in the diagonal entry
        Matrix->InsertGlobalValues(MyGlobalElements[i], 1,
                &diag, MyGlobalElements + i);

    }
    Matrix->FillComplete();
    Matrix->OptimizeStorage();

    return(Matrix);
}
*/

#endif //#ifdef HAVE_SAAMG_SOLVER