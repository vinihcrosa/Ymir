#include <ymir/vessel/Thruster.h>

#include <algorithm>
#include <cmath>

namespace ymir::naval
{

Thruster::Thruster(const ThrusterConfig& cfg)
    : cfg_(cfg)
{
    state_.currentRPM = cfg_.initialRPM;
}

void Thruster::setDemand(double rpm, double pitch_ratio, double azimuth_deg) noexcept
{
    state_.demandedRPM         = rpm;
    state_.demandedPitch       = pitch_ratio;
    state_.demandedAzimuth_deg = azimuth_deg;
}

void Thruster::update(double dt) noexcept
{
    // 1st-order filter for RPM: α = 1 - e^(-dt/τ)
    double alpha = 1.0 - std::exp(-dt / cfg_.rotationTime);
    state_.currentRPM += alpha * (state_.demandedRPM - state_.currentRPM);

    // Rate limiter for azimuth [deg/s]
    double deltaAz  = state_.demandedAzimuth_deg - state_.currentAzimuth_deg;
    double limitAz  = cfg_.azimuthSpeed * dt;
    state_.currentAzimuth_deg += std::clamp(deltaAz, -limitAz, limitAz);

    // Rate limiter for pitch [deg/s]
    double deltaPitch = state_.demandedPitch - state_.currentPitch;
    double limitPitch = cfg_.pitchRate * dt;
    state_.currentPitch += std::clamp(deltaPitch, -limitPitch, limitPitch);
}

const Thruster::ActuatorState& Thruster::state() const noexcept
{
    return state_;
}

const ThrusterConfig& Thruster::config() const noexcept
{
    return cfg_;
}

ThrustForces::ThrusterCommand Thruster::toCommand() const noexcept
{
    return {state_.currentRPM, state_.currentAzimuth_deg, state_.currentPitch};
}

} // namespace ymir::naval
