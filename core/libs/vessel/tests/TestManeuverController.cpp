#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/vessel/controllers/ManeuverController.h>
#include <ymir/vessel/Thruster.h>
#include <ymir/vessel/Rudder.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>
#include <ymir/common/math/AngleUtils.h>

#include <cmath>
#include <vector>

using Catch::Approx;
using namespace ymir::naval;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static NavalContext makeCtx(double x = 0.0, double y = 0.0,
                             double yaw = 0.0,
                             double u = 0.0, double v = 0.0)
{
    ymir::Vector6 q{}, qdot{};
    q[0] = x;
    q[1] = y;
    q[5] = yaw;
    qdot[0] = u;
    qdot[1] = v;
    NavalContext ctx{};
    ctx.state = ymir::BodyState(q, qdot, 0.0, 0.1);
    return ctx;
}

static ThrusterConfig makeThrusterCfg()
{
    ThrusterConfig cfg{};
    cfg.rotationTime = 50.0;
    cfg.azimuthSpeed = 5.0;
    cfg.pitchRate    = 5.0;
    return cfg;
}

static RudderConfig makeRudderCfg()
{
    RudderConfig cfg{};
    cfg.angleSpeed   = 2.0;   // fast rate for tests
    cfg.angleMaximum = 1.0;
    return cfg;
}

static ManeuverController::Config makeCfg(
    double kpH = 1.0, double kiH = 0.0, double kdH = 0.0,
    double kpS = 1.0, double kiS = 0.0, double kdS = 0.0,
    double captureRadius = 20.0)
{
    ManeuverController::Config cfg;
    cfg.headingKp      = kpH;
    cfg.headingKi      = kiH;
    cfg.headingKd      = kdH;
    cfg.speedKp        = kpS;
    cfg.speedKi        = kiS;
    cfg.speedKd        = kdS;
    cfg.captureRadius_m = captureRadius;
    return cfg;
}

// ---------------------------------------------------------------------------
// Unit tests — LOS bearing
// ---------------------------------------------------------------------------

TEST_CASE("ManeuverController LOS — vessel at origin, waypoint at (100,0), heading 0 — rudder demand = 0")
{
    // bearing = atan2(0, 100) = 0; headingError = wrapToPi(0 - 0) = 0; demand = 0
    auto cfg = makeCfg();
    cfg.waypoints.push_back({100.0, 0.0, 3.0});
    ManeuverController ctrl(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();

    std::vector<Thruster> thrusters;
    thrusters.reserve(1);
    thrusters.emplace_back(tcfg);

    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    NavalContext ctx = makeCtx(0.0, 0.0, 0.0);
    ctrl.update(0.0, 0.1, ctx, thrusters, rudders);

    REQUIRE(rudders[0].state().demandedAngle_rad == Approx(0.0).margin(1e-10));
}

TEST_CASE("ManeuverController LOS — waypoint at (0,100), heading 0 — rudder demand = kp * pi/2")
{
    // bearing = atan2(100, 0) = pi/2; headingError = pi/2; demand = 1.0 * pi/2
    constexpr double kp = 1.0;
    auto cfg = makeCfg(kp);
    cfg.waypoints.push_back({0.0, 100.0, 3.0});
    ManeuverController ctrl(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();

    std::vector<Thruster> thrusters;
    thrusters.reserve(1);
    thrusters.emplace_back(tcfg);

    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    NavalContext ctx = makeCtx(0.0, 0.0, 0.0);
    ctrl.update(0.0, 0.1, ctx, thrusters, rudders);

    const double expectedDemand = kp * (ymir::math::PI / 2.0);
    REQUIRE(rudders[0].state().demandedAngle_rad == Approx(expectedDemand).epsilon(1e-9));
}

// ---------------------------------------------------------------------------
// Unit tests — wrapToPi
// ---------------------------------------------------------------------------

TEST_CASE("ManeuverController wrapToPi — bearing 355 deg, yaw 0 — error = -5 deg")
{
    // Raw diff = 355 deg in rad; wrapToPi should give -5 deg
    constexpr double deg2rad = ymir::math::PI / 180.0;
    const double bearing355  = 355.0 * deg2rad;
    const double yaw0        = 0.0;

    // Controller will compute wrapToPi(bearing - yaw); verify directly
    const double error = ymir::math::wrapToPi(bearing355 - yaw0);
    REQUIRE(error == Approx(-5.0 * deg2rad).epsilon(1e-9));
}

TEST_CASE("ManeuverController wrapToPi — bearing 179 deg, yaw -179 deg — error ~ -2 deg")
{
    // Validates cross-pi wrap: 179 - (-179) = 358 deg → wraps to -2 deg
    constexpr double deg2rad = ymir::math::PI / 180.0;
    const double error = ymir::math::wrapToPi(179.0 * deg2rad - (-179.0 * deg2rad));
    REQUIRE(error == Approx(-2.0 * deg2rad).epsilon(1e-9));
}

// ---------------------------------------------------------------------------
// Unit tests — waypoint capture
// ---------------------------------------------------------------------------

TEST_CASE("ManeuverController waypoint capture — vessel inside captureRadius — activeIdx advances to 1")
{
    // Vessel at (0,0), waypoint at (10,0), captureRadius = 20 → distance=10 < 20 → captured
    auto cfg = makeCfg(1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 20.0);
    cfg.waypoints.push_back({10.0, 0.0, 3.0});   // waypoint 0 — will be captured
    cfg.waypoints.push_back({200.0, 0.0, 3.0});  // waypoint 1 — far away
    ManeuverController ctrl(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();

    std::vector<Thruster> thrusters;
    thrusters.reserve(1);
    thrusters.emplace_back(tcfg);

    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    NavalContext ctx = makeCtx(0.0, 0.0, 0.0);
    ctrl.update(0.0, 0.1, ctx, thrusters, rudders);

    REQUIRE(ctrl.activeWaypointIdx() == 1);
    REQUIRE_FALSE(ctrl.waypointsExhausted());
}

TEST_CASE("ManeuverController waypoint list exhausted — waypointsExhausted true and demands zero")
{
    // Single waypoint; vessel inside captureRadius → list exhausted
    auto cfg = makeCfg(1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 50.0);
    cfg.waypoints.push_back({5.0, 0.0, 3.0});
    ManeuverController ctrl(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();

    std::vector<Thruster> thrusters;
    thrusters.reserve(1);
    thrusters.emplace_back(tcfg);

    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    NavalContext ctx = makeCtx(0.0, 0.0, 0.0);
    ctrl.update(0.0, 0.1, ctx, thrusters, rudders);

    REQUIRE(ctrl.waypointsExhausted());
    REQUIRE(thrusters[0].state().demandedRPM == Approx(0.0));
    REQUIRE(rudders[0].state().demandedAngle_rad == Approx(0.0));
}

// ---------------------------------------------------------------------------
// Unit tests — PID heading
// ---------------------------------------------------------------------------

TEST_CASE("ManeuverController PID heading — kp=1 ki=0 kd=0, error=0.1 rad — demand = 0.1")
{
    // Vessel at origin, heading 0; waypoint placed so bearing ≈ 0.1 rad
    // bearing = atan2(sin(0.1)*100, cos(0.1)*100) = 0.1
    constexpr double kp      = 1.0;
    constexpr double err_rad = 0.1;
    const double     wpX     = std::cos(err_rad) * 100.0;
    const double     wpY     = std::sin(err_rad) * 100.0;

    auto cfg = makeCfg(kp, 0.0, 0.0);
    cfg.captureRadius_m = 5.0;
    cfg.waypoints.push_back({wpX, wpY, 0.0});
    ManeuverController ctrl(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();

    std::vector<Thruster> thrusters;
    thrusters.reserve(1);
    thrusters.emplace_back(tcfg);

    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    NavalContext ctx = makeCtx(0.0, 0.0, 0.0);
    ctrl.update(0.0, 0.1, ctx, thrusters, rudders);

    // With ki=kd=0: output = kp * error = 1.0 * 0.1 = 0.1
    REQUIRE(rudders[0].state().demandedAngle_rad == Approx(kp * err_rad).epsilon(1e-9));
}

// ---------------------------------------------------------------------------
// Unit tests — PID speed
// ---------------------------------------------------------------------------

TEST_CASE("ManeuverController PID speed — kp=1 ki=0 kd=0, demanded=3 SOG=2 — demand = 1.0")
{
    // speedError = 3.0 - 2.0 = 1.0 → rpmDemand = kp * 1.0 = 1.0
    auto cfg = makeCfg(0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    cfg.waypoints.push_back({1000.0, 0.0, 3.0});  // far away, won't be captured
    ManeuverController ctrl(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();

    std::vector<Thruster> thrusters;
    thrusters.reserve(1);
    thrusters.emplace_back(tcfg);

    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    // u=2 m/s, v=0 → SOG = 2 m/s
    NavalContext ctx = makeCtx(0.0, 0.0, 0.0, 2.0, 0.0);
    ctrl.update(0.0, 0.1, ctx, thrusters, rudders);

    REQUIRE(thrusters[0].state().demandedRPM == Approx(1.0).epsilon(1e-9));
}

// ---------------------------------------------------------------------------
// Integration test — simulated motion toward waypoint
// ---------------------------------------------------------------------------

TEST_CASE("ManeuverController integration — vessel advances toward waypoint over 100 steps")
{
    // Vessel at (0,0), heading=0. Waypoint at (100,0).
    // captureRadius = 10m so waypoint is not immediately captured (distance = 100m).
    // kp_speed=0.01 (maps speed error in m/s to RPM; 1 RPM ≡ 1 m/s in this test).
    // We interpret demandedRPM directly as SOG in m/s (simplified test dynamics).
    constexpr double kpH    = 1.0;
    constexpr double kpS    = 1.0;
    constexpr double dt     = 0.1;
    constexpr int    steps  = 100;
    constexpr double WP_X   = 100.0;
    constexpr double WP_Y   = 0.0;
    constexpr double WP_SPD = 5.0;

    auto cfg = makeCfg(kpH, 0.0, 0.0, kpS, 0.0, 0.0, 10.0);
    cfg.waypoints.push_back({WP_X, WP_Y, WP_SPD});
    ManeuverController ctrl(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();

    std::vector<Thruster> thrusters;
    thrusters.reserve(1);
    thrusters.emplace_back(tcfg);

    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    double x   = 0.0;
    double y   = 0.0;
    double yaw = 0.0;
    double sog = 0.0;

    const double initialDist = std::sqrt((WP_X - x) * (WP_X - x) + (WP_Y - y) * (WP_Y - y));

    for (int i = 0; i < steps; ++i)
    {
        if (ctrl.waypointsExhausted())
            break;

        NavalContext ctx = makeCtx(x, y, yaw, sog, 0.0);
        ctrl.update(0.0, dt, ctx, thrusters, rudders);

        // Simplified dynamics: RPM demand → SOG, rudder demand → yaw rate
        sog  = std::max(0.0, thrusters[0].state().demandedRPM);
        yaw += rudders[0].state().demandedAngle_rad * dt;
        x   += sog * std::cos(yaw) * dt;
        y   += sog * std::sin(yaw) * dt;
    }

    const double finalDist = std::sqrt((WP_X - x) * (WP_X - x) + (WP_Y - y) * (WP_Y - y));
    REQUIRE(finalDist < initialDist);
}
