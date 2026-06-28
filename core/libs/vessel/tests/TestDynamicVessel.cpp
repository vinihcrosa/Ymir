#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/vessel/DynamicVessel.h>
#include <ymir/vessel/VesselConfig.h>
#include <ymir/vessel/VesselState.h>
#include <ymir/vessel/controllers/PrescribedController.h>
#include <ymir/vessel/controllers/ManeuverController.h>
#include <ymir/vessel/controllers/BerthManeuverSystem.h>
#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/physics/forces/RudderForces.h>
#include <ymir/physics/BodyState.h>

#include <stdexcept>

using Catch::Approx;
using namespace ymir::naval;
using ymir::vessel::OperationalState;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static NavalContext makeCtx(double x = 0.0, double y = 0.0, double yaw = 0.0,
                             double u = 0.0, double v = 0.0)
{
    ymir::Vector6 q{};
    q[0] = x; q[1] = y; q[5] = yaw;
    ymir::Vector6 qdot{};
    qdot[0] = u; qdot[1] = v;
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state = bs;
    return ctx;
}

struct VesselFixture
{
    VesselConfig cfg;

    explicit VesselFixture(int nThrusters = 1, int nRudders = 1)
    {
        for (int i = 0; i < nThrusters; ++i)
        {
            ThrusterConfig tc{};
            tc.id                   = i;
            tc.rotationTime         = 50.0;
            tc.azimuthSpeed         = 5.0;
            tc.pitchRate            = 5.0;
            tc.rotationSpeedNominal = 120.0;
            tc.initialRPM           = 0.0;
            cfg.thrusters.push_back(tc);
        }
        for (int i = 0; i < nRudders; ++i)
        {
            RudderConfig rc{};
            rc.id           = i;
            rc.angleMaximum = 0.61;
            rc.angleSpeed   = 0.052;
            cfg.rudders.push_back(rc);
        }
    }

    DynamicVessel make() const
    {
        std::vector<Thruster> thrusters;
        for (const auto& tc : cfg.thrusters)
            thrusters.emplace_back(tc);

        std::vector<Rudder> rudders;
        for (const auto& rc : cfg.rudders)
            rudders.emplace_back(rc);

        return DynamicVessel(cfg, std::move(thrusters), std::move(rudders));
    }
};

// ---------------------------------------------------------------------------
// Unit tests
// ---------------------------------------------------------------------------

TEST_CASE("DynamicVessel — default controller is PrescribedController (zero demands)")
{
    VesselFixture f;
    DynamicVessel vessel = f.make();

    NavalContext ctx = makeCtx();
    vessel.updateControl(0.0, 0.1, ctx);
    vessel.updateStates(1.0);

    // PrescribedController with empty tables → zero demands → RPM stays at 0
    REQUIRE(vessel.thruster(0).state().currentRPM == Approx(0.0));
}

TEST_CASE("DynamicVessel — ManeuverController with speedKp > 0 sets non-zero RPM demand")
{
    VesselFixture f;
    DynamicVessel vessel = f.make();

    ManeuverController::Config mc{};
    mc.waypoints.push_back({1000.0, 0.0, 3.0});
    mc.speedKp = 100.0;
    vessel.setController(ManeuverController(mc));

    NavalContext ctx = makeCtx(0.0, 0.0, 0.0, 0.0, 0.0);
    vessel.updateControl(0.0, 0.1, ctx);

    // demandedRPM = speedKp * (demandedSpeed - 0) = 100 * 3 = 300 → clamped or as-is
    REQUIRE(vessel.thruster(0).state().demandedRPM > 0.0);
}

TEST_CASE("DynamicVessel — setController switches active controller without crash")
{
    VesselFixture f;
    DynamicVessel vessel = f.make();

    // Start with ManeuverController
    ManeuverController::Config mc{};
    mc.waypoints.push_back({100.0, 0.0, 2.0});
    mc.speedKp = 10.0;
    vessel.setController(ManeuverController(mc));
    vessel.updateControl(0.0, 0.1, makeCtx());

    // Switch to PrescribedController — must not crash
    PrescribedController::Config pc{};
    vessel.setController(PrescribedController(pc));
    vessel.updateControl(0.1, 0.1, makeCtx());

    // And back to ManeuverController
    vessel.setController(ManeuverController(mc));
    vessel.updateControl(0.2, 0.1, makeCtx());

    // No assertions beyond "did not crash" — existence of this line proves it
    REQUIRE(true);
}

TEST_CASE("DynamicVessel — updateStates advances currentRPM after demand is set")
{
    VesselFixture f;
    DynamicVessel vessel = f.make();

    // PrescribedController with a single-point RPM series: t=0 → 100 RPM
    PrescribedController::Config pc{};
    PrescribedController::TimeSeries ts;
    ts.times  = {0.0};
    ts.values = {100.0};
    pc.thrusterRPM.push_back(ts);
    vessel.setController(PrescribedController(pc));

    vessel.updateControl(0.0, 0.1, makeCtx());  // sets demandedRPM = 100

    double rpmBefore = vessel.thruster(0).state().currentRPM;
    REQUIRE(rpmBefore == Approx(0.0));  // pre-condition: no update yet

    vessel.updateStates(1.0);           // advance: currentRPM += (1-e^{-1/50}) * 100

    REQUIRE(vessel.thruster(0).state().currentRPM > 0.0);
}

TEST_CASE("DynamicVessel — syncToForceModels with null tf is no-op (no crash)")
{
    VesselFixture f;
    DynamicVessel vessel = f.make();

    // Must not crash with either null or both null
    REQUIRE_NOTHROW(vessel.syncToForceModels(nullptr, nullptr));
}

TEST_CASE("DynamicVessel — thruster(idx) throws std::out_of_range for invalid index")
{
    VesselFixture f(2, 1);
    DynamicVessel vessel = f.make();

    REQUIRE_THROWS_AS(vessel.thruster(2),  std::out_of_range);
    REQUIRE_THROWS_AS(vessel.thruster(10), std::out_of_range);
}

TEST_CASE("DynamicVessel — rudder(idx) throws std::out_of_range for invalid index")
{
    VesselFixture f(1, 1);
    DynamicVessel vessel = f.make();

    REQUIRE_THROWS_AS(vessel.rudder(1), std::out_of_range);
}

TEST_CASE("DynamicVessel — thrusterCount and rudderCount match construction")
{
    VesselFixture f(3, 2);
    DynamicVessel vessel = f.make();

    REQUIRE(vessel.thrusterCount() == 3);
    REQUIRE(vessel.rudderCount()   == 2);
}

TEST_CASE("DynamicVessel — vesselState mutable reference allows external state change")
{
    VesselFixture f;
    DynamicVessel vessel = f.make();

    REQUIRE(vessel.vesselState().operationalState == OperationalState::Underway);

    vessel.vesselState().operationalState = OperationalState::Anchored;
    REQUIRE(vessel.vesselState().operationalState == OperationalState::Anchored);

    const auto& cv = vessel;
    REQUIRE(cv.vesselState().operationalState == OperationalState::Anchored);
}

TEST_CASE("DynamicVessel — full tick sequence updateControl→updateStates→syncToForceModels")
{
    VesselFixture f;

    // ThrustForces physics model (1 thruster)
    ThrustForces::Config tfCfg{};
    ThrustForces::ThrusterConfig tc{};
    tc.position   = {-50.0, 0.0, -3.0};
    tc.diameter   = 5.0;
    tc.pitchRatio = 0.8;
    tc.nominalRPM = 120.0;
    tc.azimuth_deg = 0.0;
    tc.initialRPM  = 0.0;
    tfCfg.thrusters.push_back(tc);
    ThrustForces tf(tfCfg);

    // RudderForces physics model (1 rudder)
    RudderForces::Config rfCfg{};
    RudderForces::RudderConfig rc{};
    rc.position     = {-55.0, 0.0, -3.0};
    rc.area         = 20.0;
    rc.aspectRatio  = 2.0;
    rc.thrusterIdx  = std::numeric_limits<std::size_t>::max(); // no slipstream
    rfCfg.rudders.push_back(rc);
    RudderForces rf(rfCfg);

    DynamicVessel vessel = f.make();

    // PrescribedController: 120 RPM from t=0
    PrescribedController::Config pc{};
    PrescribedController::TimeSeries ts;
    ts.times  = {0.0};
    ts.values = {120.0};
    pc.thrusterRPM.push_back(ts);
    vessel.setController(PrescribedController(pc));

    NavalContext ctx = makeCtx();

    vessel.updateControl(0.0, 1.0, ctx);
    vessel.updateStates(1.0);
    vessel.syncToForceModels(&tf, &rf);

    // After 1 tick, thruster should have advanced RPM
    REQUIRE(vessel.thruster(0).state().currentRPM > 0.0);

    // syncToForceModels doesn't crash — already passed above
    // (ThrustForces::compute would require bindContext, out of scope here)
}

// ---------------------------------------------------------------------------
// Integration test: 10 ticks with ManeuverController, RPM is non-zero
// ---------------------------------------------------------------------------

TEST_CASE("DynamicVessel integration — 10 ticks with ManeuverController produce non-zero RPM")
{
    VesselFixture f;
    DynamicVessel vessel = f.make();

    ManeuverController::Config mc{};
    mc.waypoints.push_back({1000.0, 0.0, 3.0}); // far waypoint, demandedSpeed = 3 m/s
    mc.speedKp   = 50.0;                          // RPM demand = speedKp * speedError
    mc.headingKp = 1.0;
    vessel.setController(ManeuverController(mc));

    NavalContext ctx = makeCtx(0.0, 0.0, 0.0);
    const double dt = 1.0;

    for (int i = 0; i < 10; ++i)
    {
        vessel.updateControl(i * dt, dt, ctx);
        vessel.updateStates(dt);
    }

    // After 10 ticks the thruster should have non-zero RPM
    REQUIRE(vessel.thruster(0).state().currentRPM > 0.0);

    // RPM must be coherent with demand: currentRPM <= demandedRPM
    REQUIRE(vessel.thruster(0).state().currentRPM <=
            vessel.thruster(0).state().demandedRPM + 1e-9);
}
