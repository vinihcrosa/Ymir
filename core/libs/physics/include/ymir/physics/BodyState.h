#pragma once

#include <ymir/common/Types.h>

#include <array>

namespace ymir
{

/**
 * Immutable snapshot of a Body's kinematics at a point in time.
 * Passed by const& to all ForceModel::compute() calls.
 *
 * State layout:
 *   q[0..2]    = x, y, z      (position, inertial frame, metres)
 *   q[3..5]    = φ, θ, ψ      (roll, pitch, yaw, radians)
 *   qdot[0..2] = u, v, w      (surge, sway, heave velocity, body frame, m/s)
 *   qdot[3..5] = p, q_rot, r  (roll rate, pitch rate, yaw rate, body frame, rad/s)
 */
class BodyState
{
public:
    BodyState() noexcept = default;

    BodyState(Vector6 q, Vector6 qdot, double time, double dt) noexcept
        : q_(q), qdot_(qdot), time_(time), dt_(dt)
    {
    }

    // Position (inertial frame)
    double x()     const noexcept { return q_[0]; }
    double y()     const noexcept { return q_[1]; }
    double z()     const noexcept { return q_[2]; }
    double roll()  const noexcept { return q_[3]; }
    double pitch() const noexcept { return q_[4]; }
    double yaw()   const noexcept { return q_[5]; }

    // Velocity (body frame)
    double u()     const noexcept { return qdot_[0]; }
    double v()     const noexcept { return qdot_[1]; }
    double w()     const noexcept { return qdot_[2]; }
    double p()     const noexcept { return qdot_[3]; }
    double q_rot() const noexcept { return qdot_[4]; }
    double r()     const noexcept { return qdot_[5]; }

    // Raw access
    const Vector6& q()    const noexcept { return q_; }
    const Vector6& qdot() const noexcept { return qdot_; }
    double         time() const noexcept { return time_; }
    double         dt()   const noexcept { return dt_; }

    std::array<double, 3> bodyFrameVelocity()    const noexcept { return {qdot_[0], qdot_[1], qdot_[2]}; }
    std::array<double, 3> bodyFrameAngularRate() const noexcept { return {qdot_[3], qdot_[4], qdot_[5]}; }

private:
    Vector6 q_;
    Vector6 qdot_;
    double  time_;
    double  dt_;
};

} // namespace ymir
