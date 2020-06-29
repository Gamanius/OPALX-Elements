//
// Class for dynamic non-scaled 3D field-maps stored in H5hut files.
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

#include "Fields/FM3DH5Block_nonscale.h"
#include "Physics/Physics.h"
#include "Utilities/GeneralClassicException.h"

FM3DH5Block_nonscale::FM3DH5Block_nonscale (
    std::string aFilename) : FM3DH5BlockBase (
        aFilename
        ) {
        Type = T3DDynamicH5Block;

        OpenFileMPIOCollective (aFilename);
        GetFieldInfo ("Efield");
        GetResonanceFrequency ();
        CloseFile ();
}

FM3DH5Block_nonscale::~FM3DH5Block_nonscale (
    ) {
    freeMap();
}

void FM3DH5Block_nonscale::readMap (
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
    FieldstrengthHx_m.resize (field_size);
    FieldstrengthHy_m.resize (field_size);
    FieldstrengthHz_m.resize (field_size);

    ReadField (
        "Efield",
        &(FieldstrengthEx_m[0]),
        &(FieldstrengthEy_m[0]),
        &(FieldstrengthEz_m[0]));
    ReadField (
        "Hfield",
        &(FieldstrengthHx_m[0]),
        &(FieldstrengthHy_m[0]),
        &(FieldstrengthHz_m[0]));

    CloseFile ();

    for (long long i = 0; i < num_gridpx_m * num_gridpy_m * num_gridpz_m; i++) {
        FieldstrengthEz_m[i] *= 1.0e6 ;
        FieldstrengthEx_m[i] *= 1.0e6 ;
        FieldstrengthEy_m[i] *= 1.0e6 ;
        FieldstrengthHx_m[i] *= 1.0e6 * Physics::mu_0 ;
        FieldstrengthHy_m[i] *= 1.0e6 * Physics::mu_0 ;
        FieldstrengthHz_m[i] *= 1.0e6 * Physics::mu_0 ;
    }
    INFOMSG (level3 << typeset_msg("fieldmap '" + Filename_m  + "' read", "info")
             << endl);
}

void FM3DH5Block_nonscale::freeMap (
    ) {
    if(FieldstrengthEz_m.empty ()) {
        return;
    }
    FieldstrengthEx_m.clear ();
    FieldstrengthEy_m.clear ();
    FieldstrengthEz_m.clear ();
    FieldstrengthHx_m.clear ();
    FieldstrengthHy_m.clear ();
    FieldstrengthHz_m.clear ();

    INFOMSG(level3 << typeset_msg("freed fieldmap '" + Filename_m + "'", "info")
            << endl);
}

bool FM3DH5Block_nonscale::getFieldstrength (
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
        FieldstrengthHx_m, FieldstrengthHy_m, FieldstrengthHz_m, R);

    return false;
}

// vi: set et ts=4 sw=4 sts=4:
// Local Variables:
// mode:c
// c-basic-offset: 4
// indent-tabs-mode: nil
// require-final-newline: nil
// End:
