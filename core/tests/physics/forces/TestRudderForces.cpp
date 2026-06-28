#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/physics/forces/RudderForces.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>

#include <cmath>

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

static RudderForces::Config makeSingleRudder()
{
    RudderForces::Config cfg{};
    RudderForces::RudderConfig r{};
    r.position    = {-50.0, 0.0, -3.0};
    r.area        = 20.0;
    r.aspectRatio = 2.0;
    cfg.rudders.push_back(r);
    return cfg;
}

TEST_CASE("RudderForces zero angle produces no lateral force")
{
    RudderForces model(makeSingleRudder());
    NavalContext ctx = makeCtx(5.0);
    model.bindContext(&ctx);

    model.setActuatorState(0, {0.0});
    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[1] == Approx(0.0).margin(1e-6));
    REQUIRE(forces.f[5] == Approx(0.0).margin(1e-6));
}

TEST_CASE("RudderForces positive angle produces sway force and yaw moment")
{
    RudderForces model(makeSingleRudder());
    NavalContext ctx = makeCtx(5.0);
    model.bindContext(&ctx);

    model.setActuatorState(0, {20.0 * M_PI / 180.0});
    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[1] != Approx(0.0).margin(1e-3));
    REQUIRE(forces.f[5] != Approx(0.0).margin(1e-3));
}

TEST_CASE("RudderForces setActuatorState angle=0.3 rad produces expected lateral force")
{
    RudderForces model(makeSingleRudder());
    NavalContext ctx = makeCtx(5.0);
    model.bindContext(&ctx);

    model.setActuatorState(0, {0.3});
    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[1] != Approx(0.0).margin(1e-3));
}

TEST_CASE("RudderForces setActuatorState angle=0 rad produces zero lateral force")
{
    RudderForces model(makeSingleRudder());
    NavalContext ctx = makeCtx(5.0);
    model.bindContext(&ctx);

    model.setActuatorState(0, {0.0});
    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[1] == Approx(0.0).margin(1e-6));
}

TEST_CASE("RudderForces default state is zero angle")
{
    RudderForces model(makeSingleRudder());
    NavalContext ctx = makeCtx(5.0);
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[1] == Approx(0.0).margin(1e-6));
}
