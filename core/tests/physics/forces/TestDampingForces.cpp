#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/physics/forces/DampingForces.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>
#include <ymir/common/Types.h>

using Catch::Approx;
using namespace ymir::naval;

static NavalContext makeCtx(double u = 0.0, double v = 0.0)
{
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    qdot[0] = u;
    qdot[1] = v;
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state = bs;
    ctx.speedToWater[0] = u;
    ctx.speedToWater[1] = v;
    return ctx;
}

TEST_CASE("DampingForces zero velocity produces zero force")
{
    DampingForces::Config cfg{};
    cfg.linear[0][0]    = 100.0;
    cfg.quadratic[0][0] = 50.0;

    DampingForces model(cfg);
    NavalContext ctx = makeCtx(0.0, 0.0);
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    for (int i = 0; i < 6; ++i)
        REQUIRE(forces.f[i] == Approx(0.0).margin(1e-12));
}

TEST_CASE("DampingForces linear surge damps surge velocity")
{
    DampingForces::Config cfg{};
    cfg.linear[0][0]        = 1000.0;  // 1 kN·s/m
    cfg.linearDampingCoeff  = 0.0;     // disable exponential decay for clean check

    DampingForces model(cfg);
    NavalContext ctx = makeCtx(2.0, 0.0);  // u = 2 m/s
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    // F = -B * u = -1000 * 2 = -2000 N
    REQUIRE(forces.f[0] == Approx(-2000.0).margin(1.0));
}
