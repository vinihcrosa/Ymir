#pragma once

#include <cstddef>
#include <vector>

#include <ymir/vessel/NavalContext.h>
#include <ymir/vessel/Thruster.h>
#include <ymir/vessel/Rudder.h>

namespace ymir::naval
{

/**
 * Autonomous waypoint-following controller using Line-of-Sight (LOS) bearing
 * and two independent PID loops for heading and speed.
 *
 * Satisfies the implicit VesselController concept required by std::variant dispatch
 * (ADR-002): void update(double t, double dt, const NavalContext&,
 *                        std::vector<Thruster>&, std::vector<Rudder>&).
 *
 * When the waypoint list is exhausted, zero demands are applied to all actuators
 * (drift mode).
 */
class ManeuverController
{
public:
    /** A geographic target with an associated speed setpoint. */
    struct Waypoint
    {
        double x_m;               ///< inertial x position [m]
        double y_m;               ///< inertial y position [m]
        double demandedSpeed_mps; ///< desired speed-over-ground when approaching [m/s]
    };

    struct Config
    {
        std::vector<Waypoint> waypoints;
        double captureRadius_m = 50.0; ///< distance at which a waypoint is considered captured [m]

        double headingKp = 0.0; ///< proportional gain for heading PID [rad/rad]
        double headingKi = 0.0; ///< integral gain for heading PID [rad/(rad·s)]
        double headingKd = 0.0; ///< derivative gain for heading PID [rad·s/rad]

        double speedKp = 0.0; ///< proportional gain for speed PID [RPM/(m/s)]
        double speedKi = 0.0; ///< integral gain for speed PID [RPM/(m/s·s)]
        double speedKd = 0.0; ///< derivative gain for speed PID [RPM·s/(m/s)]
    };

    explicit ManeuverController(Config cfg);

    /**
     * Compute LOS bearing to the active waypoint, run heading and speed PIDs,
     * and push demands to all thrusters and rudders.
     *
     * Heading error: wrapToPi(atan2(dy, dx) - yaw).
     * Speed-over-ground: sqrt(u^2 + v^2) from body-frame velocities.
     * When the waypoint list is exhausted, zero demands are applied.
     */
    void update(double t, double dt, const NavalContext& ctx,
                std::vector<Thruster>& thrusters, std::vector<Rudder>& rudders);

    /** True after the last waypoint has been captured. */
    bool waypointsExhausted() const noexcept;

    /** Index of the waypoint currently being tracked. */
    std::size_t activeWaypointIdx() const noexcept;

private:
    struct PID
    {
        double kp, ki, kd;
        double integral_  = 0.0;
        double prevError_ = 0.0;

        /** Returns kp*error + ki*integral + kd*(error - prevError)/dt. */
        double update(double error, double dt) noexcept;

        /** Resets integral and previous error to zero. */
        void reset() noexcept;
    };

    Config      cfg_;
    std::size_t activeWpt_ = 0;
    PID         headingPid_;
    PID         speedPid_;
};

} // namespace ymir::naval
