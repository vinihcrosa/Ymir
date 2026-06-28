#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>

#include <cmath>

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

static ThrustForces::Config makeSingleThruster(double initialRPM = 0.0, double azimuth_deg = 0.0)
{
    ThrustForces::Config cfg{};
    ThrustForces::ThrusterConfig t{};
    t.position    = {-50.0, 0.0, -3.0};
    t.diameter    = 5.0;
    t.pitchRatio  = 0.8;
    t.nominalRPM  = 120.0;
    t.azimuth_deg = azimuth_deg;
    t.initialRPM  = initialRPM;
    cfg.thrusters.push_back(t);
    return cfg;
}

TEST_CASE("ThrustForces zero RPM produces no thrust")
{
    ThrustForces model(makeSingleThruster(0.0));
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    for (int i = 0; i < 6; ++i)
        REQUIRE(forces.f[i] == Approx(0.0).margin(1e-6));
}

TEST_CASE("ThrustForces positive RPM aft thruster produces positive surge")
{
    ThrustForces model(makeSingleThruster(120.0, 0.0));
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[0] > 0.0);
}

TEST_CASE("ThrustForces setActuatorState RPM=0 produces zero force")
{
    ThrustForces model(makeSingleThruster(120.0));
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    model.setActuatorState(0, {0.0, 0.0, 0.8});
    auto forces = model.compute(ctx.state);
    for (int i = 0; i < 6; ++i)
        REQUIRE(forces.f[i] == Approx(0.0).margin(1e-6));
}

TEST_CASE("ThrustForces setActuatorState RPM=100 produces expected surge")
{
    ThrustForces model(makeSingleThruster(0.0, 0.0));
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    model.setActuatorState(0, {100.0, 0.0, 0.8});
    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[0] > 0.0);
    REQUIRE(forces.f[1] == Approx(0.0).margin(1e-6));
}

TEST_CASE("ThrustForces setActuatorState azimuth 90 deg produces lateral force")
{
    ThrustForces model(makeSingleThruster(0.0, 0.0));
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    model.setActuatorState(0, {100.0, 90.0, 0.8});
    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[1] != Approx(0.0).margin(1e-3));
    REQUIRE(std::abs(forces.f[0]) < std::abs(forces.f[1]));
}

TEST_CASE("ThrustForces constructor initialises commands from initialRPM")
{
    ThrustForces model(makeSingleThruster(80.0, 0.0));
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[0] > 0.0);
}
