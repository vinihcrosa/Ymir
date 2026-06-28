#pragma once

#include <ymir/physics/NavalForceModel.h>
#include <ymir/world/wave/WaveSpectrum.h>

#include <array>
#include <vector>

namespace ymir::naval
{

class WaveForces final : public NavalForceModel
{
public:
    struct Config
    {
        std::array<double, 3> wavesOriginPosition{};

        // WAMIT frequency/direction axes
        std::vector<double> wamitFrequencies;  // rad/s
        std::vector<double> wamitDirections;   // rad (math convention)

        // RAO tables [dof][freq][dir] — magnitude (m/m or rad/m) and phase (rad)
        std::vector<std::vector<std::vector<double>>> raoMagnitude;
        std::vector<std::vector<std::vector<double>>> raoPhase;

        // Slow-drift QTF (optional — zero if empty) [freq_pair][dir]
        std::vector<std::vector<double>> qtfMagnitude;

        // Wave drift damping coefficient
        double driftDampingCoeff = 0.0;
    };

    WaveForces(const Config& cfg, const WaveSpectrum& spectrum);

    std::string name() const override { return "wave"; }

    void resetState() noexcept override;

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Config                     cfg_;
    std::vector<WaveComponent> components_;  // snapshot from WaveSpectrum

    // Per-DOF filtered RAO state for slow-drift
    std::array<double, 6> q_rao_prev_{};
};

} // namespace ymir::naval
