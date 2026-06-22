#include <ymir/naval/forces/SquatForces.h>

#include <ymir/naval/PhysicalConstants.h>

#include <cmath>
#include <algorithm>

namespace ymir::naval
{

SquatForces::SquatForces(const Config& cfg)
    : cfg_(cfg)
    , nabla_(cfg.volumetricWeight / (rho_water * g))
{
    double Cb = cfg.blockCoefficient;
    if      (Cb < 0.7) Cs_ = 1.7;
    else if (Cb < 0.8) Cs_ = 2.0;
    else               Cs_ = 2.4;
}

Forces SquatForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    // Effective water depth
    double depth = std::max(std::abs(ctx.waterDepth + ctx.tide), std::abs(state.z()));
    if (depth < 0.01) depth = 0.01;  // guard against zero

    double v2   = ctx.speedToWater[0] * ctx.speedToWater[0]
                + ctx.speedToWater[1] * ctx.speedToWater[1];
    double v    = std::sqrt(v2);
    double Fn   = v / std::sqrt(g * depth);

    if (Fn < 1e-6)
        return Forces::zero();

    double Cf = 0.0;
    if (Fn > 0.7)
    {
        Cf = 0.3;
        Fn = std::min(Fn, 0.8);
    }

    double denom = 1.0 - Fn * Fn;
    if (denom <= 0.0) denom = 1e-6;

    double L2 = cfg_.length_BP * cfg_.length_BP;
    double s  = -(Cs_ + Cf) * (nabla_ / (rho_water * g * L2)) * Fn * Fn / std::sqrt(denom);

    // Clamp: cannot sink below seabed
    double s_min = -(depth + 0.1 + state.z());
    if (s < s_min) s = s_min;

    Forces fsq;
    fsq.f[2] = cfg_.hydroRestHeave * s;
    return fsq;
}

} // namespace ymir::naval
