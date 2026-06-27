#include <ymir/vessel/controllers/ManeuverController.h>

#include <ymir/common/math/AngleUtils.h>

#include <cmath>

namespace ymir::naval
{

// ---------------------------------------------------------------------------
// PID
// ---------------------------------------------------------------------------

double ManeuverController::PID::update(double error, double dt) noexcept
{
    integral_ += error * dt;
    const double derivative = (dt > 0.0) ? (error - prevError_) / dt : 0.0;
    prevError_ = error;
    return kp * error + ki * integral_ + kd * derivative;
}

void ManeuverController::PID::reset() noexcept
{
    integral_  = 0.0;
    prevError_ = 0.0;
}

// ---------------------------------------------------------------------------
// ManeuverController
// ---------------------------------------------------------------------------

ManeuverController::ManeuverController(Config cfg)
    : cfg_(std::move(cfg))
    , headingPid_{cfg_.headingKp, cfg_.headingKi, cfg_.headingKd}
    , speedPid_  {cfg_.speedKp,   cfg_.speedKi,   cfg_.speedKd}
{}

void ManeuverController::update(double /*t*/, double dt, const NavalContext& ctx,
                                std::vector<Thruster>& thrusters,
                                std::vector<Rudder>&   rudders)
{
    const auto& s = ctx.state;

    // Advance past any waypoints that are already within capture radius.
    while (activeWpt_ < cfg_.waypoints.size())
    {
        const Waypoint& wp = cfg_.waypoints[activeWpt_];
        const double dx    = wp.x_m - s.x();
        const double dy    = wp.y_m - s.y();
        if (std::sqrt(dx * dx + dy * dy) >= cfg_.captureRadius_m)
            break;
        ++activeWpt_;
        headingPid_.reset();
        speedPid_.reset();
    }

    if (activeWpt_ >= cfg_.waypoints.size())
    {
        for (auto& thr : thrusters)
            thr.setDemand(0.0, thr.state().demandedPitch, thr.state().demandedAzimuth_deg);
        for (auto& rud : rudders)
            rud.setDemand(0.0);
        return;
    }

    const Waypoint& wp = cfg_.waypoints[activeWpt_];
    const double dx    = wp.x_m - s.x();
    const double dy    = wp.y_m - s.y();

    // LOS bearing and heading error
    const double bearing      = std::atan2(dy, dx);
    const double headingError = ymir::math::wrapToPi(bearing - s.yaw());
    const double rudderDemand = headingPid_.update(headingError, dt);

    for (auto& rud : rudders)
        rud.setDemand(rudderDemand);

    // Speed-over-ground and speed error
    const double sog        = std::sqrt(s.u() * s.u() + s.v() * s.v());
    const double speedError = wp.demandedSpeed_mps - sog;
    const double rpmDemand  = speedPid_.update(speedError, dt);

    for (auto& thr : thrusters)
        thr.setDemand(rpmDemand, thr.state().demandedPitch, thr.state().demandedAzimuth_deg);
}

bool ManeuverController::waypointsExhausted() const noexcept
{
    return activeWpt_ >= cfg_.waypoints.size();
}

std::size_t ManeuverController::activeWaypointIdx() const noexcept
{
    return activeWpt_;
}

} // namespace ymir::naval
