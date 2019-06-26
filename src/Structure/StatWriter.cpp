#include "StatWriter.h"

#include "OPALconfig.h"
#include "Utilities/Util.h"
#include "Utilities/Timer.h"

#include <sstream>

StatWriter::StatWriter(const std::string& fname, bool restart)
    : SDDSWriter(fname, restart)
    , StatMarkerTimer_m(IpplTimings::getTimer("Write Stat"))
{ }


void StatWriter::fillHeader_m(const losses_t &losses) {

    static bool isNotFirst = false;
    if ( isNotFirst ) {
        return;
    }
    isNotFirst = true;

    this->addColumn("t", "double", "ns", "Time");
    this->addColumn("s", "double", "m", "Path length");
    this->addColumn("numParticles", "long", "1", "Number of Macro Particles");
    this->addColumn("charge", "double", "1", "Bunch Charge");
    this->addColumn("energy", "double", "MeV", "Mean Bunch Energy");

    this->addColumn("rms_x", "double", "m", "RMS Beamsize in x");
    this->addColumn("rms_y", "double", "m", "RMS Beamsize in y");
    this->addColumn("rms_s", "double", "m", "RMS Beamsize in s");

    this->addColumn("rms_px", "double", "1", "RMS Normalized Momenta in x");
    this->addColumn("rms_py", "double", "1", "RMS Normalized Momenta in y");
    this->addColumn("rms_ps", "double", "1", "RMS Normalized Momenta in s");

    this->addColumn("emit_x", "double", "m", "Normalized Emittance x");
    this->addColumn("emit_y", "double", "m", "Normalized Emittance y");
    this->addColumn("emit_s", "double", "m", "Normalized Emittance s");

    this->addColumn("mean_x", "double", "m", "Mean Beam Position in x");
    this->addColumn("mean_y", "double", "m", "Mean Beam Position in y");
    this->addColumn("mean_s", "double", "m", "Mean Beam Position in s");

    this->addColumn("ref_x", "double", "m", "x coordinate of reference particle in lab cs");
    this->addColumn("ref_y", "double", "m", "y coordinate of reference particle in lab cs");
    this->addColumn("ref_z", "double", "m", "z coordinate of reference particle in lab cs");

    this->addColumn("ref_px", "double", "1", "x momentum of reference particle in lab cs");
    this->addColumn("ref_py", "double", "1", "y momentum of reference particle in lab cs");
    this->addColumn("ref_pz", "double", "1", "z momentum of reference particle in lab cs");

    this->addColumn("max_x", "double", "m", "Max Beamsize in x");
    this->addColumn("max_y", "double", "m", "Max Beamsize in y");
    this->addColumn("max_s", "double", "m", "Max Beamsize in s");

    this->addColumn("xpx", "double", "1", "Correlation xpx");
    this->addColumn("ypy", "double", "1", "Correlation ypy");
    this->addColumn("zpz", "double", "1", "Correlation zpz");

    this->addColumn("Dx", "double", "m", "Dispersion in x");
    this->addColumn("DDx", "double", "1", "Derivative of dispersion in x");
    this->addColumn("Dy", "double", "m", "Dispersion in y");
    this->addColumn("DDy", "double", "1", "Derivative of dispersion in y");

    this->addColumn("Bx_ref", "double", "T", "Bx-Field component of ref particle");
    this->addColumn("By_ref", "double", "T", "By-Field component of ref particle");
    this->addColumn("Bz_ref", "double", "T", "Bz-Field component of ref particle");

    this->addColumn("Ex_ref", "double", "MV/m", "Ex-Field component of ref particle");
    this->addColumn("Ey_ref", "double", "MV/m", "Ey-Field component of ref particle");
    this->addColumn("Ez_ref", "double", "MV/m", "Ez-Field component of ref particle");

    this->addColumn("dE", "double", "MeV", "energy spread of the beam");
    this->addColumn("dt", "double", "ns", "time step size");
    this->addColumn("partsOutside", "double",  "1", "outside n*sigma of the beam");

    if (OpalData::getInstance()->isInOPALCyclMode() &&
        Ippl::getNodes() == 1) {
        this->addColumn("R0_x", "double", "m", "R0 Particle position in x");
        this->addColumn("R0_y", "double", "m", "R0 Particle position in y");
        this->addColumn("R0_s", "double", "m", "R0 Particle position in z");

        this->addColumn("P0_x", "double", "1", "R0 Particle momentum in x");
        this->addColumn("P0_y", "double", "1", "R0 Particle momentum in y");
        this->addColumn("P0_s", "double", "1", "R0 Particle momentum in z");
    }

    if (OpalData::getInstance()->isInOPALCyclMode()) {
        this->addColumn("halo_x", "double", "1", "Halo in x");
        this->addColumn("halo_y", "double", "1", "Halo in y");
        this->addColumn("halo_z", "double", "1", "Halo in z");

        this->addColumn("azimuth", "double", "deg",
                        "Azimuth in global coordinates");
    }

    for (size_t i = 0; i < losses.size(); ++ i) {
        this->addColumn(losses[i].first, "long", "1",
                        "Number of lost particles in element");
    }

    if ( mode_m == std::ios::app )
        return;

    OPALTimer::Timer simtimer;
    std::string dateStr(simtimer.date());
    std::string timeStr(simtimer.time());

    std::stringstream ss;
    ss << "Statistics data '"
       << OpalData::getInstance()->getInputFn()
       << "' " << dateStr << " " << timeStr;

    this->addDescription(ss.str(), "stat parameters");

    this->addDefaultParameters();


    this->addInfo("ascii", 1);
}


void StatWriter::write(PartBunchBase<double, 3> *beam, Vector_t FDext[],
                       const losses_t &losses, const double& azimuth)
{
    /// Start timer.
    IpplTimings::startTimer(StatMarkerTimer_m);

    /// Calculate beam statistics.
    beam->calcBeamParameters();

    double Ekin = beam->get_meanKineticEnergy();

    size_t npOutside = 0;
    if (Options::beamHaloBoundary > 0)
        npOutside = beam->calcNumPartsOutside(Options::beamHaloBoundary*beam->get_rrms());

    double  pathLength = 0.0;
    if (OpalData::getInstance()->isInOPALCyclMode())
        pathLength = beam->getLPath();
    else
        pathLength = beam->get_sPos();

    /// Write data to files. If this is the first write to the beam statistics file, write SDDS
    /// header information.

    double Q = beam->getCharge();

    if (Ippl::myNode() != 0) {
        IpplTimings::stopTimer(StatMarkerTimer_m);
        return;
    }

    fillHeader_m(losses);

    this->open();

    this->writeHeader();

    SDDSDataRow row(this->getColumnNames());
    row.addColumn("t", beam->getT() * 1e9);             // 1
    row.addColumn("s", pathLength);                     // 2
    row.addColumn("numParticles", beam->getTotalNum()); // 3
    row.addColumn("charge", Q);                         // 4
    row.addColumn("energy", Ekin);                      // 5

    row.addColumn("rms_x", beam->get_rrms()(0));        // 6
    row.addColumn("rms_y", beam->get_rrms()(1));        // 7
    row.addColumn("rms_s", beam->get_rrms()(2));        // 8

    row.addColumn("rms_px", beam->get_prms()(0));       // 9
    row.addColumn("rms_py", beam->get_prms()(1));       // 10
    row.addColumn("rms_ps", beam->get_prms()(2));       // 11

    row.addColumn("emit_x", beam->get_norm_emit()(0));  // 12
    row.addColumn("emit_y", beam->get_norm_emit()(1));  // 13
    row.addColumn("emit_s", beam->get_norm_emit()(2));  // 14

    row.addColumn("mean_x", beam->get_rmean()(0) );     // 15
    row.addColumn("mean_y", beam->get_rmean()(1) );     // 16
    row.addColumn("mean_s", beam->get_rmean()(2) );     // 17

    row.addColumn("ref_x", beam->RefPartR_m(0));        // 18
    row.addColumn("ref_y", beam->RefPartR_m(1));        // 19
    row.addColumn("ref_z", beam->RefPartR_m(2));        // 20

    row.addColumn("ref_px", beam->RefPartP_m(0));       // 21
    row.addColumn("ref_py", beam->RefPartP_m(1));       // 22
    row.addColumn("ref_pz", beam->RefPartP_m(2));       // 23

    row.addColumn("max_x", beam->get_maxExtent()(0));   // 24
    row.addColumn("max_y", beam->get_maxExtent()(1));   // 25
    row.addColumn("max_s", beam->get_maxExtent()(2));   // 26

    // Write out Courant Snyder parameters.
    row.addColumn("xpx", beam->get_rprms()(0));         // 27
    row.addColumn("ypy", beam->get_rprms()(1));         // 28
    row.addColumn("zpz", beam->get_rprms()(2));         // 29

    // Write out dispersion.
    row.addColumn("Dx", beam->get_Dx());                // 30
    row.addColumn("DDx", beam->get_DDx());              // 31
    row.addColumn("Dy", beam->get_Dy());                // 32
    row.addColumn("DDy", beam->get_DDy());              // 33

    // Write head/reference particle/tail field information.
    row.addColumn("Bx_ref", FDext[0](0));               // 34 B-ref x
    row.addColumn("By_ref", FDext[0](1));               // 35 B-ref y
    row.addColumn("Bz_ref", FDext[0](2));               // 36 B-ref z

    row.addColumn("Ex_ref", FDext[1](0));               // 37 E-ref x
    row.addColumn("Ey_ref", FDext[1](1));               // 38 E-ref y
    row.addColumn("Ez_ref", FDext[1](2));               // 39 E-ref z

    row.addColumn("dE", beam->getdE());                 // 40 dE energy spread
    row.addColumn("dt", beam->getdT() * 1e9);           // 41 dt time step size
    row.addColumn("partsOutside", npOutside);           // 42 number of particles outside n*sigma

    if (OpalData::getInstance()->isInOPALCyclMode()) {
        if (Ippl::getNodes() == 1) {
            if (beam->getLocalNum() > 0) {
                row.addColumn("R0_x", beam->R[0](0));
                row.addColumn("R0_y", beam->R[0](1));
                row.addColumn("R0_s", beam->R[0](2));
                row.addColumn("P0_x", beam->P[0](0));
                row.addColumn("P0_y", beam->P[0](1));
                row.addColumn("P0_s", beam->P[0](2));
            } else {
                row.addColumn("R0_x", 0.0);
                row.addColumn("R0_y", 0.0);
                row.addColumn("R0_s", 0.0);
                row.addColumn("P0_x", 0.0);
                row.addColumn("P0_y", 0.0);
                row.addColumn("P0_s", 0.0);
            }
        }
        Vector_t halo = beam->get_halo();
        row.addColumn("halo_x", halo(0));
        row.addColumn("halo_y", halo(1));
        row.addColumn("halo_z", halo(2));

        row.addColumn("azimuth", azimuth);
    }

    for(size_t i = 0; i < losses.size(); ++ i) {
        long unsigned int loss = losses[i].second;
        row.addColumn(losses[i].first, loss);
    }

    this->writeRow(row);

    this->newline();

    this->close();

    /// %Stop timer.
    IpplTimings::stopTimer(StatMarkerTimer_m);
}


void StatWriter::write(EnvelopeBunch &beam, Vector_t FDext[],
                       double sposHead, double sposRef, double sposTail)
{
    //FIXME https://gitlab.psi.ch/OPAL/src/issues/245

    /// Function steps:
    /// Start timer.
    IpplTimings::startTimer(StatMarkerTimer_m);

    /// Calculate beam statistics and gather load balance statistics.
    beam.calcBeamParameters();
    beam.gatherLoadBalanceStatistics();

    double en = beam.get_meanKineticEnergy() * 1e-6;
    double Q  = beam.getTotalNum() * beam.getChargePerParticle();


    if (Ippl::myNode() != 0) {
        IpplTimings::stopTimer(StatMarkerTimer_m);
        return;
    }

    fillHeader_m();

    this->open();

    this->writeHeader();

    this->writeValue(beam.getT() * 1e9);   // 1
    this->writeValue(sposRef);             // 2
    this->writeValue(beam.getTotalNum());  // 3
    this->writeValue(Q);                   // 4
    this->writeValue(en);                  // 5

    this->writeValue(beam.get_rrms()(0));  // 6
    this->writeValue(beam.get_rrms()(1));  // 7
    this->writeValue(beam.get_rrms()(2));  // 8

    this->writeValue(beam.get_prms()(0));  // 9
    this->writeValue(beam.get_prms()(1));  // 10
    this->writeValue(beam.get_prms()(2));  // 11

    this->writeValue(beam.get_norm_emit()(0)); // 12
    this->writeValue(beam.get_norm_emit()(1)); // 13
    this->writeValue(beam.get_norm_emit()(2)); // 14

    this->writeValue(beam.get_rmean()(0) );    // 15
    this->writeValue(beam.get_rmean()(1) );    // 16
    this->writeValue(beam.get_rmean()(2) );    // 17

    this->writeValue(0); // 18
    this->writeValue(0); // 19
    this->writeValue(0); // 20

    this->writeValue(0); // 21
    this->writeValue(0); // 22
    this->writeValue(0); // 23

    this->writeValue(beam.get_maxExtent()(0)); // 24
    this->writeValue(beam.get_maxExtent()(1)); // 25
    this->writeValue(beam.get_maxExtent()(2)); // 26

    // Write out Courant Snyder parameters.
    this->writeValue(0);     // 27
    this->writeValue(0);     // 28
    this->writeValue(0);     // 29

    // Write out dispersion.
    this->writeValue(beam.get_Dx());      // 30
    this->writeValue(beam.get_DDx());     // 31
    this->writeValue(beam.get_Dy());      // 32
    this->writeValue(beam.get_DDy());     // 33

    // Write head/reference particle/tail field information.
    this->writeValue(FDext[0](0));         // 34 B-ref x
    this->writeValue(FDext[0](1));         // 35 B-ref y
    this->writeValue(FDext[0](2));         // 36 B-ref z

    this->writeValue(FDext[1](0));         // 37 E-ref x
    this->writeValue(FDext[1](1));         // 38 E-ref y
    this->writeValue(FDext[1](2));         // 39 E-ref z

    this->writeValue(beam.get_dEdt());  // 40 dE energy spread
    this->writeValue(0);                // 41 dt time step size
    this->writeValue(0);                // 42 number of particles outside n*sigma

    this->newline();

    this->close();

    /// %Stop timer.
    IpplTimings::stopTimer(StatMarkerTimer_m);
}