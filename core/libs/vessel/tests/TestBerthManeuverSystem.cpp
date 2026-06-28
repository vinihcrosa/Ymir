#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/vessel/controllers/BerthManeuverSystem.h>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
#include <ymir/vessel/controllers/TugParametricForces.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#include <ymir/vessel/Thruster.h>
#include <ymir/vessel/Rudder.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>
#include <ymir/world/CouplingRegistry.h>
#include <ymir/common/math/AngleUtils.h>

#include <cmath>
#if !defined(NDEBUG)
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <vector>

using Catch::Approx;
using namespace ymir::naval;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static NavalContext makeCtx(double x = 0.0, double y = 0.0,
                             double yaw = 0.0,
                             double u = 0.0, double v = 0.0,
                             double r = 0.0)
{
    ymir::Vector6 q{}, qdot{};
    q[0]    = x;
    q[1]    = y;
    q[5]    = yaw;
    qdot[0] = u;
    qdot[1] = v;
    qdot[5] = r;
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
    cfg.angleSpeed   = 2.0;
    cfg.angleMaximum = 1.0;
    return cfg;
}

/** Build a minimal BSM config with one waypoint and one tug. */
static BerthManeuverSystem::Config makeBsmCfg(
    double wpX = 0.0, double wpY = 100.0,
    double transitionDist = 50.0,
    double headingTarget  = 0.0,
    double escortForce    = 0.0,
    double pushForce      = 0.0,
    double lateralKp      = 1.0,
    double lateralKd      = 0.0,
    double headingKp      = 1.0)
{
    BerthManeuverSystem::Config cfg;
    cfg.headingKp = headingKp;
    cfg.lateralKp = lateralKp;
    cfg.lateralKd = lateralKd;

    BerthManeuverSystem::BerthWaypoint wp;
    wp.x_m               = wpX;
    wp.y_m               = wpY;
    wp.targetPhase       = BerthManeuverSystem::Phase::Sideway;
    wp.transitionDist_m  = transitionDist;
    wp.headingTarget_rad = headingTarget;
    cfg.waypoints.push_back(wp);

    BerthManeuverSystem::TugForceConfig tug;
    tug.escortForce_N = escortForce;
    tug.pushForce_N   = pushForce;
    tug.bearing_deg   = 0.0;
    tug.arm_m         = 0.0;
    cfg.tugs.push_back(tug);

    return cfg;
}

static BerthManeuverSystem::Config makeTwoTugEscortCfg()
{
    auto cfg = makeBsmCfg(0.0, 100.0, 50.0);
    cfg.tugs.clear();

    BerthManeuverSystem::TugForceConfig tug0;
    tug0.escortForce_N = 10.0;
    tug0.pushForce_N   = 50.0;
    tug0.bearing_deg   = 0.0;
    tug0.arm_m         = 2.0;
    cfg.tugs.push_back(tug0);

    BerthManeuverSystem::TugForceConfig tug1;
    tug1.escortForce_N = 20.0;
    tug1.pushForce_N   = 50.0;
    tug1.bearing_deg   = 90.0;
    tug1.arm_m         = 3.0;
    cfg.tugs.push_back(tug1);

    return cfg;
}

static std::vector<Thruster> makeThrusterVec(const ThrusterConfig& tcfg, int n = 1)
{
    std::vector<Thruster> v;
    v.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i)
        v.emplace_back(tcfg);
    return v;
}

static std::vector<Rudder> makeRudderVec(const RudderConfig& rcfg, int n = 1)
{
    std::vector<Rudder> v;
    v.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i)
        v.emplace_back(rcfg);
    return v;
}

// ---------------------------------------------------------------------------
// Unit tests — initial state
// ---------------------------------------------------------------------------

TEST_CASE("BSM initial phase is Navigating")
{
    BerthManeuverSystem bsm(makeBsmCfg());
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Navigating);
}

// ---------------------------------------------------------------------------
// Unit tests — Navigating → Sideway transition
// ---------------------------------------------------------------------------

TEST_CASE("BSM Navigating→Sideway: vessel within transitionDist triggers phase change")
{
    // Waypoint at (0, 100), transitionDist = 50 m.
    // Vessel at (0, 70) → dist = 30 < 50 → should transition.
    auto cfg = makeBsmCfg(0.0, 100.0, 50.0);
    BerthManeuverSystem bsm(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    NavalContext ctx = makeCtx(0.0, 70.0);  // dist = 30 m < 50 m
    bsm.update(0.0, 0.1, ctx, thrusters, rudders);

    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Sideway);
}

TEST_CASE("BSM Navigating→Sideway: vessel outside transitionDist stays in Navigating")
{
    // Vessel at (0, 0), waypoint at (0, 100), transitionDist = 50 → dist = 100 > 50.
    auto cfg = makeBsmCfg(0.0, 100.0, 50.0);
    BerthManeuverSystem bsm(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    NavalContext ctx = makeCtx(0.0, 0.0);  // dist = 100 m > 50 m
    bsm.update(0.0, 0.1, ctx, thrusters, rudders);

    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Navigating);
}

// ---------------------------------------------------------------------------
// Unit tests — no reverse transitions
// ---------------------------------------------------------------------------

TEST_CASE("BSM no reverse transition: Sideway does not revert to Navigating on large distance")
{
    // Enter Sideway by placing vessel at (0, 70) (dist 30 < 50).
    auto cfg = makeBsmCfg(0.0, 100.0, 50.0);
    BerthManeuverSystem bsm(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    // First update — enters Sideway.
    bsm.update(0.0, 0.1, makeCtx(0.0, 70.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Sideway);

    // Second update — vessel moves far away.  Phase must remain Sideway.
    bsm.update(0.1, 0.1, makeCtx(0.0, 0.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Sideway);
}

// ---------------------------------------------------------------------------
// Unit tests — Sideway → TurnROTTUG transition
// ---------------------------------------------------------------------------

TEST_CASE("BSM Sideway→TurnROTTUG: lateral error < 2 m and heading error < 5 deg")
{
    // Waypoint at (0, 100), headingTarget = 0. Vessel at (0, 99) (lateral error ≈ 0),
    // heading = 0 (heading error = 0).
    // First update: vessel at (0, 70) → Sideway.
    // Second update: vessel near waypoint with correct heading → TurnROTTUG.

    // headingTarget = 0, wp = (0, 100).
    // At (0, 99): dx=0, dy=1; lateral = -0*sin(0) + 1*cos(0) = 1 < 2. Heading = 0 < 5 deg. OK.
    auto cfg = makeBsmCfg(0.0, 100.0, 50.0, 0.0);
    BerthManeuverSystem bsm(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    // Enter Sideway.
    bsm.update(0.0, 0.1, makeCtx(0.0, 70.0, 0.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Sideway);

    // Trigger TurnROTTUG: lateral error = 1 m < 2 m, heading error = 0 < 5 deg.
    bsm.update(0.1, 0.1, makeCtx(0.0, 99.0, 0.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::TurnROTTUG);
}

TEST_CASE("BSM Sideway→TurnROTTUG: heading error too large — stays in Sideway")
{
    // Lateral error OK but heading error = 10 deg > 5 deg → stays in Sideway.
    auto cfg = makeBsmCfg(0.0, 100.0, 50.0, 0.0);
    BerthManeuverSystem bsm(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    bsm.update(0.0, 0.1, makeCtx(0.0, 70.0, 0.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Sideway);

    const double badYaw = ymir::math::deg2rad(10.0);  // 10 deg heading error
    bsm.update(0.1, 0.1, makeCtx(0.0, 99.0, badYaw), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Sideway);
}

// ---------------------------------------------------------------------------
// Unit tests — FSM terminal state
// ---------------------------------------------------------------------------

TEST_CASE("BSM TurnROTTUG is the terminal state — no further transition")
{
    // Enter TurnROTTUG. Call update again with same small-error ctx. Phase unchanged.
    auto cfg = makeBsmCfg(0.0, 100.0, 50.0, 0.0);
    BerthManeuverSystem bsm(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    bsm.update(0.0, 0.1, makeCtx(0.0, 70.0, 0.0), thrusters, rudders);  // → Sideway
    bsm.update(0.1, 0.1, makeCtx(0.0, 99.0, 0.0), thrusters, rudders);  // → TurnROTTUG
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::TurnROTTUG);

    // Additional update — must stay TurnROTTUG.
    bsm.update(0.2, 0.1, makeCtx(0.0, 100.0, 0.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::TurnROTTUG);
}

// ---------------------------------------------------------------------------
// Unit tests — tugForces in Navigating
// ---------------------------------------------------------------------------

TEST_CASE("BSM tugForces: Navigating with escortForce_N = 0 returns zero Vector6")
{
    auto cfg = makeBsmCfg(0.0, 100.0, 50.0, 0.0, 0.0 /*escortForce*/);
    BerthManeuverSystem bsm(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    // Vessel far from waypoint (stays in Navigating).
    bsm.update(0.0, 0.1, makeCtx(0.0, 0.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Navigating);

    const auto& tf = bsm.tugForces();
    for (int i = 0; i < 6; ++i)
        REQUIRE(tf[i] == Approx(0.0).margin(1e-12));
}

TEST_CASE("BSM tugForces: Navigating without registry accumulates expected escorting forces")
{
    BerthManeuverSystem bsm(makeTwoTugEscortCfg());

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    bsm.update(0.0, 0.1, makeCtx(0.0, 0.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Navigating);

    const auto& tf = bsm.tugForces();
    REQUIRE(tf[0] == Approx(10.0).margin(1e-12));
    REQUIRE(tf[1] == Approx(20.0).margin(1e-12));
    REQUIRE(tf[2] == Approx(0.0).margin(1e-12));
    REQUIRE(tf[3] == Approx(0.0).margin(1e-12));
    REQUIRE(tf[4] == Approx(0.0).margin(1e-12));
    REQUIRE(tf[5] == Approx(60.0).margin(1e-12));
}

TEST_CASE("BSM registry mode writes per-tug forces and leaves tugForces zero")
{
    constexpr int shipBodyId      = 7;
    constexpr int tugBodyId0      = 101;
    constexpr int tugBodyId1      = 102;
    constexpr int observerBodyId0 = 201;
    constexpr int observerBodyId1 = 202;

    ymir::CouplingRegistry registry;
    registry.addLink(tugBodyId0, observerBodyId0);
    registry.addLink(tugBodyId1, observerBodyId1);

    BerthManeuverSystem bsm(makeTwoTugEscortCfg());
    bsm.setCouplingRegistry(&registry, shipBodyId, {tugBodyId0, tugBodyId1});

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    bsm.update(0.0, 0.1, makeCtx(0.0, 0.0), thrusters, rudders);
    registry.resolve();

    const ymir::Forces tug0 = registry.consumedForce(observerBodyId0);
    const ymir::Forces tug1 = registry.consumedForce(observerBodyId1);

    REQUIRE(tug0.f[0] == Approx(10.0).margin(1e-12));
    REQUIRE(tug0.f[1] == Approx(0.0).margin(1e-12));
    REQUIRE(tug0.f[5] == Approx(0.0).margin(1e-12));

    REQUIRE(tug1.f[0] == Approx(0.0).margin(1e-12));
    REQUIRE(tug1.f[1] == Approx(20.0).margin(1e-12));
    REQUIRE(tug1.f[5] == Approx(60.0).margin(1e-12));

    for (int i = 0; i < 6; ++i)
        REQUIRE(bsm.tugForces()[i] == Approx(0.0).margin(1e-12));
}

#if !defined(NDEBUG)
TEST_CASE("BSM setCouplingRegistry asserts when tug body id count is invalid")
{
    const pid_t pid = fork();
    REQUIRE(pid >= 0);

    if (pid == 0)
    {
        std::signal(SIGABRT, SIG_DFL);
        const int devNull = open("/dev/null", O_WRONLY);
        if (devNull >= 0)
        {
            dup2(devNull, STDOUT_FILENO);
            dup2(devNull, STDERR_FILENO);
            close(devNull);
        }

        ymir::CouplingRegistry registry;
        BerthManeuverSystem bsm(makeTwoTugEscortCfg());
        bsm.setCouplingRegistry(&registry, 7, {101});
        std::_Exit(EXIT_SUCCESS);
    }

    int status = 0;
    REQUIRE(waitpid(pid, &status, 0) == pid);
    REQUIRE(WIFSIGNALED(status));
    REQUIRE(WTERMSIG(status) == SIGABRT);
}
#endif

// ---------------------------------------------------------------------------
// Unit tests — tugForces in Sideway
// ---------------------------------------------------------------------------

TEST_CASE("BSM tugForces: Sideway with pushForce_N > 0 returns positive lateral force")
{
    // Waypoint at (0, 100), headingTarget = 0.
    // Vessel at (0, 70): dist = 30 < 50 → transitions to Sideway in first update.
    // At (0, 70): dx=0, dy=30; lateral = -0*sin(0)+30*cos(0) = 30 > 0.
    // PD: force = lateralKp * 30 + 0 = 30 → clamped to pushForce_N = 20.
    // tugForces[1] = 20 > 0.

    constexpr double pushForce = 20.0;
    auto cfg = makeBsmCfg(0.0, 100.0, 50.0, 0.0,
                           0.0 /*escortForce*/, pushForce,
                           1.0 /*lateralKp*/, 0.0 /*lateralKd*/);
    BerthManeuverSystem bsm(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    // This single call both triggers Sideway and runs Sideway logic.
    bsm.update(0.0, 0.1, makeCtx(0.0, 70.0, 0.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Sideway);
    REQUIRE(bsm.tugForces()[1] > 0.0);
    REQUIRE(bsm.tugForces()[1] == Approx(pushForce).epsilon(1e-9));
}

// ---------------------------------------------------------------------------
// Unit tests — TugParametricForces
// ---------------------------------------------------------------------------

TEST_CASE("TugParametricForces::computeNaval returns forces matching bsm.tugForces()")
{
    // Enter Sideway with a non-zero push force so tugForces is non-trivial.
    constexpr double pushForce = 15.0;
    auto cfg = makeBsmCfg(0.0, 100.0, 50.0, 0.0,
                           0.0, pushForce,
                           1.0, 0.0);
    BerthManeuverSystem bsm(std::move(cfg));
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    TugParametricForces tpf(&bsm);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    bsm.update(0.0, 0.1, makeCtx(0.0, 70.0, 0.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Sideway);

    // Bind context (TugParametricForces reads tugForces() — no ctx needed internally).
    NavalContext ctx = makeCtx(0.0, 70.0, 0.0);
    tpf.bindContext(&ctx);

    ymir::Vector6 q{}, qdot{};
    ymir::BodyState state(q, qdot, 0.0, 0.1);
    const ymir::Forces result = tpf.compute(state);

    const auto& tf = bsm.tugForces();
    for (int i = 0; i < 6; ++i)
        REQUIRE(result.f[i] == Approx(tf[i]).margin(1e-12));
}

// ---------------------------------------------------------------------------
// Integration test — Navigating→Sideway: rudder demand changes + lateral force
// ---------------------------------------------------------------------------

TEST_CASE("BSM integration: after Navigating→Sideway, heading PID replaces LOS and tugForces lateral appears")
{
    // Ship at (0, 0), heading = PI/2 (pointing +y). Waypoint at (0, 100), headingTarget = 0.
    // In Navigating: LOS bearing = atan2(100, 0) = PI/2; heading error = wrapToPi(PI/2 - PI/2) = 0 → rudder ≈ 0.
    // In Sideway: heading error = wrapToPi(0 - PI/2) = -PI/2 → large rudder demand (clamped to maxRudder).
    // Also: lateral error = -0*sin(0) + 30*cos(0) = 30 → tugForces[1] = pushForce.

    constexpr double pushForce = 25.0;
    BerthManeuverSystem::Config cfg;
    cfg.headingKp          = 1.0;
    cfg.lateralKp          = 1.0;
    cfg.maxRudderAngle_rad = 0.5;

    BerthManeuverSystem::BerthWaypoint wp;
    wp.x_m               = 0.0;
    wp.y_m               = 100.0;
    wp.targetPhase       = BerthManeuverSystem::Phase::Sideway;
    wp.transitionDist_m  = 50.0;
    wp.headingTarget_rad = 0.0;  // target heading = 0 (pointing +x)
    cfg.waypoints.push_back(wp);

    BerthManeuverSystem::TugForceConfig tug;
    tug.escortForce_N = 0.0;
    tug.pushForce_N   = pushForce;
    tug.bearing_deg   = 0.0;
    tug.arm_m         = 0.0;
    cfg.tugs.push_back(tug);

    BerthManeuverSystem bsm(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    RudderConfig   rcfg = makeRudderCfg();
    rcfg.angleSpeed   = 10.0;  // fast rate
    rcfg.angleMaximum = 1.0;
    auto thrusters = makeThrusterVec(tcfg);
    auto rudders   = makeRudderVec(rcfg);

    // Step 1: vessel far from waypoint (100 m) — Navigating phase.
    // LOS bearing = atan2(100, 0) = PI/2; vessel heading = PI/2 → heading error = 0 → rudder ≈ 0.
    bsm.update(0.0, 0.1, makeCtx(0.0, 0.0, ymir::math::PI / 2.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Navigating);
    const double rudderNavigating = std::abs(rudders[0].state().demandedAngle_rad);

    // Step 2: vessel at (0, 70) — within transitionDist → enters Sideway.
    // Vessel heading = PI/2, headingTarget = 0 → heading error = -PI/2 → large rudder demand.
    bsm.update(0.1, 0.1, makeCtx(0.0, 70.0, ymir::math::PI / 2.0), thrusters, rudders);
    REQUIRE(bsm.currentPhase() == BerthManeuverSystem::Phase::Sideway);

    const double rudderSideway    = std::abs(rudders[0].state().demandedAngle_rad);
    const double lateralForceSideway = bsm.tugForces()[1];

    // In Sideway, heading error is large (PI/2) → rudder demand is clamped to max.
    // In Navigating, heading error was 0 → rudder demand was 0.
    REQUIRE(rudderSideway > rudderNavigating);
    REQUIRE(lateralForceSideway > 0.0);
}
