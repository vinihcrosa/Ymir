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

        // 1st-order excitation transfer function [dof][freq][dir] —
        // magnitude (N/m or N·m/m) and phase (rad)
        std::vector<std::vector<std::vector<double>>> raoMagnitude;
        std::vector<std::vector<std::vector<double>>> raoPhase;

        // 2nd-order mean-drift coefficient [dof][freq][dir] — steady drift force
        // per unit wave-amplitude² (N/m² or N·m/m²). Only surge(0)/sway(1)/yaw(5)
        // are physically populated; other DOF are ignored. Empty ⇒ no drift.
        std::vector<std::vector<std::vector<double>>> meanDriftCoeff;

        // Wave-drift damping coefficients vs incidence direction, indexed to
        // wamitDirections. bw = in-line, br = cross-coupling term. Populate
        // surge(0)/sway(1)/yaw(5). Empty vector ⇒ no damping for that DOF.
        std::array<std::vector<double>, 6> driftDampingBw;
        std::array<std::vector<double>, 6> driftDampingBr;
    };

    WaveForces(const Config& cfg, const WaveSpectrum& spectrum);

    std::string name() const override { return "wave"; }

    /** Replace the wave field (e.g. when the scenario sea state changes) without
     *  rebuilding the model — keeps the RAO tables, swaps the spectral components. */
    void setSpectrum(const WaveSpectrum& spectrum);

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Config                     cfg_;
    std::vector<WaveComponent> components_;  // snapshot from WaveSpectrum
};

} // namespace ymir::naval
