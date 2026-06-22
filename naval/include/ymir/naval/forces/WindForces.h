#pragma once

#include <ymir/naval/NavalForceModel.h>
#include <ymir/naval/VesselConfig.h>

#include <array>
#include <vector>

namespace ymir::naval
{

class WindForces final : public NavalForceModel
{
public:
    struct Config
    {
        WindModel             model         = WindModel::REGULAR;
        double                frontalArea   = 0.0;
        double                lateralArea   = 0.0;
        double                frontalHeight = 0.0;  // centroid z from WAMIT origin
        double                lateralHeight = 0.0;
        double                midshipDistance = 0.0;
        std::array<double, 3> wavesOriginPosition{};
        double                length_BP     = 100.0;
        double                beam          = 30.0;
        double                draft         = 10.0;

        // Cd table columns (separated for Interpolation::linear API)
        std::vector<double> angles;  // deg 0..360
        std::vector<double> cdx;
        std::vector<double> cdy;
        std::vector<double> cdz;
    };

    explicit WindForces(const Config& cfg);

    std::string name() const override { return "wind"; }

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Config cfg_;
};

} // namespace ymir::naval
