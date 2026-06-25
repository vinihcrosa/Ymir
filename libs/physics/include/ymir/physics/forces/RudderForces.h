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
        double                angle_deg    = 0.0;   // rudder angle (+port, −starboard typical)
        double                rateLimit    = 5.0;   // deg/s
        std::size_t           thrusterIdx  = 0;     // linked thruster for slipstream (SIZE_MAX = none)
    };

    struct Config
    {
        std::vector<RudderConfig> rudders;
        double rho = 1025.0;
    };

    explicit RudderForces(const Config& cfg, const ThrustForces* thrust = nullptr);

    std::string name() const override { return "rudder"; }

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Config               cfg_;
    const ThrustForces*  thrust_;
    std::vector<double>  currentAngle_;  // dynamic rudder angle state (deg)
};

} // namespace ymir::naval
