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

        // --- Open-water propeller model (matches MATLAB dynamics/thruster.m) ---
        // Each entry = {J, Kq, Kt}, J ascending. Empty → legacy linear-Kt fallback.
        std::vector<std::array<double, 3>> openWaterCurve;

        double rotationSpeedMax       = 150.0;  // RPM — used for paddle-effect ratio
        double maximumPowerW          = 5e6;    // W  — power-saturation ceiling
        double mechanicalEfficiency   = 1.0;    // dimensionless — shaft/gear losses
        double asternEfficiency       = 1.0;    // dimensionless — thrust penalty in reverse
        double transversalSpeedLimit  = 2.0;    // m/s — max transverse inflow for reduction
        double transversalReductionCoeff = 0.5; // dimensionless — reduc form factor
        std::array<double, 3> paddleCoeffs{};   // {c0, c1, c2} side-wash/paddle force
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

    // Last-computed shaft torque [N·m] for thruster [id] (telemetry / tests)
    double getTorque(std::size_t id) const noexcept;

    // Last-computed shaft power [W] for thruster [id] (telemetry / tests)
    double getPower(std::size_t id) const noexcept;

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Config                       cfg_;
    std::vector<ThrusterCommand> commands_;   // external actuator state
    std::vector<double>          lastThrust_; // cached for RudderForces
    std::vector<double>          lastTorque_; // N·m, cached for telemetry
    std::vector<double>          lastPower_;  // W,   cached for telemetry
};

} // namespace ymir::naval
