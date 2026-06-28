#pragma once

#include <ymir/common/Types.h>

#include <cmath>

namespace ymir::math
{

constexpr double PI = 3.14159265358979323846;

// ---------------------------------------------------------------------------
// Radians
// ---------------------------------------------------------------------------

inline double wrapTo2Pi(double rad) noexcept
{
    rad = std::fmod(rad, 2.0 * PI);
    if (rad < 0.0)
        rad += 2.0 * PI;
    return rad;
}

inline double wrapToPi(double rad) noexcept
{
    rad = std::fmod(rad + PI, 2.0 * PI);
    if (rad < 0.0)
        rad += 2.0 * PI;
    return rad - PI;
}

// ---------------------------------------------------------------------------
// Degrees
// ---------------------------------------------------------------------------

inline double wrapTo360(double deg) noexcept
{
    deg = std::fmod(deg, 360.0);
    if (deg < 0.0)
        deg += 360.0;
    return deg;
}

inline double wrapTo180(double deg) noexcept
{
    deg = std::fmod(deg + 180.0, 360.0);
    if (deg < 0.0)
        deg += 360.0;
    return deg - 180.0;
}

// ---------------------------------------------------------------------------
// Conversion
// ---------------------------------------------------------------------------

inline double deg2rad(double deg) noexcept { return deg * PI / 180.0; }
inline double rad2deg(double rad) noexcept { return rad * 180.0 / PI; }

inline Vector6 deg2rad(const Vector6& v) noexcept
{
    return {v[0] * PI / 180.0, v[1] * PI / 180.0, v[2] * PI / 180.0,
            v[3] * PI / 180.0, v[4] * PI / 180.0, v[5] * PI / 180.0};
}

inline Vector6 rad2deg(const Vector6& v) noexcept
{
    return {v[0] * 180.0 / PI, v[1] * 180.0 / PI, v[2] * 180.0 / PI,
            v[3] * 180.0 / PI, v[4] * 180.0 / PI, v[5] * 180.0 / PI};
}

} // namespace ymir::math
