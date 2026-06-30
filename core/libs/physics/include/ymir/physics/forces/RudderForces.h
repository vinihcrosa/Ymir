#pragma once

#include <ymir/physics/NavalForceModel.h>

#include <array>
#include <limits>
#include <vector>

namespace ymir::naval
{

class ThrustForces;

class RudderForces final : public NavalForceModel
{
public:
    struct RudderConfig
    {
        std::array<double, 3> position{};            // body frame attachment point (m)
        double                area             = 20.0; // m²
        std::size_t           thrusterIdx      = std::numeric_limits<std::size_t>::max(); // linked thruster for slipstream (max = standalone)
        double                thrusterDiameter = 5.0;  // m — propeller diameter for slipstream actuator term
        double                hullEfficiency   = 1.0;  // wake fraction applied to forward inflow (Va·w)
        double                p1               = 1.0;  // geometric slipstream factor (0.5..1), precomputed from rudder/thruster spacing

        // Foil coefficient table: each entry = {angle_deg, Cl, Cd}, interpolated vs inflow incidence β.
        std::vector<std::array<double, 3>> coefficients;
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
