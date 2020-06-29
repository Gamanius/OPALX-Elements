//
// Class for magneto-static 3D field-maps stored in H5hut files.
//
// Copyright (c) 2020, Achim Gsell, Paul Scherrer Institut, Villigen PSI, Switzerland
// All rights reserved.
//
// This file is part of OPAL.
//
// OPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// You should have received a copy of the GNU General Public License
// along with OPAL.  If not, see <https://www.gnu.org/licenses/>.
//

#include "Fields/FM3DMagnetoStaticH5Block.h"
#include "Utilities/GeneralClassicException.h"

FM3DMagnetoStaticH5Block::FM3DMagnetoStaticH5Block (
    std::string aFilename) : FM3DH5BlockBase (
        aFilename
        ) {
        Type = T3DMagnetoStaticH5Block;

        OpenFileMPIOCollective (aFilename);
        GetFieldInfo ("Efield");
        GetResonanceFrequency ();
        CloseFile ();
}

FM3DMagnetoStaticH5Block::~FM3DMagnetoStaticH5Block (
    ) {
    freeMap ();
}

void FM3DMagnetoStaticH5Block::readMap (
    ) {
    if (!FieldstrengthEz_m.empty()) {
        return;
    }
    OpenFileMPIOCollective (Filename_m);
    long long last_step = GetNumSteps () - 1;
    SetStep (last_step);

    size_t field_size = num_gridpx_m * num_gridpy_m * num_gridpz_m;
    FieldstrengthEx_m.resize (field_size);
    FieldstrengthEy_m.resize (field_size);
    FieldstrengthEz_m.resize (field_size);
    FieldstrengthBx_m.resize (field_size);
    FieldstrengthBy_m.resize (field_size);
    FieldstrengthBz_m.resize (field_size);

    ReadField (
        "Efield",
        &(FieldstrengthEx_m[0]),
        &(FieldstrengthEy_m[0]),
        &(FieldstrengthEz_m[0]));
    ReadField (
        "Bfield",
        &(FieldstrengthBx_m[0]),
        &(FieldstrengthBy_m[0]),
        &(FieldstrengthBz_m[0]));

    CloseFile ();
    INFOMSG (level3 << typeset_msg("fieldmap '" + Filename_m  + "' read", "info")
             << endl);
}
void FM3DMagnetoStaticH5Block::freeMap (
    ) {
    if(FieldstrengthEz_m.empty()) {
        return;
    }
    FieldstrengthEx_m.clear();
    FieldstrengthEy_m.clear();
    FieldstrengthEz_m.clear();
    FieldstrengthBx_m.clear();
    FieldstrengthBy_m.clear();
    FieldstrengthBz_m.clear();
    INFOMSG(level3 << typeset_msg("freed fieldmap '" + Filename_m + "'", "info")
            << endl);
}

bool FM3DMagnetoStaticH5Block::getFieldstrength (
    const Vector_t& R,
    Vector_t& E,
    Vector_t& B
    ) const {
    if (!isInside(R)) {
        return true;
    }
    E += interpolateTrilinearly (
        FieldstrengthEx_m, FieldstrengthEy_m, FieldstrengthEz_m, R);
    B += interpolateTrilinearly (
        FieldstrengthBx_m, FieldstrengthBy_m, FieldstrengthBz_m, R);

    return false;
}

double FM3DMagnetoStaticH5Block::getFrequency (
    ) const {
    return 0.0;
}

// vi: set et ts=4 sw=4 sts=4:
// Local Variables:
// mode:c
// c-basic-offset: 4
// indent-tabs-mode: nil
// require-final-newline: nil
// End:
