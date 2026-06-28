#include <ymir/physics/forces/InertialForces.h>

namespace ymir::naval
{

InertialForces::InertialForces(const InertialConfig& cfg)
    : cfg_(cfg)
    , M11_(cfg.totalMass[0][0])
    , M22_(cfg.totalMass[1][1])
    , dXa_(cfg.addedMass[0][0] - cfg.addedMass[1][1])
{
}

Forces InertialForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    const double u    = state.u();
    const double v    = state.v();
    const double w    = state.w();
    const double p    = state.p();
    const double q    = state.q_rot();
    const double r    = state.r();  // ψ̇
    const double Vc0  = ctx.speedToWater[0];
    const double Vc1  = ctx.speedToWater[1];

    const double cx   = cfg_.cg[0];
    const double cy   = cfg_.cg[1];
    const double cz   = cfg_.cg[2];
    const double m    = cfg_.mass;

    Forces fi;
    fi.f[0] = r  * (M22_ * v + dXa_ * Vc1);
    fi.f[1] = r  * (-M11_ * u + dXa_ * Vc0);
    fi.f[2] = 0.0;
    fi.f[3] = m  * (cz * (u * r - w * p) + cy * (u * q - v * p));
    fi.f[4] = m  * (cx * (v * p - u * q) + cz * (v * r - w * q));
    fi.f[5] = r  * (cfg_.addedMass[0][5] * v - cfg_.addedMass[1][5] * u);
    return fi;
}

} // namespace ymir::naval
