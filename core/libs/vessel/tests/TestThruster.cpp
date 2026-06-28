#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/vessel/Thruster.h>
#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>

#include <cmath>

using Catch::Approx;
using namespace ymir::naval;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ThrusterConfig makeCfg(double tau          = 50.0,
                               double azimuthSpeed = 5.0,
                               double initialRPM   = 0.0)
{
    ThrusterConfig cfg{};
    cfg.rotationTime  = tau;
    cfg.azimuthSpeed  = azimuthSpeed;
    cfg.pitchRate     = azimuthSpeed;
    cfg.initialRPM    = initialRPM;
    return cfg;
}

static NavalContext makeCtx()
{
    ymir::Vector6 q{}, qdot{};
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state = bs;
    return ctx;
}

// ---------------------------------------------------------------------------
// Unit tests
// ---------------------------------------------------------------------------

TEST_CASE("Thruster RPM 1st-order filter — single step analytic check")
{
    ThrusterConfig cfg = makeCfg(50.0);
    Thruster t(cfg);
    t.setDemand(100.0, 0.0, 0.0);
    t.update(1.0);

    double expected = 100.0 * (1.0 - std::exp(-1.0 / 50.0));
    REQUIRE(t.state().currentRPM == Approx(expected).epsilon(1e-6));
}

TEST_CASE("Thruster RPM reaches ~99.3% of demand after 5 tau")
{
    ThrusterConfig cfg = makeCfg(50.0);
    Thruster t(cfg);
    t.setDemand(100.0, 0.0, 0.0);
    for (int i = 0; i < 250; ++i)
        t.update(1.0);

    // After 5τ: 1 - e^{-5} ≈ 0.9933
    REQUIRE(t.state().currentRPM > 99.3);
    REQUIRE(t.state().currentRPM < 100.0);
}

TEST_CASE("Thruster azimuth rate limiter — positive demand clamps to rate*dt")
{
    ThrusterConfig cfg = makeCfg(50.0, 5.0);
    Thruster t(cfg);
    t.setDemand(0.0, 0.0, 90.0);
    t.update(1.0);

    REQUIRE(t.state().currentAzimuth_deg == Approx(5.0).epsilon(1e-9));
}

TEST_CASE("Thruster azimuth rate limiter — negative demand clamps to -rate*dt")
{
    ThrusterConfig cfg = makeCfg(50.0, 5.0);
    Thruster t(cfg);
    t.setDemand(0.0, 0.0, -90.0);
    t.update(1.0);

    REQUIRE(t.state().currentAzimuth_deg == Approx(-5.0).epsilon(1e-9));
}

TEST_CASE("setDemand does not alter current state before update")
{
    ThrusterConfig cfg = makeCfg();
    Thruster t(cfg);
    t.setDemand(100.0, 0.8, 45.0);

    REQUIRE(t.state().currentRPM          == Approx(0.0));
    REQUIRE(t.state().currentAzimuth_deg  == Approx(0.0));
    REQUIRE(t.state().currentPitch        == Approx(0.0));
}

TEST_CASE("Thruster constructor initialises currentRPM from cfg.initialRPM")
{
    ThrusterConfig cfg = makeCfg(50.0, 5.0, 75.0);
    Thruster t(cfg);

    REQUIRE(t.state().currentRPM == Approx(75.0));
}

TEST_CASE("toCommand reflects current state after update")
{
    ThrusterConfig cfg = makeCfg(50.0, 5.0, 0.0);
    Thruster t(cfg);
    t.setDemand(100.0, 0.8, 90.0);
    t.update(1.0);

    auto cmd = t.toCommand();
    double expectedRPM = 100.0 * (1.0 - std::exp(-1.0 / 50.0));
    REQUIRE(cmd.currentRPM         == Approx(expectedRPM).epsilon(1e-6));
    REQUIRE(cmd.currentAzimuth_deg == Approx(5.0).epsilon(1e-9));
}

// ---------------------------------------------------------------------------
// Integration test: Thruster + ThrustForces
// ---------------------------------------------------------------------------

TEST_CASE("Thruster integration — update+toCommand+setActuatorState produces non-zero force")
{
    // Vessel-layer thruster entity
    ThrusterConfig cfg = makeCfg(50.0, 5.0, 0.0);
    Thruster t(cfg);
    t.setDemand(120.0, 0.8, 0.0);
    t.update(10.0);   // after 10 s: currentRPM = 120*(1-e^{-0.2}) ≈ 21.7 RPM

    REQUIRE(t.state().currentRPM > 0.0);

    // Physics-layer ThrustForces model (aft thruster, forward azimuth)
    ThrustForces::Config tfCfg{};
    ThrustForces::ThrusterConfig tc{};
    tc.position    = {-50.0, 0.0, -3.0};
    tc.diameter    = 5.0;
    tc.pitchRatio  = 0.8;
    tc.nominalRPM  = 120.0;
    tc.azimuth_deg = 0.0;
    tc.initialRPM  = 0.0;
    tfCfg.thrusters.push_back(tc);

    ThrustForces model(tfCfg);
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    model.setActuatorState(0, t.toCommand());
    auto forces = model.compute(ctx.state);

    REQUIRE(forces.f[0] > 0.0);   // positive surge from forward-facing thruster
}
