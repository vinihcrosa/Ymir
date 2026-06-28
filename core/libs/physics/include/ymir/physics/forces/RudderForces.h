#pragma once

#include <ymir/physics/NavalForceModel.h>

#include <array>
#include <vector>

namespace ymir::naval
{

class ThrustForces;

class RudderForces final : public NavalForceModel
{
public:
    struct RudderConfig
    {
        std::array<double, 3> position{};      // body frame attachment point (m)
        double                area         = 20.0;  // m²
        double                aspectRatio  = 2.0;
        std::size_t           thrusterIdx  = 0;     // linked thruster for slipstream (SIZE_MAX = none)
    };

    /** External actuator state fed by Rudder entity each tick. */
    struct RudderCommand
    {
        double currentAngle_rad = 0.0;
    };

    struct Config
    {
        std::vector<RudderConfig> rudders;
        double rho = 1025.0;
    };

    explicit RudderForces(const Config& cfg, const ThrustForces* thrust = nullptr);

    std::string name() const override { return "rudder"; }

    /** Set actuator state for rudder [id]. Call before computeNaval each tick. */
    void setActuatorState(std::size_t id, const RudderCommand& cmd) noexcept;

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Config                     cfg_;
    const ThrustForces*        thrust_;
    std::vector<RudderCommand> commands_;  // external actuator state
};

} // namespace ymir::naval
