#include "Algorithms/CoordinateSystemTrafo.h"

CoordinateSystemTrafo::CoordinateSystemTrafo()
    : origin_m(0.0), orientation_m(1.0, 0.0, 0.0, 0.0), rotationMatrix_m(3, 3) {
    rotationMatrix_m(0, 0) = 1.0;
    rotationMatrix_m(0, 1) = 0.0;
    rotationMatrix_m(0, 2) = 0.0;
    rotationMatrix_m(1, 0) = 0.0;
    rotationMatrix_m(1, 1) = 1.0;
    rotationMatrix_m(1, 2) = 0.0;
    rotationMatrix_m(2, 0) = 0.0;
    rotationMatrix_m(2, 1) = 0.0;
    rotationMatrix_m(2, 2) = 1.0;
}

CoordinateSystemTrafo::CoordinateSystemTrafo(const CoordinateSystemTrafo& right)
    : origin_m(right.origin_m),
      orientation_m(right.orientation_m),
      rotationMatrix_m(right.rotationMatrix_m) {
}

CoordinateSystemTrafo::CoordinateSystemTrafo(
    const ippl::Vector<double, 3>& origin, const Quaternion& orientation)
    : origin_m(origin),
      orientation_m(orientation),
      rotationMatrix_m(orientation_m.getRotationMatrix()) {
}

void CoordinateSystemTrafo::invert() {
    origin_m      = -orientation_m.rotate(origin_m);
    orientation_m = orientation_m.conjugate();
    // \todo    rotationMatrix_m = transpose(rotationMatrix_m);
    rotationMatrix_m = boost::numeric::ublas::trans(rotationMatrix_m);
}

CoordinateSystemTrafo CoordinateSystemTrafo::operator*(const CoordinateSystemTrafo& right) const {
    CoordinateSystemTrafo result(*this);

    result *= right;
    return result;
}

void CoordinateSystemTrafo::operator*=(const CoordinateSystemTrafo& right) {
    origin_m = right.orientation_m.conjugate().rotate(origin_m) + right.origin_m;
    orientation_m *= right.orientation_m;
    orientation_m.normalize();
    rotationMatrix_m = orientation_m.getRotationMatrix();
}
