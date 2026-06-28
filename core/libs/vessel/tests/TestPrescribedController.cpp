#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/vessel/controllers/PrescribedController.h>
#include <ymir/vessel/Thruster.h>
#include <ymir/vessel/Rudder.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace { constexpr double PI = 3.14159265358979323846; }

using Catch::Approx;
using namespace ymir::naval;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static NavalContext makeCtx()
{
    ymir::Vector6 q{}, qdot{};
    NavalContext ctx{};
    ctx.state = ymir::BodyState(q, qdot, 0.0, 0.1);
    return ctx;
}

static ThrusterConfig makeThrusterCfg(double tau = 50.0)
{
    ThrusterConfig cfg{};
    cfg.rotationTime = tau;
    cfg.azimuthSpeed = 5.0;
    cfg.pitchRate    = 5.0;
    return cfg;
}

static RudderConfig makeRudderCfg()
{
    RudderConfig cfg{};
    cfg.angleSpeed   = 0.1;
    cfg.angleMaximum = 0.61;
    return cfg;
}

// Build a TimeSeries with uniformly-spaced times and arbitrary values.
static PrescribedController::TimeSeries makeLinearSeries(
    double t0, double t1, std::size_t n,
    double v0, double v1)
{
    PrescribedController::TimeSeries ts;
    ts.times.resize(n);
    ts.values.resize(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        double alpha   = (n == 1) ? 0.0 : static_cast<double>(i) / static_cast<double>(n - 1);
        ts.times[i]    = t0 + alpha * (t1 - t0);
        ts.values[i]   = v0 + alpha * (v1 - v0);
    }
    return ts;
}

// Build a cosine RPM series: values[i] = amplitude * cos(2π * i / (n-1))
static PrescribedController::TimeSeries makeCosineSeries(
    double t0, double t1, std::size_t n, double amplitude)
{
    PrescribedController::TimeSeries ts;
    ts.times.resize(n);
    ts.values.resize(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        double alpha   = (n == 1) ? 0.0 : static_cast<double>(i) / static_cast<double>(n - 1);
        ts.times[i]    = t0 + alpha * (t1 - t0);
        ts.values[i]   = amplitude * std::cos(2.0 * PI * alpha);
    }
    return ts;
}

// ---------------------------------------------------------------------------
// Unit tests — interpolation behaviour
// ---------------------------------------------------------------------------

TEST_CASE("PrescribedController interpolation at exact table point — error = 0")
{
    // 5-point series: times = {0, 1, 2, 3, 4}, values = {10, 20, 30, 40, 50}
    PrescribedController::TimeSeries ts = makeLinearSeries(0.0, 4.0, 5, 10.0, 50.0);

    PrescribedController::Config cfg;
    cfg.rudderAngle_rad.push_back(ts);
    PrescribedController pc(std::move(cfg));

    RudderConfig rcfg = makeRudderCfg();
    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    std::vector<Thruster> thrusters;
    NavalContext ctx = makeCtx();

    // Exact table point: times[2] == 2.0, values[2] == 30.0
    pc.update(2.0, 0.0, ctx, thrusters, rudders);
    REQUIRE(rudders[0].state().demandedAngle_rad == Approx(30.0).epsilon(1e-12));
}

TEST_CASE("PrescribedController interpolation between two points — error < 1e-6")
{
    PrescribedController::TimeSeries ts = makeLinearSeries(0.0, 4.0, 5, 10.0, 50.0);

    PrescribedController::Config cfg;
    cfg.rudderAngle_rad.push_back(ts);
    PrescribedController pc(std::move(cfg));

    RudderConfig rcfg = makeRudderCfg();
    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    std::vector<Thruster> thrusters;
    NavalContext ctx = makeCtx();

    // Midpoint between times[0]=0 and times[1]=1: t = 0.5
    // Expected: (10 + 20) / 2 = 15.0
    const double tMid     = (ts.times[0] + ts.times[1]) / 2.0;
    const double expected = (ts.values[0] + ts.values[1]) / 2.0;
    pc.update(tMid, 0.0, ctx, thrusters, rudders);
    REQUIRE(rudders[0].state().demandedAngle_rad == Approx(expected).epsilon(1e-6));
}

TEST_CASE("PrescribedController t < times.front() — returns values.front()")
{
    PrescribedController::TimeSeries ts = makeLinearSeries(1.0, 5.0, 5, 100.0, 200.0);

    PrescribedController::Config cfg;
    cfg.rudderAngle_rad.push_back(ts);
    PrescribedController pc(std::move(cfg));

    RudderConfig rcfg = makeRudderCfg();
    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    std::vector<Thruster> thrusters;
    NavalContext ctx = makeCtx();

    pc.update(-999.0, 0.0, ctx, thrusters, rudders);
    REQUIRE(rudders[0].state().demandedAngle_rad == Approx(ts.values.front()).epsilon(1e-12));
}

TEST_CASE("PrescribedController t > times.back() — returns values.back()")
{
    PrescribedController::TimeSeries ts = makeLinearSeries(0.0, 4.0, 5, 100.0, 200.0);

    PrescribedController::Config cfg;
    cfg.rudderAngle_rad.push_back(ts);
    PrescribedController pc(std::move(cfg));

    RudderConfig rcfg = makeRudderCfg();
    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    std::vector<Thruster> thrusters;
    NavalContext ctx = makeCtx();

    pc.update(9999.0, 0.0, ctx, thrusters, rudders);
    REQUIRE(rudders[0].state().demandedAngle_rad == Approx(ts.values.back()).epsilon(1e-12));
}

TEST_CASE("PrescribedController single-point series — any t returns the single value")
{
    PrescribedController::TimeSeries ts;
    ts.times  = {5.0};
    ts.values = {42.0};

    PrescribedController::Config cfg;
    cfg.rudderAngle_rad.push_back(ts);
    PrescribedController pc(std::move(cfg));

    RudderConfig rcfg = makeRudderCfg();
    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    std::vector<Thruster> thrusters;
    NavalContext ctx = makeCtx();

    for (double t : {-100.0, 0.0, 5.0, 100.0})
    {
        rudders[0].setDemand(0.0);  // reset demand
        pc.update(t, 0.0, ctx, thrusters, rudders);
        REQUIRE(rudders[0].state().demandedAngle_rad == Approx(42.0).epsilon(1e-12));
    }
}

// ---------------------------------------------------------------------------
// Unit tests — update() interaction with actuators
// ---------------------------------------------------------------------------

TEST_CASE("PrescribedController update() sets correct RPM demand on thruster 0")
{
    // Linear series: t=[0..10], values=[0..100]
    PrescribedController::TimeSeries ts = makeLinearSeries(0.0, 10.0, 11, 0.0, 100.0);

    PrescribedController::Config cfg;
    cfg.thrusterRPM.push_back(ts);
    PrescribedController pc(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    std::vector<Thruster> thrusters;
    thrusters.reserve(1);
    thrusters.emplace_back(tcfg);

    std::vector<Rudder> rudders;
    NavalContext ctx = makeCtx();

    // At t=5.0: expected RPM = 50.0 (exact midpoint)
    pc.update(5.0, 0.0, ctx, thrusters, rudders);
    REQUIRE(thrusters[0].state().demandedRPM == Approx(50.0).epsilon(1e-6));
}

TEST_CASE("PrescribedController update() sets correct angle demand on rudder 0")
{
    // Series: t=[0..3.14], values=[0..3.14] (identity)
    PrescribedController::TimeSeries ts = makeLinearSeries(0.0, PI, 10, 0.0, PI);

    PrescribedController::Config cfg;
    cfg.rudderAngle_rad.push_back(ts);
    PrescribedController pc(std::move(cfg));

    RudderConfig rcfg = makeRudderCfg();
    std::vector<Rudder> rudders;
    rudders.reserve(1);
    rudders.emplace_back(rcfg);

    std::vector<Thruster> thrusters;
    NavalContext ctx = makeCtx();

    // Exact table endpoint: t = π → angle = π
    pc.update(PI, 0.0, ctx, thrusters, rudders);
    REQUIRE(rudders[0].state().demandedAngle_rad == Approx(PI).epsilon(1e-6));
}

TEST_CASE("PrescribedController size mismatch — cfg has fewer series than thrusters, no crash")
{
    // Only one RPM series, but two thrusters
    PrescribedController::TimeSeries ts = makeLinearSeries(0.0, 10.0, 5, 0.0, 100.0);

    PrescribedController::Config cfg;
    cfg.thrusterRPM.push_back(ts);  // 1 series
    PrescribedController pc(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg();
    std::vector<Thruster> thrusters;
    thrusters.reserve(2);
    thrusters.emplace_back(tcfg);
    thrusters.emplace_back(tcfg);  // 2 thrusters

    std::vector<Rudder> rudders;
    NavalContext ctx = makeCtx();

    // Must not crash; only thruster[0] is updated
    pc.update(5.0, 0.0, ctx, thrusters, rudders);
    REQUIRE(thrusters[0].state().demandedRPM == Approx(50.0).epsilon(1e-6));
    REQUIRE(thrusters[1].state().demandedRPM == Approx(0.0));  // untouched
}

// ---------------------------------------------------------------------------
// Integration test — cosine RPM series + Thruster dynamic
// ---------------------------------------------------------------------------

TEST_CASE("PrescribedController integration — cosine RPM series demand tracks analytic value")
{
    // 50-point cosine series from t=0 to t=60s: values[i] = 100 * cos(2π * i/49)
    // At t=60s (last point): value = 100 * cos(2π) = 100.0
    constexpr std::size_t N        = 50;
    constexpr double      T_END    = 60.0;
    constexpr double      DT       = 0.1;
    constexpr double      AMPLITUDE = 100.0;

    auto ts = makeCosineSeries(0.0, T_END, N, AMPLITUDE);

    PrescribedController::Config cfg;
    cfg.thrusterRPM.push_back(ts);
    PrescribedController pc(std::move(cfg));

    ThrusterConfig tcfg = makeThrusterCfg(50.0);
    std::vector<Thruster> thrusters;
    thrusters.reserve(1);
    thrusters.emplace_back(tcfg);

    std::vector<Rudder> rudders;
    NavalContext ctx = makeCtx();

    // Run 600 steps of dt=0.1s
    double t = 0.0;
    for (int step = 0; step < static_cast<int>(T_END / DT); ++step)
    {
        pc.update(t, DT, ctx, thrusters, rudders);
        thrusters[0].update(DT);
        t += DT;
    }

    // At t=60.0s (beyond last table point), demanded RPM must equal last value.
    // cos(2π * 49/49) = cos(2π) = 1.0 → last value = 100.0
    pc.update(T_END, DT, ctx, thrusters, rudders);
    REQUIRE(thrusters[0].state().demandedRPM == Approx(ts.values.back()).epsilon(1e-6));

    // currentRPM follows demand with 1st-order filter (τ=50s) — must be in range of series
    const double minVal = *std::min_element(ts.values.begin(), ts.values.end());
    const double maxVal = *std::max_element(ts.values.begin(), ts.values.end());
    REQUIRE(thrusters[0].state().currentRPM >= minVal - 1.0);
    REQUIRE(thrusters[0].state().currentRPM <= maxVal + 1.0);
}
