//
// Class MGPoissonSolver
//   This class contains methods for solving Poisson's equation for the
//   space charge portion of the calculation.
//
//   A smoothed aggregation based AMG preconditioned iterative solver for space charge
//   \see FFTPoissonSolver
//   \warning This solver is in an EXPERIMENTAL STAGE. For reliable simulations use the FFTPoissonSolver
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

//#define DBG_STENCIL

#include "Solvers/MGPoissonSolver.h"

#include "Structure/BoundaryGeometry.h"
#include "ArbitraryDomain.h"
#include "EllipticDomain.h"
#include "BoxCornerDomain.h"

#include "Track/Track.h"
#include "Physics/Physics.h"
#include "Utilities/OpalException.h"
#include "Attributes/Attributes.h"
#include "ValueDefinitions/RealVariable.h"
#include "AbstractObjects/OpalData.h"
#include "Utilities/Options.h"

// #include "Epetra_Operator.h"
// #include "EpetraExt_RowMatrixOut.h"
// #include "Epetra_Import.h"

#include <Tpetra_Import.hpp>
#include <BelosTpetraAdapter.hpp>

#include "Teuchos_CommandLineProcessor.hpp"

#include "BelosLinearProblem.hpp"
#include "BelosRCGSolMgr.hpp"
// #include "BelosEpetraAdapter.hpp"
#include "BelosBlockCGSolMgr.hpp"

#include <MueLu_CreateTpetraPreconditioner.hpp>
// #include <MueLu_TpetraOperator.hpp>

// #include "ml_MultiLevelPreconditioner.h"
// #include "ml_MultiLevelOperator.h"
// #include "ml_epetra_utils.h"

// #include "Isorropia_Exception.hpp"
// #include "Isorropia_Epetra.hpp"
// #include "Isorropia_EpetraRedistributor.hpp"
// #include "Isorropia_EpetraPartitioner.hpp"

#include <algorithm>

using Teuchos::RCP;
using Teuchos::rcp;
using Physics::c;

MGPoissonSolver::MGPoissonSolver ( PartBunch *beam,
                                   Mesh_t *mesh,
                                   FieldLayout_t *fl,
                                   std::vector<BoundaryGeometry *> geometries,
                                   std::string itsolver,
                                   std::string interpl,
                                   double tol,
                                   int maxiters,
                                   std::string precmode)
    : isMatrixfilled_m(false)
    , geometries_m(geometries)
    , tol_m(tol)
    , maxiters_m(maxiters)
    , comm_mp(new Comm_t(Ippl::getComm()))
    , itsBunch_m(beam)
    , mesh_m(mesh)
    , layout_m(fl)
{

    domain_m = layout_m->getDomain();

    for (int i = 0; i < 3; i++) {
        hr_m[i] = mesh_m->get_meshSpacing(i);
        orig_nr_m[i] = domain_m[i].length();
    }

    if (itsolver == "CG") itsolver_m = AZ_cg;
    else if (itsolver == "BICGSTAB") itsolver_m = AZ_bicgstab;
    else if (itsolver == "GMRES") itsolver_m = AZ_gmres;
    else throw OpalException("MGPoissonSolver", "No valid iterative solver selected!");

    precmode_m = STD_PREC;
    if (precmode == "STD") precmode_m = STD_PREC;
    else if (precmode == "HIERARCHY") precmode_m = REUSE_HIERARCHY;
    else if (precmode == "REUSE") precmode_m = REUSE_PREC;
    else if (precmode == "NO") precmode_m = NO;

    tolerableIterationsCount_m = 2;
    numIter_m = -1;
    forcePreconditionerRecomputation_m = false;

    hasParallelDecompositionChanged_m = true;
    repartFreq_m = 1000;
    useRCB_m = false;
    if (Ippl::Info->getOutputLevel() > 3)
        verbose_m = true;
    else
        verbose_m = false;

    // Find CURRENT geometry
    currentGeometry = geometries_m[0];
    if (currentGeometry->getFilename() == "") {
        if (currentGeometry->getTopology() == "ELLIPTIC"){
            bp_m = std::unique_ptr<IrregularDomain>(
                new EllipticDomain(currentGeometry, orig_nr_m, hr_m, interpl));

        } else if (currentGeometry->getTopology() == "BOXCORNER") {
            bp_m = std::unique_ptr<IrregularDomain>(
                new BoxCornerDomain(currentGeometry->getA(),
                                    currentGeometry->getB(),
                                    currentGeometry->getC(),
                                    currentGeometry->getLength(),
                                    currentGeometry->getL1(),
                                    currentGeometry->getL2(),
                                    orig_nr_m, hr_m, interpl));
            bp_m->compute(itsBunch_m->get_hr());
        } else {
            throw OpalException("MGPoissonSolver::MGPoissonSolver",
                                "Geometry not known");
        }
    } else {
        NDIndex<3> localId = layout_m->getLocalNDIndex();
        if (localId[0].length() != domain_m[0].length() ||
            localId[1].length() != domain_m[1].length()) {
            throw OpalException("ArbitraryDomain::compute",
                                "The class ArbitraryDomain only works with parallelization\n"
                                "in z-direction.\n"
                                "Please set PARFFTX=FALSE, PARFFTY=FALSE, PARFFTT=TRUE in \n"
                                "the definition of the field solver in the input file.\n");
        }
        bp_m = std::unique_ptr<IrregularDomain>(
            new ArbitraryDomain(currentGeometry, orig_nr_m, hr_m, interpl));
    }

    map_p = Teuchos::null;
    A = Teuchos::null;
    LHS = Teuchos::null;
    RHS = Teuchos::null;
    MueLuPrec = Teuchos::null;
    prec_mp = Teuchos::null;

    numBlocks_m = Options::numBlocks;
    recycleBlocks_m = Options::recycleBlocks;
    nLHS_m = Options::nLHS;
    setupMueLuList();
    SetupBelosList();
    problem_mp = rcp(new Belos::LinearProblem<TpetraScalar_t, TpetraMultiVector_t, TpetraOperator_t>);
    // setup Belos solver
    if (numBlocks_m == 0 || recycleBlocks_m == 0)
        solver_mp = rcp(new Belos::BlockCGSolMgr<TpetraScalar_t, TpetraMultiVector_t, TpetraOperator_t>());
    else
        solver_mp = rcp(new Belos::RCGSolMgr<TpetraScalar_t, TpetraMultiVector_t, TpetraOperator_t>());
    solver_mp->setParameters(rcp(&belosList, false));
    convStatusTest = rcp(new Belos::StatusTestGenResNorm<TpetraScalar_t, TpetraMultiVector_t, TpetraOperator_t> (tol));
    convStatusTest->defineScaleForm(Belos::NormOfRHS, Belos::TwoNorm);

    //all timers used here
    FunctionTimer1_m = IpplTimings::getTimer("BGF-IndexCoordMap");
    FunctionTimer2_m = IpplTimings::getTimer("computeMap");
    FunctionTimer3_m = IpplTimings::getTimer("IPPL to RHS");
    FunctionTimer4_m = IpplTimings::getTimer("ComputeStencil");
    FunctionTimer5_m = IpplTimings::getTimer("ML");
    FunctionTimer6_m = IpplTimings::getTimer("Setup");
    FunctionTimer7_m = IpplTimings::getTimer("CG");
    FunctionTimer8_m = IpplTimings::getTimer("LHS to IPPL");
}

void MGPoissonSolver::deletePtr() {
    MueLuPrec = Teuchos::null;
    map_p  = Teuchos::null;
    A      = Teuchos::null;
    LHS    = Teuchos::null;
    RHS    = Teuchos::null;
    prec_mp = Teuchos::null;
}

MGPoissonSolver::~MGPoissonSolver() {
    deletePtr ();
    solver_mp = Teuchos::null;
    problem_mp = Teuchos::null;
}

void MGPoissonSolver::computePotential(Field_t& /*rho*/, Vector_t /*hr*/, double /*zshift*/) {
    throw OpalException("MGPoissonSolver", "not implemented yet");
}

void MGPoissonSolver::computeMap(NDIndex<3> localId) {
    if (itsBunch_m->getLocalTrackStep()%repartFreq_m == 0){
        deletePtr();
        if (useRCB_m)
            redistributeWithRCB(localId);
        else
            IPPLToMap3D(localId);

        extrapolateLHS();
    }
}

void MGPoissonSolver::extrapolateLHS() {
// Aitken-Neville
// Pi0 (x) := yi , i = 0 : n
// Pik (x) := (x − xi ) Pi+1,k−1(x) − (x − xi+k ) Pi,k−1(x) /(xi+k − xi )
// k = 1, . . . , n, i = 0, . . . , n − k.
    //we also have to redistribute LHS

    if (Teuchos::is_null(LHS)){
        LHS = rcp(new TpetraVector_t(map_p));
        LHS->putScalar(1.0);
    } else {
        RCP<TpetraVector_t> tmplhs = rcp(new TpetraVector_t(map_p));
        Tpetra::Import<> importer(map_p, LHS->getMap());
        tmplhs->doImport(*LHS, importer, Tpetra::CombineMode::ADD);
        LHS = tmplhs;
    }

    //...and all previously saved LHS
    std::deque< TpetraVector_t >::iterator it = OldLHS.begin();
    if (OldLHS.size() > 0) {
        int n = OldLHS.size();
        for (int i = 0; i < n; ++i) {
            TpetraVector_t tmplhs = TpetraVector_t(map_p);
            Tpetra::Import<> importer(map_p, it->getMap());
            tmplhs.doImport(*it, importer, Tpetra::CombineMode::ADD);
            *it = tmplhs;
            ++it;
        }
    }

    // extrapolate last OldLHS.size LHS to get a new start vector
    it = OldLHS.begin();
    if (nLHS_m == 0 || OldLHS.size()==0)
        LHS->putScalar(1.0);
    else if (OldLHS.size() == 1)
        *LHS = *it;
    else if (OldLHS.size() == 2)
        LHS->update(2.0, *it++, -1.0, *it, 0.0);
    else if (OldLHS.size() > 2) {
        int n = OldLHS.size();
        P_mp = rcp(new TpetraMultiVector_t(map_p, nLHS_m, false));
        for (int i = 0; i < n; ++i) {
           *(P_mp->getVectorNonConst(i)) = *it++;
        }
        for (int k = 1; k < n; ++k) {
           for (int i = 0; i < n - k; ++i) {
              P_mp->getVectorNonConst(i)->update(-(i + 1) / (float)k, *(P_mp->getVector(i + 1)), (i + k + 1) / (float)k);
           }
        }
        *LHS = *(P_mp->getVector(0));
     } else
        throw OpalException("MGPoissonSolver",
                            "Invalid number of old LHS: " + std::to_string(OldLHS.size()));
}


// given a charge-density field rho and a set of mesh spacings hr,
// compute the electric field and put in eg by solving the Poisson's equation
// XXX: use matrix stencil in computation directly (no Epetra, define operators
// on IPPL GRID)
void MGPoissonSolver::computePotential(Field_t &rho, Vector_t hr) {

    Inform msg("OPAL ", INFORM_ALL_NODES);
    nr_m[0] = orig_nr_m[0];
    nr_m[1] = orig_nr_m[1];
    nr_m[2] = orig_nr_m[2];

    bp_m->setGlobalMeanR(itsBunch_m->getGlobalMeanR());
    bp_m->setGlobalToLocalQuaternion(itsBunch_m->getGlobalToLocalQuaternion());
    bp_m->setNr(nr_m);

    NDIndex<3> localId = layout_m->getLocalNDIndex();

    IpplTimings::startTimer(FunctionTimer1_m);
    // Compute boundary intersections (only do on the first step)
    if (!itsBunch_m->getLocalTrackStep())
        bp_m->compute(hr, localId);
    IpplTimings::stopTimer(FunctionTimer1_m);

    // Define the Map
    INFOMSG(level3 << "* Computing Map..." << endl);
    IpplTimings::startTimer(FunctionTimer2_m);
    computeMap(localId);
    IpplTimings::stopTimer(FunctionTimer2_m);
    INFOMSG(level3 << "* Done." << endl);

    // Allocate the RHS with the new Epetra Map
    if (Teuchos::is_null(RHS))
        RHS = rcp(new TpetraVector_t(map_p));
    RHS->putScalar(0.0);

    // // get charge densities from IPPL field and store in Epetra vector (RHS)
    if (verbose_m) {
        Ippl::Comm->barrier();
        msg << "* Node:" << Ippl::myNode() << ", Filling RHS..." << endl;
        Ippl::Comm->barrier();
    }
    IpplTimings::startTimer(FunctionTimer3_m);
    int id = 0;
    float scaleFactor = itsBunch_m->getdT();


    if (verbose_m) {
        msg << "* Node:" << Ippl::myNode() << ", Rho for final element: "
            << rho[localId[0].last()][localId[1].last()][localId[2].last()].get()
            << endl;

        Ippl::Comm->barrier();
        msg << "* Node:" << Ippl::myNode() << ", Local nx*ny*nz = "
            <<  localId[2].last() *  localId[0].last() *  localId[1].last()
            << endl;
        msg << "* Node:" << Ippl::myNode()
            << ", Number of reserved local elements in RHS: "
            << RHS->getLocalLength() << endl;
        msg << "* Node:" << Ippl::myNode()
            << ", Number of reserved global elements in RHS: "
            << RHS->getGlobalLength() << endl;
        Ippl::Comm->barrier();
    }
    for (int idz = localId[2].first(); idz <= localId[2].last(); idz++) {
        for (int idy = localId[1].first(); idy <= localId[1].last(); idy++) {
            for (int idx = localId[0].first(); idx <= localId[0].last(); idx++) {
                if (bp_m->isInside(idx, idy, idz))
                        RHS->getDataNonConst()[id++] = 4*M_PI*rho[idx][idy][idz].get()/scaleFactor;
            }
        }
    }

    IpplTimings::stopTimer(FunctionTimer3_m);
    if (verbose_m) {
        Ippl::Comm->barrier();
        msg << "* Node:" << Ippl::myNode()
            << ", Number of Local Inside Points " << id << endl;
        msg << "* Node:" << Ippl::myNode() << ", Done." << endl;
        Ippl::Comm->barrier();
    }
    // build discretization matrix
    INFOMSG(level3 << "* Building Discretization Matrix..." << endl);
    IpplTimings::startTimer(FunctionTimer4_m);
    if (Teuchos::is_null(A))
        A = rcp(new TpetraCrsMatrix_t(map_p,  7, Tpetra::StaticProfile));
    ComputeStencil(hr, RHS);
    IpplTimings::stopTimer(FunctionTimer4_m);
    INFOMSG(level3 << "* Done." << endl);

#ifdef DBG_STENCIL
    EpetraExt::RowMatrixToMatlabFile("DiscrStencil.dat", *A);
#endif

    INFOMSG(level3 << "* Computing Preconditioner..." << endl);
    /* FIXME --> MueLu reuse types
    IpplTimings::startTimer(FunctionTimer5_m);
    if (MueLuPrec == Teuchos::null) {
        MueLuPrec = MueLu::CreateTpetraPreconditioner(A, MLList_m);
    } else if (precmode_m == REUSE_HIERARCHY) {
        // FIXME MueLuPrec->ReComputePreconditioner();
    } else if (precmode_m == REUSE_PREC){
    }
    IpplTimings::stopTimer(FunctionTimer5_m);
    */
    INFOMSG(level3 << "* Done." << endl);

    // setup preconditioned iterative solver
    // use old LHS solution as initial guess
    INFOMSG(level3 << "* Final Setup of Solver..." << endl);
    IpplTimings::startTimer(FunctionTimer6_m);
    problem_mp->setOperator(A);
    problem_mp->setLHS(LHS);
    problem_mp->setRHS(RHS);
    if (Teuchos::is_null(prec_mp)) {
        Teuchos::RCP<TpetraOperator_t> At = Teuchos::rcp_dynamic_cast<TpetraOperator_t>(A);
        prec_mp = MueLu::CreateTpetraPreconditioner(At, MueLuList_m);
    }
    problem_mp->setLeftPrec(prec_mp);
    solver_mp->setProblem( problem_mp);
    if (!problem_mp->isProblemSet()) {
        if (problem_mp->setProblem() == false) {
            ERRORMSG("Belos::LinearProblem failed to set up correctly!" << endl);
        }
    }
    IpplTimings::stopTimer(FunctionTimer6_m);
    INFOMSG(level3 << "* Done." << endl);

    double time = MPI_Wtime();

    INFOMSG(level3 << "* Solving for Space Charge..." << endl);
    IpplTimings::startTimer(FunctionTimer7_m);
    solver_mp->solve();
    IpplTimings::stopTimer(FunctionTimer7_m);
    INFOMSG(level3 << "* Done." << endl);

    std::ofstream timings;
    if (true || verbose_m) {
        time = MPI_Wtime() - time;
        double minTime = 0, maxTime = 0, avgTime = 0;
        reduce(time, minTime, 1, std::less<double>());
        reduce(time, maxTime, 1, std::greater<double>());
        reduce(time, avgTime, 1, std::plus<double>());
        avgTime /= comm_mp->getSize();
        if (comm_mp->getRank() == 0) {
            char filename[50];
            sprintf(filename, "timing_MX%d_MY%d_MZ%d_nProc%d_recB%d_numB%d_nLHS%d",
                    orig_nr_m[0], orig_nr_m[1], orig_nr_m[2],
                    comm_mp->getSize(), recycleBlocks_m, numBlocks_m, nLHS_m);

            timings.open(filename, std::ios::app);
            timings << solver_mp->getNumIters() << "\t"
                    //<< time <<" "<<
                    << minTime << "\t"
                    << maxTime << "\t"
                    << avgTime << "\t"
                    << numBlocks_m << "\t"
                    << recycleBlocks_m << "\t"
                    << nLHS_m << "\t"
                    //<< OldLHS.size() <<"\t"<<
                    << std::endl;

            timings.close();
        }

    }
    // Store new LHS in OldLHS
    if (nLHS_m > 1) OldLHS.push_front(*(LHS.get()));
    if (OldLHS.size() > nLHS_m) OldLHS.pop_back();

    //now transfer solution back to IPPL grid
    IpplTimings::startTimer(FunctionTimer8_m);
    id = 0;
    rho = 0.0;
    for (int idz = localId[2].first(); idz <= localId[2].last(); idz++) {
        for (int idy = localId[1].first(); idy <= localId[1].last(); idy++) {
            for (int idx = localId[0].first(); idx <= localId[0].last(); idx++) {
                  NDIndex<3> l(Index(idx, idx), Index(idy, idy), Index(idz, idz));
                  if (bp_m->isInside(idx, idy, idz))
                     rho.localElement(l) = LHS->getData()[id++] * scaleFactor;
            }
        }
    }
    IpplTimings::stopTimer(FunctionTimer8_m);

    if (itsBunch_m->getLocalTrackStep()+1 == (long long)Track::block->localTimeSteps.front()) {
        A = Teuchos::null;
        LHS = Teuchos::null;
        RHS = Teuchos::null;
        prec_mp = Teuchos::null;
    }
}


void MGPoissonSolver::redistributeWithRCB(NDIndex<3> localId) {

    int numMyGridPoints = 0;

    for (int idz = localId[2].first(); idz <= localId[2].last(); idz++) {
        for (int idy = localId[1].first(); idy <= localId[1].last(); idy++) {
            for (int idx = localId[0].first(); idx <= localId[0].last(); idx++) {
                if (bp_m->isInside(idx, idy, idz))
                    numMyGridPoints++;
             }
        }
     }


    int indexbase = 0;
    Teuchos::RCP<TpetraMap_t> bmap = Teuchos::rcp(new TpetraMap_t(Teuchos::OrdinalTraits<Tpetra::global_size_t>::invalid(),
                                                                  numMyGridPoints, indexbase, comm_mp));
    Teuchos::RCP<const TpetraMultiVector_t> coords = Teuchos::rcp(
        new TpetraMultiVector_t(bmap, 3, false));

/*FIXME --> Compare src/Solvers/AMR_MG/MueLuPreconditioner.hpp
    double *v;
    int stride = 0, stride2 = 0;
    coords->extractView(&v, &stride);
    stride2 = 2 * stride;

    for (int idz = localId[2].first(); idz <= localId[2].last(); idz++) {
        for (int idy = localId[1].first(); idy <= localId[1].last(); idy++) {
            for (int idx = localId[0].first(); idx <= localId[0].last(); idx++) {
                if (bp_m->isInside(idx, idy, idz)) {
                    v[0] = (double)idx;
                    v[stride] = (double)idy;
                    v[stride2] = (double)idz;
                    v++;
                }
            }
        }
    }
    */

    Teuchos::ParameterList paramlist;
    paramlist.set("Partitioning Method", "RCB");
    Teuchos::ParameterList &sublist = paramlist.sublist("ZOLTAN");
    sublist.set("RCB_RECTILINEAR_BLOCKS", "1");
    sublist.set("DEBUG_LEVEL", "1");

    /*
     * FIXME
    Teuchos::RCP<Isorropia::Epetra::Partitioner> part = Teuchos::rcp(
        new Isorropia::Epetra::Partitioner(coords, paramlist));

    Isorropia::Epetra::Redistributor rd(part);
    Teuchos::RCP<TpetraMultiVector_t> newcoords = rd.redistribute(*coords);

    newcoords->extractView(&v, &stride);
    stride2 = 2 * stride;
    numMyGridPoints = 0;
    */
    std::vector<int> MyGlobalElements;
    /* FIXME
    for (unsigned int i = 0; i < newcoords->getLocalLength(); i++) {
        MyGlobalElements.push_back(bp_m->getIdx(v[0], v[stride], v[stride2]));
        v++;
        numMyGridPoints++;
    }
*/
    map_p = Teuchos::rcp(new TpetraMap_t(Teuchos::OrdinalTraits<Tpetra::global_size_t>::invalid(),
                                         &MyGlobalElements[0], numMyGridPoints, indexbase, comm_mp));
}

void MGPoissonSolver::IPPLToMap3D(NDIndex<3> localId) {

    int NumMyElements = 0;
    std::vector<int> MyGlobalElements;

    for (int idz = localId[2].first(); idz <= localId[2].last(); idz++) {
        for (int idy = localId[1].first(); idy <= localId[1].last(); idy++) {
            for (int idx = localId[0].first(); idx <= localId[0].last(); idx++) {
                if (bp_m->isInside(idx, idy, idz)) {
                    MyGlobalElements.push_back(bp_m->getIdx(idx, idy, idz));
                    NumMyElements++;
                }
            }
        }
    }
    int indexbase = 0;
    map_p = Teuchos::rcp(new TpetraMap_t(Teuchos::OrdinalTraits<Tpetra::global_size_t>::invalid(),
                                         &MyGlobalElements[0], NumMyElements, indexbase, comm_mp));
}

void MGPoissonSolver::ComputeStencil(Vector_t /*hr*/, Teuchos::RCP<TpetraVector_t> RHS) {

    A->resumeFill();
    A->setAllToScalar(0.0);

    int NumMyElements = map_p->getNodeNumElements();
    auto MyGlobalElements = map_p->getMyGlobalIndices();

    std::vector<double> Values(6);
    std::vector<int> Indices(6);

    for (int i = 0 ; i < NumMyElements ; i++) {

        int NumEntries = 0;

        double WV, EV, SV, NV, FV, BV, CV, scaleFactor=1.0;
        int W, E, S, N, F, B;

        bp_m->getBoundaryStencil(MyGlobalElements[i], WV, EV, SV, NV, FV, BV, CV, scaleFactor);
        RHS->scale(scaleFactor);

        bp_m->getNeighbours(MyGlobalElements[i], W, E, S, N, F, B);
        if (E != -1) {
            Indices[NumEntries] = E;
            Values[NumEntries++] = EV;
        }
        if (W != -1) {
            Indices[NumEntries] = W;
            Values[NumEntries++] = WV;
        }
        if (S != -1) {
            Indices[NumEntries] = S;
            Values[NumEntries++] = SV;
        }
        if (N != -1) {
            Indices[NumEntries] = N;
            Values[NumEntries++] = NV;
        }
        if (F != -1) {
            Indices[NumEntries] = F;
            Values[NumEntries++] = FV;
        }
        if (B != -1) {
            Indices[NumEntries] = B;
            Values[NumEntries++] = BV;
        }

        // if matrix has already been filled (fillComplete()) we can only
        // replace entries

        if (isMatrixfilled_m) {
            // off-diagonal entries
            A->replaceGlobalValues(MyGlobalElements[i], NumEntries, &Values[0], &Indices[0]);
            // diagonal entry
            A->replaceGlobalValues(MyGlobalElements[i], 1, &CV, &MyGlobalElements[i]);
        } else {
            // off-diagonal entries
            A->insertGlobalValues(MyGlobalElements[i], NumEntries, &Values[0], &Indices[0]);
            // diagonal entry
            A->insertGlobalValues(MyGlobalElements[i], 1, &CV, &MyGlobalElements[i]);
        }
    }

    RCP<ParameterList_t> params = Teuchos::parameterList();
    params->set ("Optimize Storage", true);
    A->fillComplete(params);
    isMatrixfilled_m = true;
}

void MGPoissonSolver::printLoadBalanceStats() {

    //compute some load balance statistics
    size_t myNumPart = map_p->getNodeNumElements();
    size_t NumPart = map_p->getGlobalNumElements() * 1.0 / comm_mp->getSize();
    double imbalance = 1.0;
    if (myNumPart >= NumPart)
        imbalance += (myNumPart - NumPart) / NumPart;
    else
        imbalance += (NumPart - myNumPart) / NumPart;

    double max = 0.0, min = 0.0, avg = 0.0;
    size_t minn = 0, maxn = 0;
    reduce(imbalance, min, 1, std::less<double>());
    reduce(imbalance, max, 1, std::greater<double>());
    reduce(imbalance, avg, 1, std::plus<double>());
    reduce(myNumPart, minn, 1, std::less<size_t>());
    reduce(myNumPart, maxn, 1, std::greater<size_t>());

    avg /= comm_mp->getSize();
    *gmsg << "LBAL min = " << min << ", max = " << max << ", avg = " << avg << endl;
    *gmsg << "min nr gridpoints = " << minn << ", max nr gridpoints = " << maxn << endl;
}

Inform &MGPoissonSolver::print(Inform &os) const {
    os << "* *************** M G P o i s s o n S o l v e r ************************************ " << endl;
    os << "* h " << hr_m << '\n';
    os << "* ********************************************************************************** " << endl;
    return os;
}

// vi: set et ts=4 sw=4 sts=4:
// Local Variables:
// mode:c
// c-basic-offset: 4
// indent-tabs-mode: nil
// require-final-newline: nil
// End:
