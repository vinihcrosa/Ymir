#include <ymir/vessel/controllers/TugParametricForces.h>

namespace ymir::naval
{

TugParametricForces::TugParametricForces(const BerthManeuverSystem* bsm) noexcept
    : bsm_(bsm)
{}

Forces TugParametricForces::computeNaval(const BodyState& /*state*/, const NavalContext& /*ctx*/)
{
    Forces f{};
    const Vector6& tug = bsm_->tugForces();
    for (int i = 0; i < 6; ++i)
        f.f[i] = tug[i];
    return f;
}

} // namespace ymir::naval
