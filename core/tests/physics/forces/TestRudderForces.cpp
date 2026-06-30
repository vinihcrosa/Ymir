#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/physics/forces/RudderForces.h>
#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>

#include <array>
#include <cmath>
#include <vector>

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

// Compact symmetric foil table {angle_deg, Cl, Cd}, ascending in angle.
// Nodes at 10° and 20° let tests hit both an exact node and a midpoint.
static std::vector<std::array<double, 3>> makeTable()
{
    return {
        {   0.0,  0.000, 0.050},
        {  10.0,  0.400, 0.080},
        {  20.0,  0.750, 0.200},
        { 340.0, -0.750, 0.200},
        { 350.0, -0.400, 0.080},
        { 360.0,  0.000, 0.050},
    };
}

static RudderForces::Config makeSingleRudder()
{
    RudderForces::Config cfg{};
    RudderForces::RudderConfig r{};
    r.position       = {-50.0, 0.0, -3.0};
    r.area           = 20.0;
    r.hullEfficiency = 1.0;          // unit wake so inflow == speedToWater
    r.coefficients   = makeTable();
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

TEST_CASE("RudderForces Cl/Cd come from the coefficient table at the inflow incidence")
{
    // Standalone rudder, unit wake, no sway → axial inflow, alphaVr = 0,
    // so V2 = u² = 25 and beta = -delta. rho = 1025 (Config default).
    constexpr double rho = 1025.0;
    constexpr double area = 20.0;
    const double q = 0.5 * rho * area * 25.0;  // dynamic-pressure × area = 256250

    RudderForces model(makeSingleRudder());
    NavalContext ctx = makeCtx(5.0);
    model.bindContext(&ctx);

    SECTION("exact table node: delta = -10 deg → beta = 10 deg")
    {
        model.setActuatorState(0, {-10.0 * M_PI / 180.0});
        auto forces = model.compute(ctx.state);

        // beta = 10° → Cl = 0.40, Cd = 0.08 (drag coefficient is negated).
        REQUIRE(forces.f[0] == Approx(q * (-0.080)));  // fx = drag (axial inflow)
        REQUIRE(forces.f[1] == Approx(-q * 0.400));    // fy = -lift
    }

    SECTION("interpolated midpoint: delta = -15 deg → beta = 15 deg")
    {
        model.setActuatorState(0, {-15.0 * M_PI / 180.0});
        auto forces = model.compute(ctx.state);

        // beta = 15° → linear midpoint of (10°,20°): Cl = 0.575, Cd = 0.14.
        REQUIRE(forces.f[0] == Approx(q * (-0.140)));
        REQUIRE(forces.f[1] == Approx(-q * 0.575));
    }
}

TEST_CASE("RudderForces slipstream amplifies inflow when a thruster is associated")
{
    // Propeller producing positive thrust: Kt = 0.4·0.8 - 0.1 = 0.22,
    // n = 2 rev/s, D = 5 m → T = rho·n²·D⁴·Kt > 0.
    ThrustForces::Config tcfg{};
    ThrustForces::ThrusterConfig tc{};
    tc.diameter   = 5.0;
    tc.pitchRatio = 0.8;
    tcfg.thrusters.push_back(tc);

    ThrustForces thrust(tcfg);
    NavalContext ctx = makeCtx(5.0);
    thrust.bindContext(&ctx);
    thrust.setActuatorState(0, {120.0, 0.0, 0.8});  // RPM, azimuth, pitch
    thrust.compute(ctx.state);                       // populate cached thrust
    REQUIRE(thrust.getThrust(0) > 0.0);

    // Two identical rudders: one standalone, one fed by the propeller slipstream.
    auto makeCfg = [](bool linked) {
        RudderForces::Config cfg{};
        RudderForces::RudderConfig r{};
        r.position         = {-50.0, 0.0, -3.0};
        r.area             = 20.0;
        r.hullEfficiency   = 1.0;
        r.thrusterDiameter = 5.0;
        r.p1               = 1.0;
        r.coefficients     = makeTable();
        if (linked) r.thrusterIdx = 0;  // else default = standalone
        cfg.rudders.push_back(r);
        return cfg;
    };

    RudderForces standalone(makeCfg(false));
    RudderForces slipstream(makeCfg(true), &thrust);
    standalone.bindContext(&ctx);
    slipstream.bindContext(&ctx);

    standalone.setActuatorState(0, {-10.0 * M_PI / 180.0});
    slipstream.setActuatorState(0, {-10.0 * M_PI / 180.0});

    auto fStandalone = standalone.compute(ctx.state);
    auto fSlip       = slipstream.compute(ctx.state);

    // Slipstream raises inflow speed, so the (same-sign) sway force grows.
    REQUIRE(std::abs(fSlip.f[1]) > std::abs(fStandalone.f[1]));
    REQUIRE(fSlip.f[1] * fStandalone.f[1] > 0.0);

    // Quantitatively: relativeVx = Vi + p2 with Vi = 5,
    // p2 = sqrt(25 + 8T/(rho·pi·D²)) - 5 ⇒ force ∝ relativeVx².
    const double T  = thrust.getThrust(0);
    const double Vi = 5.0;
    const double p2 = std::sqrt(Vi * Vi + 8.0 * T / (1025.0 * M_PI * 25.0)) - Vi;
    const double vx = Vi + p2;
    REQUIRE(std::abs(fSlip.f[1]) == Approx(std::abs(fStandalone.f[1]) * (vx * vx) / 25.0));
}
