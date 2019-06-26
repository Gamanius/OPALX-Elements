//
//  Copyright & License: See Copyright.readme in src directory
//

/**
   \brief Class: DataSink

   This class acts as an observer during the calculation. It generates diagnostic
   output of the accelerated beam such as statistical beam descriptors of particle
   positions, momenta, beam phase space (emittance) etc. These are written to file
   at periodic time steps during the calculation.

   This class also writes the full beam phase space to an H5 file at periodic time
   steps in the calculation (this period is different from that of the statistical
   numbers).

   Class also writes processor load balancing data to file to track parallel
   calculation efficiency.
*/

#ifndef _OPAL_DATA_SINK_H
#define _OPAL_DATA_SINK_H

#include "Algorithms/PBunchDefs.h"

#include "StatWriter.h"
#include "H5Writer.h"

template <class T, unsigned Dim>
class PartBunchBase;
class BoundaryGeometry;
class H5PartWrapper;

class DataSink {
public:
    typedef StatWriter::losses_t        losses_t;
    typedef std::unique_ptr<StatWriter> statWriter_t;
    typedef std::unique_ptr<SDDSWriter> sddsWriter_t;
    typedef std::unique_ptr<H5Writer>   h5Writer_t;
    
    
    /** \brief Default constructor.
     *
     * The default constructor is called at the start of a new calculation (as
     * opposed to a calculation restart).
     */
    DataSink();
    DataSink(H5PartWrapper *h5wrapper, bool restart);
    DataSink(H5PartWrapper *h5wrapper);

    void dumpH5(PartBunchBase<double, 3> *beam, Vector_t FDext[]) const;
    
    int dumpH5(PartBunchBase<double, 3> *beam, Vector_t FDext[], double meanEnergy,
               double refPr, double refPt, double refPz,
               double refR, double refTheta, double refZ,
               double azimuth, double elevation, bool local) const;
    
    //FIXME https://gitlab.psi.ch/OPAL/src/issues/245
    void dumpH5(EnvelopeBunch &beam, Vector_t FDext[],
                double sposHead, double sposRef,
                double sposTail) const;
    
    
    void dumpSDDS(PartBunchBase<double, 3> *beam, Vector_t FDext[],
                  const double& azimuth = -1) const;
    
    void dumpSDDS(PartBunchBase<double, 3> *beam, Vector_t FDext[],
                  const losses_t &losses = losses_t(), const double& azimuth = -1) const;
    
    //FIXME https://gitlab.psi.ch/OPAL/src/issues/245
    void dumpSDDS(EnvelopeBunch &beam, Vector_t FDext[],
                  double sposHead, double sposRef, double sposTail) const;

    /** \brief Write cavity information from  H5 file
     */
    void storeCavityInformation();
    
    void changeH5Wrapper(H5PartWrapper *h5wrapper);
    
    
    /**
     * Write particle loss data to an ASCII fille for histogram
     * @param fn specifies the name of ASCII file
     * @param beam
     */
    void writePartlossZASCII(PartBunchBase<double, 3> *beam, BoundaryGeometry &bg, std::string fn);
    
    /**
     * Write geometry points and surface triangles to vtk file
     *
     * @param fn specifies the name of vtk file contains the geometry
     *
     */
    void writeGeomToVtk(BoundaryGeometry &bg, std::string fn);
    //void writeGeoContourToVtk(BoundaryGeometry &bg, std::string fn);
    
    
    /**
     * Write impact number and outgoing secondaries in each time step
     *
     * @param fn specifies the name of vtk file contains the geometry
     *
     */
    void writeImpactStatistics(PartBunchBase<double, 3> *beam,
                               long long int &step,
                               size_t &impact,
                               double &sey_num,
                               size_t numberOfFieldEmittedParticles,
                               bool nEmissionMode,
                               std::string fn);
    
#ifdef ENABLE_AMR
    bool writeAmrStatistics(PartBunchBase<double, 3> *beam);
    
    void noAmrDump(PartBunchBase<double, 3> *beam);
#endif

private:
    DataSink(const DataSink &) { }
    DataSink &operator = (const DataSink &) { return *this; }
    
    void rewindLines_m();
    
    void init_m(bool restart = false,
                H5PartWrapper* h5wrapper = nullptr);
    
    
    h5Writer_t      h5Writer_m;
    statWriter_t    statWriter_m;
    std::vector<sddsWriter_t> sddsWriter_m;
    
    static std::string convertToString(int number);

    /** \brief First write to the H5 surface loss file.
     *
     * If true, file name will be assigned and file will be prepared to write.
     * Variable is then reset to false so that H5 file is only initialized once.
     */
    bool firstWriteH5Surface_m;

    /// Name of output file for surface loss data.
    std::string surfaceLossFileName_m;

    /// needed to create index for vtk file
    unsigned int lossWrCounter_m;
};

// inline
// void DataSink::reset() {
//     H5call_m = 0;
// }


inline
std::string DataSink::convertToString(int number) {
    std::stringstream ss;
    ss << std::setw(5) << std::setfill('0') <<  number;
    return ss.str();
}

#endif // DataSink_H_

// vi: set et ts=4 sw=4 sts=4:
// Local Variables:
// mode:c
// c-basic-offset: 4
// indent-tabs-mode:nil
// End: