//
// Class ParticleMatterInteraction
//   The class for the OPAL PARTICLEMATTERINTERACTION command.
//
// Copyright (c) 2012-2021, Andreas Adelmann, Paul Scherrer Institut, Villigen PSI, Switzerland
//                          Christof Metzger-Kraus, Helmholtz-Zentrum Berlin
//                          Pedro Calvo, CIEMAT, Spain
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
#include "Structure/ParticleMatterInteraction.h"

#include "AbsBeamline/ElementBase.h"
#include "AbstractObjects/OpalData.h"
#include "Attributes/Attributes.h"
#include "Physics/Physics.h"
#include "Solvers/BeamStrippingPhysics.hh"
#include "Solvers/ScatteringPhysics.hh"
#include "Utilities/OpalException.h"
#include "Utilities/Util.h"

extern Inform* gmsg;

namespace {
    enum {
        // DESCRIPTION OF PARTICLE MATTER INTERACTION:
        TYPE,
        MATERIAL,
        ENABLERUTHERFORD,
        LOWENERGYTHR,
        SIZE
    };
}

ParticleMatterInteraction::ParticleMatterInteraction():
    Definition(SIZE, "PARTICLEMATTERINTERACTION",
               "The \"PARTICLEMATTERINTERACTION\" statement defines data for "
               "the particle matter interaction handler on an element."),
    handler_m(0) {
    itsAttr[TYPE] = Attributes::makeUpperCaseString
        ("TYPE", "Specifies the particle matter interaction handler: SCATTERING, BEAMSTRIPPING");

    itsAttr[MATERIAL] = Attributes::makeUpperCaseString
        ("MATERIAL", "The material of the surface");

    itsAttr[ENABLERUTHERFORD] = Attributes::makeBool
        ("ENABLERUTHERFORD", "Enable large angle scattering", true);

    itsAttr[LOWENERGYTHR] = Attributes::makeReal
        ("LOWENERGYTHR", "Lower Energy threshold for energy loss calculation [MeV]. Default = 0.01 MeV", 0.01);

    ParticleMatterInteraction* defParticleMatterInteraction = clone("UNNAMED_PARTICLEMATTERINTERACTION");
    defParticleMatterInteraction->builtin = true;

    try {
        defParticleMatterInteraction->update();
        OpalData::getInstance()->define(defParticleMatterInteraction);
    } catch(...) {
        delete defParticleMatterInteraction;
    }

    registerOwnership(AttributeHandler::STATEMENT);
}


ParticleMatterInteraction::ParticleMatterInteraction(const std::string& name, ParticleMatterInteraction* parent):
    Definition(name, parent),
    handler_m(parent->handler_m)
{}


ParticleMatterInteraction::~ParticleMatterInteraction() {
    if (handler_m)
        delete handler_m;
}


bool ParticleMatterInteraction::canReplaceBy(Object* object) {
    // Can replace only by another PARTICLEMATTERINTERACTION.
    return dynamic_cast<ParticleMatterInteraction*>(object) != 0;
}


ParticleMatterInteraction* ParticleMatterInteraction::clone(const std::string& name) {
    return new ParticleMatterInteraction(name, this);
}


void ParticleMatterInteraction::execute() {
    update();
}


ParticleMatterInteraction* ParticleMatterInteraction::find(const std::string& name) {
    ParticleMatterInteraction* parmatint = dynamic_cast<ParticleMatterInteraction*>(OpalData::getInstance()->find(name));

    if (parmatint == 0) {
        throw OpalException("ParticleMatterInteraction::find()", "ParticleMatterInteraction \"" + name + "\" not found.");
    }
    return parmatint;
}


void ParticleMatterInteraction::update() {
    // Set default name.
    if (getOpalName().empty()) setOpalName("UNNAMED_PARTICLEMATTERINTERACTION");
}


void ParticleMatterInteraction::initParticleMatterInteractionHandler(ElementBase& element) {

    const std::string type = Attributes::getString(itsAttr[TYPE]);
    std::string material   = Attributes::getString(itsAttr[MATERIAL]);
    bool enableRutherford  = Attributes::getBool(itsAttr[ENABLERUTHERFORD]);
    double lowEnergyThr    = Attributes::getReal(itsAttr[LOWENERGYTHR]);

    if (type.empty()) {
        throw OpalException("ParticleMatterInteraction::initParticleMatterInteractionHandler",
                            "TYPE is not defined for PARTICLEMATTERINTERACTION");    
    } else if (type == "SCATTERING") {
         handler_m = new ScatteringPhysics(getOpalName(), &element, material, enableRutherford, lowEnergyThr);
         *gmsg << *this << endl;
    } else if (type == "BEAMSTRIPPING") {
         handler_m = new BeamStrippingPhysics(getOpalName(), &element);
         *gmsg << *this << endl;
    } else {
        throw OpalException("ParticleMatterInteraction::initParticleMatterInteractionHandler",
                            getOpalName() + ": TYPE == " + Attributes::getString(itsAttr[TYPE])
                            + " is not defined!");
    }
}

void ParticleMatterInteraction::updateElement(ElementBase* element) {
    handler_m->updateElement(element);
}

void ParticleMatterInteraction::print(std::ostream& os) const {
    os << "* ************* P A R T I C L E  M A T T E R  I N T E R A C T I O N ****************** " << std::endl;
    os << "* PARTICLEMATTERINTERACTION " << getOpalName() << '\n'
       << "* TYPE           " << Attributes::getString(itsAttr[TYPE]) << '\n'
       << "* ELEMENT        " << handler_m->getElement()->getName() << '\n'; 
    if ( Attributes::getString(itsAttr[TYPE]) == "SCATTERING" ) {
        os << "* MATERIAL       " << Attributes::getString(itsAttr[MATERIAL]) << '\n';
        os << "* LOWENERGYTHR   " << Attributes::getReal(itsAttr[LOWENERGYTHR]) << " MeV" << '\n';
    }
    os << "* ********************************************************************************** " << std::endl;
}
