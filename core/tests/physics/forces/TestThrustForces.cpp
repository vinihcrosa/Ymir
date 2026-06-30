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

// ---------------------------------------------------------------------------
// Open-water Kt/Kq table physics (matches MATLAB dynamics/thruster.m)
// ---------------------------------------------------------------------------

namespace
{
constexpr double kRho = 1025.0;

// Body-frame velocity relative to current: u (surge), v (sway), r (yaw rate).
NavalContext makeCtxVel(double u = 0.0, double v = 0.0, double r = 0.0)
{
    ymir::Vector6 q{}, qdot{};
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state = bs;
    ctx.speedToWater = {u, v, 0.0, 0.0, 0.0, r};
    return ctx;
}

// Reference open-water curve: {J, Kq, Kt}, J ascending.
ThrustForces::Config makeTableThruster(double diameter = 5.0)
{
    ThrustForces::Config cfg{};
    cfg.rho = kRho;
    ThrustForces::ThrusterConfig t{};
    t.position    = {0.0, 0.0, 0.0};   // at origin: isolate axial force, no moments/levers
    t.diameter    = diameter;
    t.openWaterCurve = {
        {0.0, 0.05, 0.50},
        {0.5, 0.04, 0.40},
        {1.0, 0.02, 0.20},
    };
    t.rotationSpeedMax     = 150.0;
    t.maximumPowerW        = 1e12;     // effectively unlimited → no saturation
    t.mechanicalEfficiency = 1.0;
    t.asternEfficiency     = 1.0;
    t.transversalSpeedLimit = 2.0;
    t.transversalReductionCoeff = 0.5;
    cfg.thrusters.push_back(t);
    return cfg;
}
} // namespace

TEST_CASE("ThrustForces open-water Kt at J=0 (table endpoint) matches hand value")
{
    // n = 1 rev/s (60 RPM), pitch P/D = 0.8, D = 5, zero inflow → J = 0 → Kt = 0.50.
    ThrustForces model(makeTableThruster());
    NavalContext ctx = makeCtxVel();   // zero velocity → J = 0
    model.bindContext(&ctx);

    model.setActuatorState(0, {60.0, 0.0, 0.8});
    auto forces = model.compute(ctx.state);

    const double n = 1.0, D = 5.0, Kt = 0.50;
    const double expectedT = kRho * n * n * std::pow(D, 4) * Kt;  // 320312.5 N
    REQUIRE(forces.f[0] == Approx(expectedT));
    REQUIRE(model.getThrust(0) == Approx(expectedT));
}

TEST_CASE("ThrustForces open-water Kq/Kt interpolated at J=0.25")
{
    // pitch=1.0, n=1, D=5 → denom = 5; Va = 1.25 m/s → J = 0.25 (midpoint of [0,0.5]).
    // Kt = 0.5·0.50 + 0.5·0.40 = 0.45 ; Kq = 0.5·0.05 + 0.5·0.04 = 0.045.
    ThrustForces model(makeTableThruster());
    NavalContext ctx = makeCtxVel(1.25);   // surge inflow 1.25 m/s
    model.bindContext(&ctx);

    model.setActuatorState(0, {60.0, 0.0, 1.0});
    auto forces = model.compute(ctx.state);

    const double n = 1.0, D = 5.0, Kt = 0.45, Kq = 0.045;
    const double expectedT  = kRho * n * n * std::pow(D, 4) * Kt;       // 288281.25 N
    const double expectedQ  = kRho * n * n * std::pow(D, 5) * Kq;       // 144140.625 N·m
    REQUIRE(forces.f[0] == Approx(expectedT));
    REQUIRE(model.getTorque(0) == Approx(expectedQ));
}

TEST_CASE("ThrustForces shaft power computed from Kq (2*pi*n*Q)")
{
    ThrustForces model(makeTableThruster());
    NavalContext ctx = makeCtxVel();
    model.bindContext(&ctx);

    model.setActuatorState(0, {60.0, 0.0, 0.8});  // n=1, J=0 → Kq=0.05
    model.compute(ctx.state);

    const double torque = kRho * 1.0 * std::pow(5.0, 5) * 0.05;   // 160156.25 N·m
    const double power  = 2.0 * M_PI * 1.0 * torque;             // ~1.006e6 W
    REQUIRE(model.getTorque(0) == Approx(torque));
    REQUIRE(model.getPower(0)  == Approx(power));
}

TEST_CASE("ThrustForces power saturation caps shaft power and reduces thrust")
{
    auto cfg = makeTableThruster();
    cfg.thrusters[0].maximumPowerW = 200000.0;   // below the ~1.006e6 W unsaturated demand
    ThrustForces model(cfg);
    NavalContext ctx = makeCtxVel();              // J=0 → Kq=0.05 independent of n
    model.bindContext(&ctx);

    model.setActuatorState(0, {60.0, 0.0, 0.8});
    auto forces = model.compute(ctx.state);

    // Power must be brought under the ceiling...
    REQUIRE(model.getPower(0) <= Approx(200000.0).epsilon(1e-9));
    REQUIRE(model.getPower(0) > 0.0);
    // ...and the delivered thrust is well below the unsaturated 320312.5 N.
    const double unsaturated = kRho * 1.0 * std::pow(5.0, 4) * 0.50;
    REQUIRE(forces.f[0] < unsaturated);
    REQUIRE(forces.f[0] > 0.0);
}

TEST_CASE("ThrustForces astern efficiency penalises reverse thrust")
{
    auto cfgFull = makeTableThruster();          // asternEfficiency = 1.0
    auto cfgHalf = makeTableThruster();
    cfgHalf.thrusters[0].asternEfficiency = 0.5;

    ThrustForces full(cfgFull);
    ThrustForces half(cfgHalf);
    NavalContext ctx = makeCtxVel();
    full.bindContext(&ctx);
    half.bindContext(&ctx);

    // Reverse rotation, positive pitch → astern operation, thrust negative.
    full.setActuatorState(0, {-60.0, 0.0, 0.8});
    half.setActuatorState(0, {-60.0, 0.0, 0.8});
    full.compute(ctx.state);
    half.compute(ctx.state);

    REQUIRE(full.getThrust(0) < 0.0);
    REQUIRE(half.getThrust(0) == Approx(0.5 * full.getThrust(0)));
}

TEST_CASE("ThrustForces transverse inflow reduces thrust")
{
    ThrustForces straight(makeTableThruster());
    ThrustForces crossed(makeTableThruster());
    NavalContext ctx0  = makeCtxVel(0.0, 0.0);   // no transverse inflow → reduc = 1
    NavalContext ctxXv = makeCtxVel(0.0, 1.0);   // sway 1 m/s, limit 2 → ratio 0.5
    straight.bindContext(&ctx0);
    crossed.bindContext(&ctxXv);

    straight.setActuatorState(0, {60.0, 0.0, 0.8});
    crossed.setActuatorState(0, {60.0, 0.0, 0.8});
    straight.compute(ctx0.state);
    crossed.compute(ctxXv.state);

    // formFactor 0.5 ⇒ reduc = (1-0.5)·(1-0) = 0.5.
    REQUIRE(crossed.getThrust(0) == Approx(0.5 * straight.getThrust(0)));
}

TEST_CASE("ThrustForces paddle side-wash force appears only astern")
{
    auto cfg = makeTableThruster();
    cfg.thrusters[0].paddleCoeffs = {5000.0, 0.0, 0.0};  // c0=5000, c1→10, c2→1 defaults
    ThrustForces model(cfg);
    NavalContext ctx = makeCtxVel();
    model.bindContext(&ctx);

    // Astern (negative rotation) → thrust<0 → paddle active, lateral (sway) force.
    // ratio = |pitch·nEff / nMaxRev| = |0.8·(-1)/2.5| = 0.32 ; paddle = 5000·1·0.32 = 1600 N.
    model.setActuatorState(0, {-60.0, 0.0, 0.8});
    auto astern = model.compute(ctx.state);
    REQUIRE(astern.f[1] == Approx(1600.0));

    // Ahead (positive rotation) → thrust>0 → no paddle, no lateral force.
    model.setActuatorState(0, {60.0, 0.0, 0.8});
    auto ahead = model.compute(ctx.state);
    REQUIRE(ahead.f[1] == Approx(0.0).margin(1e-6));
}
