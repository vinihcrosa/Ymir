#pragma once

#include <ymir/vessel/VesselConfig.h>
#include <ymir/physics/forces/RudderForces.h>

namespace ymir::naval
{

/**
 * Actuator entity for a single rudder.
 *
 * Holds the dynamic state (current and demanded angles) and advances it each
 * tick via update(dt) using a rate limiter.  The static parameters live in the
 * referenced RudderConfig, which must outlive this object.
 *
 * Non-copyable — move-constructible so std::vector<Rudder> can emplace_back.
 * Moving only copies the config reference (references cannot be rebound) and
 * the POD state; the moved-from object remains valid and unmodified.
 */
class Rudder
{
public:
    /** Mutable runtime state of the actuator. */
    struct ActuatorState
    {
        double currentAngle_rad  = 0.0;
        double demandedAngle_rad = 0.0;
    };

    /** Construct from static config.  cfg must outlive this object. */
    explicit Rudder(const RudderConfig& cfg);

    Rudder(const Rudder&)            = delete;
    Rudder& operator=(const Rudder&) = delete;
    Rudder(Rudder&&)                 = default;
    Rudder& operator=(Rudder&&)      = delete;

    /**
     * Set demanded angle.  Current state is unchanged until update() is called.
     * @param angle_rad  demanded rudder angle [rad]
     */
    void setDemand(double angle_rad) noexcept;

    /**
     * Advance actuator dynamics by dt seconds.
     * Angle: rate-limited at cfg.angleSpeed [rad/s], clamped to ±cfg.angleMaximum.
     */
    void update(double dt) noexcept;

    /** Read-only access to current actuator state. */
    const ActuatorState& state()  const noexcept;

    /** Read-only access to static configuration. */
    const RudderConfig&  config() const noexcept;

    /** Convert current state to a RudderCommand for RudderForces::setActuatorState(). */
    RudderForces::RudderCommand toCommand() const noexcept;

private:
    const RudderConfig& cfg_;
    ActuatorState       state_;
};

} // namespace ymir::naval
