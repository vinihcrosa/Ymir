#include <ymir/vessel/Rudder.h>

#include <algorithm>

namespace ymir::naval
{

Rudder::Rudder(const RudderConfig& cfg)
    : cfg_(cfg)
{
}

void Rudder::setDemand(double angle_rad) noexcept
{
    state_.demandedAngle_rad = angle_rad;
}

void Rudder::update(double dt) noexcept
{
    double delta = state_.demandedAngle_rad - state_.currentAngle_rad;
    double limit = cfg_.angleSpeed * dt;
    state_.currentAngle_rad += std::clamp(delta, -limit, limit);
    state_.currentAngle_rad  = std::clamp(state_.currentAngle_rad,
                                           -cfg_.angleMaximum,
                                           +cfg_.angleMaximum);
}

const Rudder::ActuatorState& Rudder::state() const noexcept
{
    return state_;
}

const RudderConfig& Rudder::config() const noexcept
{
    return cfg_;
}

RudderForces::RudderCommand Rudder::toCommand() const noexcept
{
    return {state_.currentAngle_rad};
}

} // namespace ymir::naval
