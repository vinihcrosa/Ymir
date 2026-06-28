#pragma once

#include <ymir/vessel/VesselConfig.h>
#include <ymir/physics/forces/ThrustForces.h>

namespace ymir::naval
{

/**
 * Actuator entity for a single thruster.
 *
 * Holds the dynamic state (current and demanded values) and advances it each
 * tick via update(dt).  The static parameters live in the referenced
 * ThrusterConfig, which must outlive this object.
 *
 * Non-copyable — move-constructible so std::vector<Thruster> can emplace_back.
 * Moving only copies the config reference (references cannot be rebound) and
 * the POD state; the moved-from object remains valid and unmodified.
 */
class Thruster
{
public:
    /** Mutable runtime state of the actuator. */
    struct ActuatorState
    {
        double currentRPM          = 0.0;
        double currentPitch        = 0.0;
        double currentAzimuth_deg  = 0.0;
        double demandedRPM         = 0.0;
        double demandedPitch       = 0.0;
        double demandedAzimuth_deg = 0.0;
    };

    /** Construct from static config.  cfg must outlive this object. */
    explicit Thruster(const ThrusterConfig& cfg);

    Thruster(const Thruster&)            = delete;
    Thruster& operator=(const Thruster&) = delete;
    Thruster(Thruster&&)                 = default;
    Thruster& operator=(Thruster&&)      = delete;

    /**
     * Set demanded values.  Current state is unchanged until update() is called.
     * @param rpm          demanded propeller speed [RPM]
     * @param pitch_ratio  demanded pitch ratio [P/D]
     * @param azimuth_deg  demanded azimuth angle [deg]
     */
    void setDemand(double rpm, double pitch_ratio, double azimuth_deg) noexcept;

    /**
     * Advance actuator dynamics by dt seconds.
     * RPM: 1st-order filter with τ = cfg.rotationTime.
     * Azimuth and pitch: rate-limited at cfg.azimuthSpeed / cfg.pitchRate [deg/s].
     */
    void update(double dt) noexcept;

    /** Read-only access to current actuator state. */
    const ActuatorState&  state()  const noexcept;

    /** Read-only access to static configuration. */
    const ThrusterConfig& config() const noexcept;

    /** Convert current state to a ThrusterCommand for ThrustForces::setActuatorState(). */
    ThrustForces::ThrusterCommand toCommand() const noexcept;

private:
    const ThrusterConfig& cfg_;
    ActuatorState         state_;
};

} // namespace ymir::naval
