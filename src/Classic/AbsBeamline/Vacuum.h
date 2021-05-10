//
// Class Vacuum
//   Defines the abstract interface environment for
//   beam stripping physics.
//
// Copyright (c) 2018-2019, Pedro Calvo, CIEMAT, Spain
// All rights reserved
//
// Implemented as part of the PhD thesis
// "Optimizing the radioisotope production of the novel AMIT
// superconducting weak focusing cyclotron"
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
#ifndef CLASSIC_Vacuum_HH
#define CLASSIC_Vacuum_HH

#include "AbsBeamline/Component.h"

#include <string>
#include <vector>

class BeamlineVisitor;
class Cyclotron;

struct PFieldData {
    std::string filename;
    // pressure field known from file
    std::vector<double> pfld;

    // Grid-Size
    //need to be read from inputfile.
    int nrad, ntet;

    // one more grid line is stored in azimuthal direction:
    int ntetS;

    // total grid points number.
    int ntot;
};

struct PPositions {
    // these 4 parameters are need to be read from field file.
    double  rmin, delr;
    double  tetmin, dtet;

    // Radii and step width of initial Grid
    std::vector<double> rarr;

    double  Pfact; // MULTIPLICATION FACTOR FOR PRESSURE MAP
};

enum class ResidualGas:short {
    NOGAS = -1,
    AIR   = 0,
    H2    = 1
};

class Vacuum: public Component {

public:

    /// Constructor with given name.
    explicit Vacuum(const std::string& name);

    Vacuum();
    Vacuum(const Vacuum& rhs);
    virtual ~Vacuum();

    /// Apply visitor to Vacuum.
    virtual void accept(BeamlineVisitor&) const;

    virtual bool checkVacuum(PartBunchBase<double, 3>* bunch,
                                    Cyclotron* cycl);

    virtual void initialise(PartBunchBase<double, 3>* bunch,
                            double& startField, double& endField);

    virtual void initialise(PartBunchBase<double, 3>* bunch,
                            const double& scaleFactor);

    virtual void finalise();

    virtual bool bends() const;

    virtual void goOnline(const double& kineticEnergy);
    virtual void goOffline();

    virtual ElementBase::ElementType getType() const;

    virtual void getDimensions(double& zBegin, double& zEnd) const;

    std::string  getVacuumShape();

    int checkPoint(const double& x, const double& y, const double& z);
    
    double checkPressure(const double& x, const double& y);

    void setPressure(double pressure);
    double getPressure() const;

    void setTemperature(double temperature);
    double getTemperature() const;

    void setPressureMapFN(std::string pmapfn);
    virtual std::string getPressureMapFN() const;
    
    void setPScale(double ps);
    virtual double getPScale() const;

    void setResidualGas(std::string gas);
    ResidualGas getResidualGas() const;
    std::string getResidualGasName();

    void setStop(bool stopflag);
    virtual bool getStop() const;

protected:

    void   initR(double rmin, double dr, int nrad);

    void   getPressureFromFile(const double &scaleFactor);

    inline int idx(int irad, int ktet) {return (ktet + PField.ntetS * irad);}

private:

    ///@{ parameters for Vacuum
    ResidualGas gas_m;
    double pressure_m;    /// mbar
    std::string pmapfn_m; /// stores the filename of the pressure map
    double pscale_m;      /// a scale factor for the P-field
    double temperature_m; /// K
    bool stop_m;          /// Flag if particles should be stripped or stopped
    ///@}

    ///@{ size limits took from cyclotron
    double minr_m;   /// mm
    double maxr_m;   /// mm
    double minz_m;   /// mm
    double maxz_m;   /// mm
    ///@}

    ParticleMatterInteractionHandler* parmatint_m;

protected:
    // object of Matrices including pressure field map and its derivates
    PFieldData PField;

    // object of parameters about the map grid
    PPositions PP;
};

#endif // CLASSIC_Vacuum_HH
