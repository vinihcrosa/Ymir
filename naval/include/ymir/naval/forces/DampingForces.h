#pragma once

#include <ymir/core/Types.h>
#include <ymir/naval/NavalForceModel.h>

namespace ymir::naval
{

class DampingForces final : public NavalForceModel
{
public:
    struct Config
    {
        Matrix6x6 potential{};
        Matrix6x6 linear{};
        Matrix6x6 quadratic{};
        double    linearDampingCoeff = 0.1;
    };

    explicit DampingForces(const Config& cfg);

    std::string name() const override { return "damping"; }

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Config cfg_;
};

} // namespace ymir::naval
