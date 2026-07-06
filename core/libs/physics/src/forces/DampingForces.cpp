#include <ymir/physics/forces/DampingForces.h>

#include <ymir/common/math/LinearAlgebra.h>

#include <cmath>

namespace ymir::naval
{

DampingForces::DampingForces(const Config& cfg) : cfg_(cfg) {}

Forces DampingForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    const auto& qdot = state.qdot();

    // Potential (radiation) damping
    Vector6 fd_pot = ymir::math::matVecProduct(cfg_.potential, qdot, -1.0);

    // Auxiliary velocity: surge+sway use speedToWater, rest use qdot. NOTE: the
    // reference uses -speedToWater because its speedToWater is fluid-relative-to-
    // vessel; Ymir's speedToWater is vessel-relative-to-water (bs.u() - current),
    // the opposite sign, so no negation is needed here for damping to oppose motion.
    Vector6 aux{};
    aux[0] = ctx.speedToWater[0];
    aux[1] = ctx.speedToWater[1];
    for (int i = 2; i < 6; ++i)
        aux[i] = qdot[i];

    // Exponential decay factor — reference uses the full 6-DOF velocity norm.
    double vNorm2 = 0.0;
    for (int i = 0; i < 6; ++i)
        vNorm2 += aux[i] * aux[i];
    double decay = std::exp(-cfg_.linearDampingCoeff * vNorm2);

    Vector6 exp_vel{};
    exp_vel[0] = aux[0] * decay;
    exp_vel[1] = aux[1] * decay;
    for (int i = 2; i < 6; ++i)
        exp_vel[i] = aux[i];

    Vector6 fd_lin  = ymir::math::matVecProduct(cfg_.linear, exp_vel, -1.0);
    Vector6 fd_quad = ymir::math::matVecQuadratic(cfg_.quadratic, qdot);
    // matVecQuadratic gives A*v*|v|; damping opposes motion → negate
    for (int i = 0; i < 6; ++i)
        fd_quad[i] = -fd_quad[i];

    Forces fd;
    for (int i = 0; i < 6; ++i)
        fd.f[i] = fd_pot[i] + fd_lin[i] + fd_quad[i];
    return fd;
}

} // namespace ymir::naval
