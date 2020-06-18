//
// Class OpalDegrader
//   The DEGRADER element.
//
// Copyright (c) 200x - 2020, Paul Scherrer Institut, Villigen PSI, Switzerland
// All rights reserved
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
#include "Elements/OpalDegrader.h"
#include "Attributes/Attributes.h"
#include "BeamlineCore/DegraderRep.h"
#include "Structure/ParticleMatterInteraction.h"


OpalDegrader::OpalDegrader():
    OpalElement(SIZE, "DEGRADER",
                "The \"DEGRADER\" element defines a degrader."),
    parmatint_m(NULL) {
    itsAttr[XSIZE] = Attributes::makeReal
        ("XSIZE", "not used",0.0);
    itsAttr[YSIZE] = Attributes::makeReal
        ("YSIZE", "not used",0.0);
    itsAttr[OUTFN] = Attributes::makeString
        ("OUTFN", "Degrader output filename");

    registerStringAttribute("OUTFN");

    registerOwnership();

    setElement(new DegraderRep("DEGRADER"));
}


OpalDegrader::OpalDegrader(const std::string &name, OpalDegrader *parent):
    OpalElement(name, parent),
    parmatint_m(NULL) {
    setElement(new DegraderRep(name));
}


OpalDegrader::~OpalDegrader() {
    if(parmatint_m)
        delete parmatint_m;
}


OpalDegrader *OpalDegrader::clone(const std::string &name) {
    return new OpalDegrader(name, this);
}


void OpalDegrader::fillRegisteredAttributes(const ElementBase &base, ValueFlag flag) {
    OpalElement::fillRegisteredAttributes(base, flag);
}


void OpalDegrader::update() {
    OpalElement::update();

    DegraderRep *deg =
        dynamic_cast<DegraderRep *>(getElement()->removeWrappers());
    double length = Attributes::getReal(itsAttr[LENGTH]);
    deg->setElementLength(length);

    deg->setOutputFN(Attributes::getString(itsAttr[OUTFN]));

    if(itsAttr[PARTICLEMATTERINTERACTION] && parmatint_m == NULL) {
        parmatint_m = (ParticleMatterInteraction::find(Attributes::getString(itsAttr[PARTICLEMATTERINTERACTION])))->clone(getOpalName() + std::string("_parmatint"));
        parmatint_m->initParticleMatterInteractionHandler(*deg);
        deg->setParticleMatterInteraction(parmatint_m->handler_m);
    }

    // Transmit "unknown" attributes.
    OpalElement::updateUnknown(deg);
}