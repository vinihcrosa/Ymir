#pragma once

#include <ymir/physics/BodyState.h>
#include <ymir/common/Types.h>

namespace ymir::naval
{

/**
 * Per-step snapshot of body kinematics plus naval environment derived quantities.
 *
 * Built by NavalSimulation at the start of each step; passed as const& to all
 * NavalForceModel::computeNaval() implementations. Never modified inside compute().
 */
struct NavalContext
{
    BodyState state;               // M1 kinematic snapshot (q, qdot, time, dt)
    Vector6   speedToWater{};      // body-frame velocity relative to current (m/s)
    Vector6   speedToWind{};       // body-frame wind velocity (m/s, positive = blowing toward +x/+y)
    Vector6   q_avg{};             // EMA-smoothed position, τ=16.5s (used by wave engine)
    double    waterDepth = 100.0;  // m — positive downward
    double    tide       = 0.0;    // m — positive = raised water level
};

} // namespace ymir::naval
