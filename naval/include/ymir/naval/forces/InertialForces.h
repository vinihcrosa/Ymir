#pragma once

#include <ymir/core/Types.h>
#include <ymir/naval/NavalForceModel.h>
#include <ymir/naval/VesselConfig.h>

#include <array>

namespace ymir::naval
{

struct InertialConfig
{
    Matrix6x6              totalMass{};   // (massMatrix + addedMass) in kg
    Matrix6x6              addedMass{};   // kg
    std::array<double, 3>  cg{};          // m — CG from WAMIT origin
    double                 mass = 0.0;    // kg — body mass only (massMatrix[0][0])
};

class InertialForces final : public NavalForceModel
{
public:
    explicit InertialForces(const InertialConfig& cfg);

    std::string name() const override { return "inertial"; }

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    InertialConfig cfg_;
    double M11_, M22_, dXa_;  // cached diagonal + asymmetry
};

} // namespace ymir::naval
