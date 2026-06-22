#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/naval/forces/RudderForces.h>
#include <ymir/naval/NavalContext.h>
#include <ymir/core/BodyState.h>

using Catch::Approx;
using namespace ymir::naval;

static NavalContext makeCtx(double u = 5.0)
{
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    qdot[0] = u;
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state = bs;
    ctx.speedToWater[0] = u;
    return ctx;
}

TEST_CASE("RudderForces zero angle produces no lateral force")
{
    RudderForces::Config cfg{};
    RudderForces::RudderConfig r{};
    r.position    = {-50.0, 0.0, -3.0};
    r.area        = 20.0;
    r.aspectRatio = 2.0;
    r.angle_deg   = 0.0;
    cfg.rudders.push_back(r);

    RudderForces model(cfg);
    NavalContext ctx = makeCtx(5.0);
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[1] == Approx(0.0).margin(1e-6));
    REQUIRE(forces.f[5] == Approx(0.0).margin(1e-6));
}

TEST_CASE("RudderForces positive angle produces sway force and yaw moment")
{
    RudderForces::Config cfg{};
    RudderForces::RudderConfig r{};
    r.position    = {-50.0, 0.0, -3.0};
    r.area        = 20.0;
    r.aspectRatio = 2.0;
    r.angle_deg   = 20.0;  // 20 degree rudder
    cfg.rudders.push_back(r);

    RudderForces model(cfg);
    NavalContext ctx = makeCtx(5.0);
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[1] != Approx(0.0).margin(1e-3));
    REQUIRE(forces.f[5] != Approx(0.0).margin(1e-3));
}
