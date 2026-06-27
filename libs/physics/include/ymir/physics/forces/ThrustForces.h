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
        double                initialRPM  = 0.0;    // initial actuator RPM (warm-start)
    };

    /** External actuator state fed by Thruster entity each tick. */
    struct ThrusterCommand
    {
        double currentRPM         = 0.0;
        double currentAzimuth_deg = 0.0;
        double currentPitch       = 0.0;
    };

    struct Config
    {
        std::vector<ThrusterConfig> thrusters;
        double rho = 1025.0;
    };

    explicit ThrustForces(const Config& cfg);

    std::string name() const override { return "thrust"; }

    /** Set actuator state for thruster [id]. Call before computeNaval each tick. */
    void setActuatorState(std::size_t id, const ThrusterCommand& cmd) noexcept;

    // Returns thrust magnitude for thruster [id] (used by RudderForces slipstream)
    double getThrust(std::size_t id) const noexcept;

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Config                       cfg_;
    std::vector<ThrusterCommand> commands_;   // external actuator state
    std::vector<double>          lastThrust_; // cached for RudderForces
};

} // namespace ymir::naval
