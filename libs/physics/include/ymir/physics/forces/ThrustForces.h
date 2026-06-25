#pragma once

#include <ymir/physics/NavalForceModel.h>

#include <array>
#include <vector>

namespace ymir::naval
{

class ThrustForces final : public NavalForceModel
{
public:
    struct ThrusterConfig
    {
        std::array<double, 3> position{};   // body frame (m)
        double                diameter    = 5.0;    // propeller diameter (m)
        double                pitchRatio  = 0.8;    // P/D
        double                nominalRPM  = 120.0;
        double                azimuth_deg = 0.0;    // 0 = forward, +90 = starboard
        double                commandRPM  = 0.0;    // current command RPM
        double                rpmTimeCst  = 5.0;    // first-order RPM lag (s)
    };

    struct Config
    {
        std::vector<ThrusterConfig> thrusters;
        double rho = 1025.0;
    };

    explicit ThrustForces(const Config& cfg);

    std::string name() const override { return "thrust"; }

    // Returns thrust magnitude for thruster [id] (used by RudderForces slipstream)
    double getThrust(std::size_t id) const noexcept;

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Config              cfg_;
    std::vector<double> currentRPM_;     // dynamic RPM state
    std::vector<double> lastThrust_;     // cached for RudderForces
};

} // namespace ymir::naval
