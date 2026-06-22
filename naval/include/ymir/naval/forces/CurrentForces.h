#pragma once

#include <ymir/naval/NavalForceModel.h>
#include <ymir/naval/VesselConfig.h>

#include <array>
#include <vector>

namespace ymir::naval
{

class CurrentForces final : public NavalForceModel
{
public:
    struct Config
    {
        CurrentModel model = CurrentModel::OBOKATA;

        // Geometry
        double                length_BP       = 100.0;
        double                beam            = 30.0;
        double                frontalHeight   = 0.0;
        double                lateralHeight   = 0.0;
        double                midshipDistance = 0.0;
        std::array<double, 3> wavesOriginPosition{};

        // Section integration (OBOKATA)
        int                 n_sections = 50;
        std::vector<double> sectionLocalPositions;  // x in body frame (m)

        // Drag coefficient table columns
        std::vector<double> angles;  // deg 0..360
        std::vector<double> cdx;
        std::vector<double> cdy;
        std::vector<double> cdz;

        // Projected areas (REGULAR)
        double frontalArea = 0.0;
        double lateralArea = 0.0;

        // Van der Pol (REGULAR)
        double st  = 0.2;
        double ez  = 0.3;
        double az  = 0.1;
        double clO = 1.0;
    };

    explicit CurrentForces(const Config& cfg);

    std::string name() const override { return "current"; }

    void resetState() noexcept override;

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Forces computeObokata(const BodyState& state, const NavalContext& ctx);
    Forces computeRegular(const BodyState& state, const NavalContext& ctx);
    void   stepVdp(double dt, double acc0);  // RK4 for VDP oscillator

    Config cfg_;

    // Van der Pol state
    double vdp_qz_    = 0.0;
    double vdp_dqzdt_ = 0.0;
};

} // namespace ymir::naval
