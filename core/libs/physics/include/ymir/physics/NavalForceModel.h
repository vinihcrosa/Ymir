#pragma once

#include <ymir/physics/ForceModel.h>
#include <ymir/vessel/NavalContext.h>

namespace ymir::naval
{

/**
 * Bridge between core::ForceModel and the naval data plane.
 *
 * Holds a const NavalContext* bound by NavalSimulation before each step.
 * compute(BodyState&) asserts the context is bound, then delegates to the
 * naval-specific computeNaval(BodyState&, NavalContext&).
 *
 * Non-copyable and non-movable: raw context pointer invalidates on move.
 */
class NavalForceModel : public ymir::ForceModel
{
public:
    NavalForceModel(const NavalForceModel&)            = delete;
    NavalForceModel& operator=(const NavalForceModel&) = delete;
    NavalForceModel(NavalForceModel&&)                 = delete;
    NavalForceModel& operator=(NavalForceModel&&)      = delete;

    void bindContext(const NavalContext* ctx) noexcept { ctx_ = ctx; }

    /** Called by NavalSimulation::reset() on stateful models. Default: no-op. */
    virtual void resetState() noexcept {}

    // ForceModel::compute — asserts context bound, delegates to computeNaval
    Forces compute(const BodyState& state) final;

protected:
    NavalForceModel() = default;

    virtual Forces computeNaval(const BodyState& state, const NavalContext& ctx) = 0;

private:
    const NavalContext* ctx_ = nullptr;
};

} // namespace ymir::naval
