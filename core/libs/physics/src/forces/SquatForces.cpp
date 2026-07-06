#include <ymir/physics/forces/SquatForces.h>

#include <ymir/common/PhysicalConstants.h>

#include <cmath>
#include <algorithm>

namespace ymir::naval
{

SquatForces::SquatForces(const Config& cfg)
    : cfg_(cfg)
{
    // Squat coefficient Cs lookup — matches MATLAB VesselFastTime.squatForce
    // ordering (dynamics repo): Cb>1 passes through, then the banded table.
    double Cb = cfg.blockCoefficient;
    if      (Cb > 1.0) Cs_ = Cb;
    else if (Cb < 0.7) Cs_ = 1.7;
    else if (Cb < 0.8) Cs_ = 2.0;
    else               Cs_ = 2.4;
}

Forces SquatForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    // Heave relative to still-water equilibrium. The reference's q[2] is zero at
    // the floating waterline; Ymir's state.z() is draft-referenced (z ≈ -draft at
    // equilibrium), so shift by +draft to match the reference convention. Using the
    // raw draft-referenced z here makes |z| (~draft) wrongly dominate the depth and
    // invert the seabed clamp.
    const double zRel = state.z() + cfg_.draft;

    // Effective water depth
    double depth = std::max(std::abs(ctx.waterDepth + ctx.tide), std::abs(zRel));
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

    // Sinkage — matches reference squatForces/SquatForces.cpp:48-49:
    //   s = -(Cs+Cf) * volumetricWeight/(rho*g*L^2) * Fn^2 / sqrt(1-Fn^2)
    // (volumetricWeight is a weight in N; single division by rho*g).
    double L2 = cfg_.length_BP * cfg_.length_BP;
    double s  = -(Cs_ + Cf) * (cfg_.volumetricWeight / (rho_water * g * L2)) * Fn * Fn
                / std::sqrt(denom);

    // Clamp: cannot sink below seabed (reference: s = max(-depth-0.1-q[2], s))
    double s_min = -(depth + 0.1 + zRel);
    if (s < s_min) s = s_min;

    Forces fsq;
    fsq.f[2] = cfg_.hydroRestHeave * s;
    return fsq;
}

} // namespace ymir::naval
