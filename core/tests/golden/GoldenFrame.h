#pragma once

// -----------------------------------------------------------------------------
// GoldenFrame — earth/nautical -> body-frame conversion for golden tests.
//
// Mirrors NavalDomain::nautToBodyFrame (libs/simulation/src/NavalDomain.cpp):
// converts an environmental flow given as (speed, nautical "from" direction)
// into the vessel's body frame using the golden yaw. Used to reconstruct the
// body-frame current/wind vectors that the force models expect, from the
// controlDict environment + the golden trajectory heading.
// -----------------------------------------------------------------------------

#include <array>
#include <cmath>

namespace ymir::test
{

/// (speed, nautical-from degrees, yaw radians) -> body-frame (x, y) flow.
inline std::array<double, 2> nautToBodyFrame(double speedMag, double nautDeg, double yawRad)
{
    constexpr double kDeg2Rad = 3.14159265358979323846 / 180.0;
    const double     towardNaut = nautDeg + 180.0;
    const double     mathRad    = (90.0 - towardNaut) * kDeg2Rad;
    const double     vEast      = speedMag * std::cos(mathRad);
    const double     vNorth     = speedMag * std::sin(mathRad);
    return {std::cos(yawRad) * vEast + std::sin(yawRad) * vNorth,
            -std::sin(yawRad) * vEast + std::cos(yawRad) * vNorth};
}

} // namespace ymir::test
