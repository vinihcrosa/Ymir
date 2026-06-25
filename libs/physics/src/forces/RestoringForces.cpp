#include <ymir/physics/forces/RestoringForces.h>

#include <ymir/common/PhysicalConstants.h>

#include <cmath>

namespace ymir::naval
{

RestoringForces::RestoringForces(const RestoringConfig& cfg)
    : cfg_(cfg)
    , netBuoyancy_(cfg.mass * g - cfg.volumetricWeight)
{
}

Forces RestoringForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    const auto& q    = state.q();
    const auto& orig = cfg_.wavesOriginPosition;

    // Displacement from equilibrium
    double dq0 = q[0] - orig[0];
    double dq1 = q[1] - orig[1];
    double dq2 = q[2] - orig[2] + cfg_.draft - ctx.tide;
    // dq[3..5] = q[3..5] (angles from zero)

    Forces fr;

    // Diagonal spring
    fr.f[0] = -cfg_.hydro_rest[0][0] * dq0;
    fr.f[1] = -cfg_.hydro_rest[1][1] * dq1;
    fr.f[2] = -cfg_.hydro_rest[2][2] * dq2;
    fr.f[3] = -cfg_.hydro_rest[3][3] * q[3];
    fr.f[4] = -cfg_.hydro_rest[4][4] * q[4];
    fr.f[5] = -cfg_.hydro_rest[5][5] * q[5];

    // Heave: subtract net buoyancy offset
    fr.f[2] -= netBuoyancy_;

    // Roll moment: buoyancy × centre of flotation arm, minus gravity × CG arm
    const double cosRoll  = std::cos(q[3]);
    const double cosPitch = std::cos(q[4]);
    fr.f[3] += (fr.f[2] * cfg_.cf[1]
                - cfg_.mass * g * (cfg_.cg[1] - orig[1])) * cosRoll;

    // Pitch moment
    fr.f[4] -= (fr.f[2] * cfg_.cf[0]
                - cfg_.mass * g * (cfg_.cg[0] - orig[0])) * cosPitch;

    return fr;
}

} // namespace ymir::naval
