#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/naval/forces/WindForces.h>
#include <ymir/naval/NavalContext.h>
#include <ymir/core/BodyState.h>

using Catch::Approx;
using namespace ymir::naval;

static WindForces::Config makeWindCfg()
{
    WindForces::Config cfg{};
    cfg.model        = WindModel::REGULAR;
    cfg.frontalArea  = 200.0;
    cfg.lateralArea  = 1000.0;
    cfg.frontalHeight = 5.0;
    cfg.lateralHeight = 5.0;
    cfg.length_BP    = 100.0;
    cfg.beam         = 20.0;
    cfg.draft        = 8.0;
    // Simple Cd table: 0 and 360 degrees both Cd=1.0
    cfg.angles = {0.0, 90.0, 180.0, 270.0, 360.0};
    cfg.cdx    = {1.0, 0.0, -1.0, 0.0, 1.0};
    cfg.cdy    = {0.0, 1.0,  0.0, -1.0, 0.0};
    cfg.cdz    = {0.0, 0.0,  0.0, 0.0, 0.0};
    return cfg;
}

static NavalContext makeCtx(double wind_u, double wind_v)
{
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state = bs;
    ctx.speedToWind[0] = wind_u;
    ctx.speedToWind[1] = wind_v;
    return ctx;
}

TEST_CASE("WindForces zero wind produces no force")
{
    WindForces model(makeWindCfg());
    NavalContext ctx = makeCtx(0.0, 0.0);
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    for (int i = 0; i < 6; ++i)
        REQUIRE(forces.f[i] == Approx(0.0).margin(1e-12));
}

TEST_CASE("WindForces head wind produces surge drag")
{
    WindForces model(makeWindCfg());
    NavalContext ctx = makeCtx(10.0, 0.0);  // wind from ahead
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[0] != Approx(0.0).margin(1e-3));
    REQUIRE(std::abs(forces.f[1]) < std::abs(forces.f[0]));  // surge dominant
}

TEST_CASE("WindForces beam wind produces sway force")
{
    WindForces model(makeWindCfg());
    NavalContext ctx = makeCtx(0.0, 10.0);  // wind from beam
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[1] != Approx(0.0).margin(1e-3));
}
