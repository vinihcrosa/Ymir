#pragma once

#include <ymir/physics/NavalForceModel.h>
#include <ymir/vessel/controllers/BerthManeuverSystem.h>

namespace ymir::naval
{

/**
 * NavalForceModel adapter that injects BerthManeuverSystem tug forces into the
 * physics body before each CVODE integration step.
 *
 * Holds a non-owning pointer to a BerthManeuverSystem.  The BSM must outlive
 * this object.  On each computeNaval() call, reads bsm_->tugForces() and returns
 * the pre-computed forces — no state is kept here.
 *
 * Usage:
 *   auto tugForces = std::make_unique<TugParametricForces>(&bsm);
 *   navalSim.addNavalForceModel(bodyId, std::move(tugForces));
 */
class [[deprecated("Use CouplingRegistry + CouplingForceModel. TugParametricForces will be removed in Phase 3.")]]
TugParametricForces : public NavalForceModel
{
public:
    /** bsm must not be null and must outlive this object. */
    explicit TugParametricForces(const BerthManeuverSystem* bsm) noexcept;

protected:
    /** Returns the tug forces accumulated by BerthManeuverSystem::update(). */
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

private:
    const BerthManeuverSystem* bsm_;
};

} // namespace ymir::naval
