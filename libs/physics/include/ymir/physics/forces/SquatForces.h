#pragma once

#include <ymir/physics/NavalForceModel.h>

namespace ymir::naval
{

class SquatForces final : public NavalForceModel
{
public:
    struct Config
    {
        double blockCoefficient  = 0.75;
        double volumetricWeight  = 0.0;   // N
        double length_BP         = 100.0; // m
        double hydroRestHeave    = 0.0;   // N/m — hydro_rest[2][2]
    };

    explicit SquatForces(const Config& cfg);

    std::string name() const override { return "squat"; }

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Config cfg_;
    double Cs_;    // cached block coefficient lookup
    double nabla_; // volumetric displacement (m³)
};

} // namespace ymir::naval
