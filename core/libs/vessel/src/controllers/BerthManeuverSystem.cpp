#include <ymir/vessel/controllers/BerthManeuverSystem.h>

#include <ymir/common/math/AngleUtils.h>
#include <ymir/world/CouplingRegistry.h>

#include <algorithm>
#include <cassert>
#include <cmath>

namespace ymir::naval
{

// ---------------------------------------------------------------------------
// Helper: build the internal ManeuverController for the Navigating phase.
// captureRadius = 0 so the MC never auto-advances waypoints; BSM owns all transitions.
// ---------------------------------------------------------------------------

static ManeuverController buildNavMc(const BerthManeuverSystem::Config& cfg)
{
    ManeuverController::Config mcCfg;
    mcCfg.captureRadius_m = 0.0;
    mcCfg.headingKp       = cfg.headingKp;
    mcCfg.headingKi       = cfg.headingKi;
    mcCfg.headingKd       = cfg.headingKd;
    mcCfg.speedKp = mcCfg.speedKi = mcCfg.speedKd = 0.0;
    for (const auto& wp : cfg.waypoints)
        mcCfg.waypoints.push_back({wp.x_m, wp.y_m, 0.0});
    return ManeuverController(std::move(mcCfg));
}

// ---------------------------------------------------------------------------
// PID
// ---------------------------------------------------------------------------

double BerthManeuverSystem::PID::update(double error, double dt) noexcept
{
    integral_ += error * dt;
    const double derivative = (dt > 0.0) ? (error - prevError_) / dt : 0.0;
    prevError_ = error;
    return kp * error + ki * integral_ + kd * derivative;
}

void BerthManeuverSystem::PID::reset() noexcept
{
    integral_  = 0.0;
    prevError_ = 0.0;
}

// ---------------------------------------------------------------------------
// BerthManeuverSystem
// ---------------------------------------------------------------------------

BerthManeuverSystem::BerthManeuverSystem(Config cfg)
    : cfg_(std::move(cfg))
    , navMc_(buildNavMc(cfg_))
    , headingPid_{cfg_.headingKp, cfg_.headingKi, cfg_.headingKd}
    , rotRatePid_{cfg_.rotRateKp, cfg_.rotRateKi, cfg_.rotRateKd}
{}

// ---------------------------------------------------------------------------

void BerthManeuverSystem::setCouplingRegistry(ymir::CouplingRegistry* registry,
                                               int shipBodyId,
                                               const std::vector<int>& tugBodyIds)
{
    if (registry != nullptr)
        assert(tugBodyIds.size() == cfg_.tugs.size());

    registry_   = registry;
    shipBodyId_ = shipBodyId;
    tugBodyIds_ = tugBodyIds;
}

// ---------------------------------------------------------------------------

void BerthManeuverSystem::update(double t, double dt, const NavalContext& ctx,
                                  std::vector<Thruster>& thrusters,
                                  std::vector<Rudder>&   rudders)
{
    tugForces_ = {};

    checkTransitions(ctx);

    switch (phase_)
    {
        case Phase::Navigating:  updateNavigating (t, dt, ctx, thrusters, rudders); break;
        case Phase::Sideway:     updateSideway    (t, dt, ctx, thrusters, rudders); break;
        case Phase::TurnROTTUG:  updateTurnROTTUG (t, dt, ctx, thrusters, rudders); break;
    }
}

// ---------------------------------------------------------------------------

void BerthManeuverSystem::checkTransitions(const NavalContext& ctx)
{
    const auto& s = ctx.state;

    if (phase_ == Phase::Navigating && activeWpt_ < cfg_.waypoints.size())
    {
        const auto& wp  = cfg_.waypoints[activeWpt_];
        const double dx = wp.x_m - s.x();
        const double dy = wp.y_m - s.y();
        if (std::sqrt(dx * dx + dy * dy) < wp.transitionDist_m)
        {
            phase_ = Phase::Sideway;
            headingPid_.reset();
        }
    }
    else if (phase_ == Phase::Sideway && activeWpt_ < cfg_.waypoints.size())
    {
        const auto& wp = cfg_.waypoints[activeWpt_];

        constexpr double kLateralThreshold_m   = 2.0;
        constexpr double kHeadingThreshold_rad = 5.0 * ymir::math::PI / 180.0;

        const double lateralError = std::abs(computeLateralError(ctx));
        const double headingError = std::abs(ymir::math::wrapToPi(wp.headingTarget_rad - s.yaw()));

        if (lateralError < kLateralThreshold_m && headingError < kHeadingThreshold_rad)
        {
            phase_ = Phase::TurnROTTUG;
            rotRatePid_.reset();
        }
    }
    // TurnROTTUG is the terminal state — no further transitions.
}

// ---------------------------------------------------------------------------

double BerthManeuverSystem::computeLateralError(const NavalContext& ctx) const
{
    if (activeWpt_ >= cfg_.waypoints.size())
        return 0.0;
    const auto& wp  = cfg_.waypoints[activeWpt_];
    const double dx = wp.x_m - ctx.state.x();
    const double dy = wp.y_m - ctx.state.y();
    // Project (wp - pos) onto the axis perpendicular to headingTarget.
    return -dx * std::sin(wp.headingTarget_rad) + dy * std::cos(wp.headingTarget_rad);
}

// ---------------------------------------------------------------------------

void BerthManeuverSystem::updateNavigating(double t, double dt, const NavalContext& ctx,
                                            std::vector<Thruster>& thrusters,
                                            std::vector<Rudder>&   rudders)
{
    // Delegate LOS + PID control to internal ManeuverController.
    navMc_.update(t, dt, ctx, thrusters, rudders);

    // Tugs in ESCORTING mode: force along bearing direction (body frame).
    for (std::size_t i = 0; i < cfg_.tugs.size(); ++i)
    {
        const auto& tug = cfg_.tugs[i];
        const double bearing_rad = ymir::math::deg2rad(tug.bearing_deg);
        const double fx          = tug.escortForce_N * std::cos(bearing_rad);
        const double fy          = tug.escortForce_N * std::sin(bearing_rad);

        Vector6 tugForce{};
        tugForce[0] = fx;
        tugForce[1] = fy;
        tugForce[5] = fy * tug.arm_m;
        writeTugForce(i, tugForce);
    }
}

// ---------------------------------------------------------------------------

void BerthManeuverSystem::updateSideway(double /*t*/, double dt, const NavalContext& ctx,
                                         std::vector<Thruster>& thrusters,
                                         std::vector<Rudder>&   rudders)
{
    if (activeWpt_ < cfg_.waypoints.size())
    {
        const auto& wp = cfg_.waypoints[activeWpt_];

        // Heading PID: maintain headingTarget_rad via rudder.
        const double headingError = ymir::math::wrapToPi(wp.headingTarget_rad - ctx.state.yaw());
        const double rudderDemand = std::clamp(headingPid_.update(headingError, dt),
                                               -cfg_.maxRudderAngle_rad,
                                                cfg_.maxRudderAngle_rad);
        for (auto& rud : rudders)
            rud.setDemand(rudderDemand);

        // Lateral PD: push tugs to reduce perpendicular offset.
        const double lateralError = computeLateralError(ctx);
        const double lateralVel   = ctx.state.v();  // body-frame sway as rate proxy
        const double forcePD      = cfg_.lateralKp * lateralError + cfg_.lateralKd * lateralVel;

        for (std::size_t i = 0; i < cfg_.tugs.size(); ++i)
        {
            const auto& tug = cfg_.tugs[i];
            const double applied = std::clamp(forcePD, -tug.pushForce_N, tug.pushForce_N);

            Vector6 tugForce{};
            tugForce[1] = applied;
            tugForce[5] = applied * tug.arm_m;
            writeTugForce(i, tugForce);
        }
    }

    // Thrusters: hold current demand (no speed changes during Sideway approach).
    for (auto& thr : thrusters)
        thr.setDemand(thr.state().demandedRPM,
                      thr.state().demandedPitch,
                      thr.state().demandedAzimuth_deg);
}

// ---------------------------------------------------------------------------

void BerthManeuverSystem::updateTurnROTTUG(double /*t*/, double dt, const NavalContext& ctx,
                                            std::vector<Thruster>& thrusters,
                                            std::vector<Rudder>&   rudders)
{
    // ROT PID: drive yaw rate to zero via rudder.
    const double rotError     = 0.0 - ctx.state.r();
    const double rudderDemand = std::clamp(rotRatePid_.update(rotError, dt),
                                           -cfg_.maxRudderAngle_rad,
                                            cfg_.maxRudderAngle_rad);
    for (auto& rud : rudders)
        rud.setDemand(rudderDemand);

    // Tugs in PUSH: maintain lateral position reached in Sideway phase.
    const double lateralError = computeLateralError(ctx);
    const double lateralVel   = ctx.state.v();
    const double forcePD      = cfg_.lateralKp * lateralError + cfg_.lateralKd * lateralVel;

    for (std::size_t i = 0; i < cfg_.tugs.size(); ++i)
    {
        const auto& tug = cfg_.tugs[i];
        const double applied = std::clamp(forcePD, -tug.pushForce_N, tug.pushForce_N);

        Vector6 tugForce{};
        tugForce[1] = applied;
        tugForce[5] = applied * tug.arm_m;
        writeTugForce(i, tugForce);
    }

    // Thrusters: hold current demand.
    for (auto& thr : thrusters)
        thr.setDemand(thr.state().demandedRPM,
                      thr.state().demandedPitch,
                      thr.state().demandedAzimuth_deg);
}

// ---------------------------------------------------------------------------

BerthManeuverSystem::Phase BerthManeuverSystem::currentPhase() const noexcept
{
    return phase_;
}

const Vector6& BerthManeuverSystem::tugForces() const noexcept
{
    return tugForces_;
}

void BerthManeuverSystem::writeTugForce(std::size_t tugIndex, const Vector6& tugForce)
{
    if (registry_ != nullptr)
    {
        assert(tugBodyIds_.size() == cfg_.tugs.size());
        assert(tugIndex < tugBodyIds_.size());

        Forces force = Forces::zero();
        for (std::size_t i = 0; i < force.f.size(); ++i)
            force.f[i] = tugForce[i];
        registry_->writeForce(tugBodyIds_[tugIndex], force);
        return;
    }

    for (std::size_t i = 0; i < tugForces_.size(); ++i)
        tugForces_[i] += tugForce[i];
}

} // namespace ymir::naval
