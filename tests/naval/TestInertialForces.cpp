#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/naval/forces/InertialForces.h>
#include <ymir/naval/NavalContext.h>
#include <ymir/core/BodyState.h>
#include <ymir/core/Types.h>

using Catch::Approx;
using namespace ymir::naval;

static NavalContext makeCtx(double r = 0.0, double u = 0.0, double v = 0.0)
{
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    qdot[0] = u;
    qdot[1] = v;
    qdot[5] = r;
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state = bs;
    return ctx;
}

TEST_CASE("InertialForces constructs without crash")
{
    InertialConfig cfg{};
    for (int i = 0; i < 6; ++i) cfg.totalMass[i][i] = 1e6;
    cfg.mass = 1e6;

    InertialForces model(cfg);
    REQUIRE(true);
}

TEST_CASE("InertialForces Coriolis coupling r·v produces surge force")
{
    InertialConfig cfg{};
    // totalMass = body mass + added mass diagonal
    for (int i = 0; i < 6; ++i) cfg.totalMass[i][i] = 1000.0;
    cfg.totalMass[0][0] = 1100.0;  // body + Xa
    cfg.totalMass[1][1] = 1200.0;  // body + Ya
    cfg.mass = 1000.0;

    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            cfg.addedMass[i][j] = 0.0;
    cfg.addedMass[0][0] = 100.0;
    cfg.addedMass[1][1] = 200.0;

    NavalContext ctx = makeCtx(0.5, 0.0, 2.0);  // r=0.5 rad/s, v=2 m/s

    InertialForces model(cfg);
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    // f[0] = r * (M22*v + dXa*Vc1), with r=0.5, v=2, M22=1200, dXa=100-200=-100
    // f[0] = 0.5 * (1200*2 + 0) = 1200
    REQUIRE(forces.f[0] == Approx(1200.0).margin(1.0));
}
