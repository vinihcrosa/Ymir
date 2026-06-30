#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/vessel/Rudder.h>
#include <ymir/physics/forces/RudderForces.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>

#include <limits>
#include <cmath>

using Catch::Approx;
using namespace ymir::naval;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static RudderConfig makeCfg(double angleSpeed   = 0.087,
                             double angleMaximum = 0.61)
{
    RudderConfig cfg{};
    cfg.angleSpeed   = angleSpeed;
    cfg.angleMaximum = angleMaximum;
    return cfg;
}

static NavalContext makeCtx(double surgeMps = 5.0)
{
    ymir::Vector6 q{}, qdot{};
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state             = bs;
    ctx.speedToWater[0]   = surgeMps;
    return ctx;
}

// ---------------------------------------------------------------------------
// Unit tests
// ---------------------------------------------------------------------------

TEST_CASE("Rudder rate limiter — positive demand clamps to rate*dt")
{
    RudderConfig cfg = makeCfg(0.087);
    Rudder r(cfg);
    r.setDemand(0.35);
    r.update(1.0);

    REQUIRE(r.state().currentAngle_rad == Approx(0.087).epsilon(1e-9));
}

TEST_CASE("Rudder rate limiter — negative demand clamps to -rate*dt from non-zero state")
{
    RudderConfig cfg = makeCfg(0.087);
    Rudder r(cfg);
    // Manually advance to 0.35 rad first: ceil(0.35/0.087) = 5 steps
    r.setDemand(0.35);
    for (int i = 0; i < 10; ++i)
        r.update(1.0);
    REQUIRE(r.state().currentAngle_rad == Approx(0.35).epsilon(1e-9));

    // Now demand 0 — one step retreats by 0.087
    r.setDemand(0.0);
    r.update(1.0);

    REQUIRE(r.state().currentAngle_rad == Approx(0.35 - 0.087).epsilon(1e-9));
}

TEST_CASE("Rudder clamping at angleMaximum — currentAngle_rad never exceeds max")
{
    RudderConfig cfg = makeCfg(0.087, 0.61);
    Rudder r(cfg);
    r.setDemand(10.0);   // far beyond max

    for (int i = 0; i < 200; ++i)
        r.update(1.0);

    REQUIRE(r.state().currentAngle_rad <= cfg.angleMaximum + 1e-12);
    REQUIRE(r.state().currentAngle_rad == Approx(0.61).epsilon(1e-9));
}

TEST_CASE("Rudder clamping at negative angleMaximum — currentAngle_rad never below -max")
{
    RudderConfig cfg = makeCfg(0.087, 0.61);
    Rudder r(cfg);
    r.setDemand(-10.0);

    for (int i = 0; i < 200; ++i)
        r.update(1.0);

    REQUIRE(r.state().currentAngle_rad >= -(cfg.angleMaximum + 1e-12));
    REQUIRE(r.state().currentAngle_rad == Approx(-0.61).epsilon(1e-9));
}

TEST_CASE("setDemand does not alter currentAngle_rad before update")
{
    RudderConfig cfg = makeCfg();
    Rudder r(cfg);
    r.setDemand(0.35);

    REQUIRE(r.state().currentAngle_rad  == Approx(0.0));
    REQUIRE(r.state().demandedAngle_rad == Approx(0.35));
}

TEST_CASE("toCommand returns currentAngle_rad after update")
{
    RudderConfig cfg = makeCfg(0.087);
    Rudder r(cfg);
    r.setDemand(0.35);
    r.update(1.0);

    auto cmd = r.toCommand();
    REQUIRE(cmd.currentAngle_rad == Approx(r.state().currentAngle_rad).epsilon(1e-12));
    REQUIRE(cmd.currentAngle_rad == Approx(0.087).epsilon(1e-9));
}

TEST_CASE("Rudder rate limiter — symmetric: positive and negative produce same magnitude")
{
    RudderConfig cfg = makeCfg(0.087);

    Rudder rPos(cfg);
    rPos.setDemand(+0.35);
    rPos.update(1.0);

    Rudder rNeg(cfg);
    rNeg.setDemand(-0.35);
    rNeg.update(1.0);

    REQUIRE(rPos.state().currentAngle_rad == Approx( 0.087).epsilon(1e-9));
    REQUIRE(rNeg.state().currentAngle_rad == Approx(-0.087).epsilon(1e-9));
    REQUIRE(std::abs(rPos.state().currentAngle_rad) ==
            Approx(std::abs(rNeg.state().currentAngle_rad)).epsilon(1e-12));
}

// ---------------------------------------------------------------------------
// Integration test: Rudder entity + RudderForces model
// ---------------------------------------------------------------------------

TEST_CASE("Rudder integration — update+toCommand+setActuatorState produces non-zero force")
{
    // Vessel-layer rudder entity: advance angle to ~0.087 rad in 1 second
    RudderConfig vesselCfg = makeCfg(0.087, 0.61);
    Rudder rudder(vesselCfg);
    rudder.setDemand(0.35);
    rudder.update(1.0);

    REQUIRE(rudder.state().currentAngle_rad > 0.0);

    // Physics-layer RudderForces model
    RudderForces::RudderConfig rc{};
    rc.position    = {-50.0, 0.0, -3.0};
    rc.area        = 20.0;
    rc.thrusterIdx = std::numeric_limits<std::size_t>::max();  // no slipstream
    for (int a = 0; a <= 360; a += 5)  // balanced-rudder foil table {angle_deg, Cl, Cd}
    {
        const double ar = a * M_PI / 180.0;
        rc.coefficients.push_back({static_cast<double>(a),
                                   1.3 * std::sin(2.0 * ar),
                                   0.04 + 0.10 * (1.0 - std::cos(2.0 * ar))});
    }

    RudderForces::Config rfCfg{};
    rfCfg.rudders.push_back(rc);
    rfCfg.rho = 1025.0;

    RudderForces model(rfCfg);

    // Provide non-zero forward speed so rudder lift is computed
    NavalContext ctx = makeCtx(5.0);
    model.bindContext(&ctx);

    model.setActuatorState(0, rudder.toCommand());
    auto forces = model.compute(ctx.state);

    // With positive angle and positive surge speed, expect non-zero sway force
    REQUIRE(std::abs(forces.f[1]) > 0.0);
}
