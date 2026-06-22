#pragma once

#include <ymir/core/Types.h>
#include <ymir/naval/NavalForceModel.h>

#include <array>

namespace ymir::naval
{

struct RestoringConfig
{
    Matrix6x6             hydro_rest{};
    std::array<double, 3> wavesOriginPosition{};
    double                draft            = 0.0;   // m
    double                mass             = 0.0;   // kg
    double                volumetricWeight = 0.0;   // N
    std::array<double, 3> cg{};
    std::array<double, 3> cf{};
};

class RestoringForces final : public NavalForceModel
{
public:
    explicit RestoringForces(const RestoringConfig& cfg);

    std::string name() const override { return "restoring"; }

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    RestoringConfig cfg_;
    double          netBuoyancy_;  // mass·g - volumetricWeight (constant)
};

} // namespace ymir::naval
