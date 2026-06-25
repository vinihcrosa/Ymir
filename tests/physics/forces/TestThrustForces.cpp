#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>

using Catch::Approx;
using namespace ymir::naval;

static NavalContext makeCtx()
{
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state = bs;
    return ctx;
}

TEST_CASE("ThrustForces zero RPM produces no thrust")
{
    ThrustForces::Config cfg{};
    ThrustForces::ThrusterConfig t{};
    t.position   = {-50.0, 0.0, -3.0};
    t.diameter   = 5.0;
    t.pitchRatio = 0.8;
    t.nominalRPM = 120.0;
    t.azimuth_deg = 0.0;
    t.commandRPM  = 0.0;
    cfg.thrusters.push_back(t);

    ThrustForces model(cfg);
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    for (int i = 0; i < 6; ++i)
        REQUIRE(forces.f[i] == Approx(0.0).margin(1e-6));
}

TEST_CASE("ThrustForces positive RPM aft thruster produces positive surge")
{
    ThrustForces::Config cfg{};
    ThrustForces::ThrusterConfig t{};
    t.position    = {-50.0, 0.0, -3.0};
    t.diameter    = 5.0;
    t.pitchRatio  = 0.8;
    t.nominalRPM  = 120.0;
    t.azimuth_deg = 0.0;  // pointing forward
    t.commandRPM  = 120.0;
    cfg.thrusters.push_back(t);

    ThrustForces model(cfg);
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[0] > 0.0);
}
