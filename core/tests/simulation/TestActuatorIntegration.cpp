/**
 * TestActuatorIntegration.cpp
 *
 * Integration tests verifying that thruster and rudder actuator commands produce
 * real motion in NavalDomain.  These tests exist because the WASM bindings had
 * stub setThrusterCommand / setRudderAngle methods and vessels were not moving.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/simulation/NavalDomain.h>
#include <ymir/world/World.h>
#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/physics/forces/RudderForces.h>
#include <ymir/physics/forces/DampingForces.h>
#include <ymir/physics/forces/CurrentForces.h>
#include <ymir/physics/forces/InertialForces.h>
#include <ymir/physics/forces/RestoringForces.h>
#include <ymir/common/Types.h>

#include <cmath>
#include <memory>

using Catch::Approx;
using namespace ymir::naval;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ymir::CvodeConfig defaultCfg()
{
    ymir::CvodeConfig cfg{};
    cfg.reltol   = 1e-4;
    cfg.abstol   = 1e-6;
    cfg.maxStep  = 0.005;  // 5ms max internal step
    cfg.maxSteps = 100000;
    return cfg;
}

static std::unique_ptr<ymir::RigidBody6DOF> makeBody(int id, double mass = 1e5)
{
    ymir::Matrix6x6 M{};
    for (int i = 0; i < 6; ++i) M[i][i] = mass;
    ymir::Matrix6x6 A{};
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    return std::make_unique<ymir::RigidBody6DOF>(id, M, A, q, qdot, defaultCfg());
}

// Body with realistic rotational inertia for a 100m vessel — prevents stiff dynamics
// when CurrentForces (Obokata) applies yaw/pitch moments via Cdz coefficients.
static std::unique_ptr<ymir::RigidBody6DOF> makeRealisticBody(int id, double mass = 1e5)
{
    ymir::Matrix6x6 M{};
    M[0][0] = mass;
    M[1][1] = mass;
    M[2][2] = mass;
    M[3][3] = mass * 5.0;       // roll  ~  I = m*B²/12 for B=25m
    M[4][4] = mass * 833.3;     // pitch ~  I = m*L²/12 for L=100m
    M[5][5] = mass * 867.0;     // yaw   ~  I = m*(L²+B²)/12
    ymir::Matrix6x6 A{};
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    return std::make_unique<ymir::RigidBody6DOF>(id, M, A, q, qdot, defaultCfg());
}

static ThrustForces::Config singleThrusterCfg()
{
    ThrustForces::Config c{};
    ThrustForces::ThrusterConfig tc{};
    tc.position    = {-50.0, 0.0, -3.0};
    tc.diameter    = 5.0;
    tc.pitchRatio  = 0.8;
    tc.nominalRPM  = 120.0;
    tc.azimuth_deg = 0.0;
    tc.initialRPM  = 0.0;
    c.thrusters.push_back(tc);
    return c;
}

static CurrentForces::Config simpleCurrentCfg(double length = 100.0, double beam = 20.0)
{
    CurrentForces::Config c{};
    c.model         = ymir::naval::CurrentModel::OBOKATA;
    c.length_BP     = length;
    c.beam          = beam;
    c.frontalHeight = 5.0;
    c.lateralHeight = 5.0;
    c.n_sections    = 10;
    c.angles = {0.0, 90.0, 180.0, 270.0, 360.0};
    // Surge: positive resistance from ahead (0°) and astern (180°)
    c.cdx = {0.05, 0.0, -0.05, 0.0, 0.05};
    c.cdy = {0.0,  1.0,  0.0, -1.0, 0.0};
    c.cdz = {0.0,  0.0,  0.0,  0.0, 0.0};
    return c;
}

// Representative balanced-rudder coefficient table {angle_deg, Cl, Cd} over the
// full inflow range. Cl = 1.2·sin 2β (Cl_max ≈ 1.2), Cd = 0.05 + 0.3·(1 − cos 2β)
// (Cd ≈ 0.25 at 35°) — realistic magnitudes for a ship rudder.
static std::vector<std::array<double, 3>> foilTable()
{
    std::vector<std::array<double, 3>> t;
    for (int a = 0; a <= 360; a += 5)
    {
        const double ar = a * M_PI / 180.0;
        t.push_back({static_cast<double>(a),
                     1.3 * std::sin(2.0 * ar),
                     0.04 + 0.10 * (1.0 - std::cos(2.0 * ar))});
    }
    return t;
}

static RudderForces::Config singleRudderCfg()
{
    RudderForces::Config c{};
    RudderForces::RudderConfig rc{};
    rc.position     = {-52.0, 0.0, -3.0};
    rc.area         = 20.0;
    rc.thrusterIdx  = 0;
    rc.coefficients = foilTable();
    c.rudders.push_back(rc);
    return c;
}

// ---------------------------------------------------------------------------
// Test: thruster at full RPM produces surge (forward motion)
// ---------------------------------------------------------------------------

TEST_CASE("Actuator integration: full-RPM thruster produces positive surge",
          "[actuator][integration]")
{
    ymir::World world;
    auto domainOwned = std::make_unique<ymir::NavalDomain>("naval");
    ymir::NavalDomain* domain = domainOwned.get();
    world.addDomain(std::move(domainOwned));

    domain->addBody(0, makeBody(0));

    auto thrust = std::make_unique<ThrustForces>(singleThrusterCfg());
    ThrustForces* thrustPtr = thrust.get();
    domain->addNavalForceModel(0, std::move(thrust));

    domain->initialize();

    // Set thruster to 100% → 120 RPM
    thrustPtr->setActuatorState(0, ThrustForces::ThrusterCommand{120.0, 0.0, 0.8});

    // Step 10 s
    for (int i = 0; i < 100; ++i)
        world.step(0.1);

    auto st = domain->state(0);
    REQUIRE(st.u() > 0.0);        // positive surge velocity
    REQUIRE(st.q()[0] > 0.0);     // positive surge position
}

// ---------------------------------------------------------------------------
// Test: zero RPM → vessel stays at rest
// ---------------------------------------------------------------------------

TEST_CASE("Actuator integration: zero-RPM thruster leaves vessel at rest",
          "[actuator][integration]")
{
    ymir::World world;
    auto domainOwned = std::make_unique<ymir::NavalDomain>("naval");
    ymir::NavalDomain* domain = domainOwned.get();
    world.addDomain(std::move(domainOwned));

    domain->addBody(0, makeBody(0));

    auto thrust = std::make_unique<ThrustForces>(singleThrusterCfg());
    ThrustForces* thrustPtr = thrust.get();
    domain->addNavalForceModel(0, std::move(thrust));

    domain->initialize();

    thrustPtr->setActuatorState(0, ThrustForces::ThrusterCommand{0.0, 0.0, 0.8});

    for (int i = 0; i < 50; ++i)
        world.step(0.1);

    auto st = domain->state(0);
    REQUIRE(st.u() == Approx(0.0).margin(1e-6));
    REQUIRE(st.q()[0] == Approx(0.0).margin(1e-6));
}

// ---------------------------------------------------------------------------
// Test: rudder at +20° with forward speed produces positive yaw rate
// ---------------------------------------------------------------------------

TEST_CASE("Actuator integration: rudder deflection with forward speed produces yaw",
          "[actuator][integration]")
{
    ymir::World world;
    auto domainOwned = std::make_unique<ymir::NavalDomain>("naval");
    ymir::NavalDomain* domain = domainOwned.get();
    world.addDomain(std::move(domainOwned));

    domain->addBody(0, makeBody(0));

    auto thrust = std::make_unique<ThrustForces>(singleThrusterCfg());
    ThrustForces* thrustPtr = thrust.get();
    domain->addNavalForceModel(0, std::move(thrust));

    auto rudder = std::make_unique<RudderForces>(singleRudderCfg(), thrustPtr);
    RudderForces* rudderPtr = rudder.get();
    domain->addNavalForceModel(0, std::move(rudder));

    domain->initialize();

    // Build up forward speed first
    thrustPtr->setActuatorState(0, ThrustForces::ThrusterCommand{120.0, 0.0, 0.8});
    for (int i = 0; i < 50; ++i)
        world.step(0.1);

    REQUIRE(domain->state(0).u() > 0.5); // vessel must be moving before rudder takes effect

    // Now apply +20° rudder (starboard)
    constexpr double kDegToRad = M_PI / 180.0;
    rudderPtr->setActuatorState(0, RudderForces::RudderCommand{20.0 * kDegToRad});

    // Step another 5 s
    for (int i = 0; i < 50; ++i)
        world.step(0.1);

    auto st = domain->state(0);
    REQUIRE(st.r() != Approx(0.0).margin(1e-6)); // non-zero yaw rate
    REQUIRE(st.q()[5] != Approx(0.0).margin(1e-6)); // heading changed
}

// ---------------------------------------------------------------------------
// Test: powerPct→RPM conversion used in YmirBindings produces same result
// ---------------------------------------------------------------------------

TEST_CASE("Actuator integration: 50% power produces measurable thrust",
          "[actuator][integration]")
{
    ymir::World world;
    auto domainOwned = std::make_unique<ymir::NavalDomain>("naval");
    ymir::NavalDomain* domain = domainOwned.get();
    world.addDomain(std::move(domainOwned));

    domain->addBody(0, makeBody(0));

    auto thrust = std::make_unique<ThrustForces>(singleThrusterCfg());
    ThrustForces* thrustPtr = thrust.get();
    domain->addNavalForceModel(0, std::move(thrust));

    domain->initialize();

    // Replicate YmirBindings power% → RPM conversion
    constexpr double kNominalRPM = 120.0;
    const double rpm = (50.0 / 100.0) * kNominalRPM; // 60 RPM
    thrustPtr->setActuatorState(0, ThrustForces::ThrusterCommand{rpm, 0.0, 0.8});

    for (int i = 0; i < 100; ++i)
        world.step(0.1);

    REQUIRE(domain->state(0).u() > 0.0);
    REQUIRE(domain->state(0).q()[0] > 0.0);
}

// ---------------------------------------------------------------------------
// Test: vessel decelerates when thruster cut to zero (CurrentForces provides drag)
// ---------------------------------------------------------------------------

TEST_CASE("Actuator integration: vessel decelerates after thruster cut to zero",
          "[actuator][integration]")
{
    ymir::World world;
    auto domainOwned = std::make_unique<ymir::NavalDomain>("naval");
    ymir::NavalDomain* domain = domainOwned.get();
    world.addDomain(std::move(domainOwned));

    domain->addBody(0, makeRealisticBody(0));

    // Hull drag (Obokata) — provides surge resistance proportional to velocity²
    domain->addNavalForceModel(0, std::make_unique<CurrentForces>(simpleCurrentCfg()));

    auto thrust = std::make_unique<ThrustForces>(singleThrusterCfg());
    ThrustForces* thrustPtr = thrust.get();
    domain->addNavalForceModel(0, std::move(thrust));

    domain->initialize();

    // Build up speed with moderate thrust (low RPM avoids unrealistically high velocities)
    thrustPtr->setActuatorState(0, ThrustForces::ThrusterCommand{40.0, 0.0, 0.8});
    for (int i = 0; i < 20; ++i)
        world.step(0.1);

    const double uAfterThrust = domain->state(0).u();
    REQUIRE(uAfterThrust > 0.01);  // must be moving

    // Cut thruster to zero
    thrustPtr->setActuatorState(0, ThrustForces::ThrusterCommand{0.0, 0.0, 0.8});

    // Step 5 more seconds — hull drag must reduce velocity
    for (int i = 0; i < 50; ++i)
        world.step(0.1);

    const double uAfterCoast = domain->state(0).u();
    REQUIRE(uAfterCoast < uAfterThrust);  // vessel must have slowed down
    REQUIRE(uAfterCoast >= 0.0);          // must not reverse or blow up
}

// ---------------------------------------------------------------------------
// Test: max thrust + max rudder stays stable (regression for WASM integrator crash)
// ---------------------------------------------------------------------------

static std::unique_ptr<ymir::RigidBody6DOF> makeVlccBody(int id)
{
    // Full VLCC mass + added mass (diagonal only, sufficient for stability test)
    ymir::Matrix6x6 M{};
    M[0][0] = 450150.0;    M[1][1] = 450150.0;    M[2][2] = 450150.0;
    M[3][3] = 232980000.0; M[4][4] = 3505800000.0; M[5][5] = 3738800000.0;

    ymir::Matrix6x6 A{};
    A[0][0] = 53272.7;    A[1][1] = 223849.0;    A[2][2] = 3618741.0;
    A[3][3] = 67394070.0; A[4][4] = 9546841000.0; A[5][5] = 2809140000.0;

    ymir::Vector6 q{}, qdot{};
    return std::make_unique<ymir::RigidBody6DOF>(id, M, A, q, qdot, defaultCfg());
}

static InertialConfig vlccInertialCfg()
{
    InertialConfig cfg{};
    ymir::Matrix6x6 M{};
    M[0][0]=450150.0;  M[1][1]=450150.0;  M[2][2]=450150.0;
    M[3][3]=232980000.0; M[4][4]=3505800000.0; M[5][5]=3738800000.0;
    ymir::Matrix6x6 A{};
    A[0][0]=53272.7; A[1][1]=223849.0; A[2][2]=3618741.0;
    A[3][3]=67394070.0; A[4][4]=9546841000.0; A[5][5]=2809140000.0;
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            cfg.totalMass[i][j] = M[i][j] + A[i][j];
    cfg.addedMass = A;
    cfg.cg   = {0.691, 0.0, 19.2};
    cfg.mass = 450150.0;
    return cfg;
}

static DampingForces::Config vlccDampingCfg()
{
    DampingForces::Config cfg{};
    cfg.linearDampingCoeff = 0.5;
    // Full potential damping matrix from vessel1.json (WAMIT)
    cfg.potential[0][0] = 1.96952;    cfg.potential[0][2] = -2.76792;   cfg.potential[0][4] = 823.503;
    cfg.potential[1][1] = 8.02247;    cfg.potential[1][3] = -51.0285;   cfg.potential[1][5] = 1.80281;
    cfg.potential[2][0] = -1.47476;   cfg.potential[2][2] = 201088.9;   cfg.potential[2][4] = 2779191.0;
    cfg.potential[3][1] = -50.7984;   cfg.potential[3][3] = 3291989.0;  cfg.potential[3][5] = -11.0002;
    cfg.potential[4][0] = 836.414;    cfg.potential[4][2] = 2779092.0;  cfg.potential[4][4] = 1393379000.0;
    cfg.potential[5][1] = 2.05521;    cfg.potential[5][3] = -12.6653;   cfg.potential[5][5] = 20.7451;
    cfg.linear[0][0]    = 3.0;
    cfg.linear[3][3]    = 12854400.0;
    cfg.quadratic[5][5] = 16446722225.0;
    return cfg;
}

static RestoringConfig vlccRestoringCfg()
{
    RestoringConfig cfg{};
    cfg.hydro_rest[2][2] = 213018.0;
    cfg.hydro_rest[3][3] = 36249900.0;
    cfg.hydro_rest[4][4] = 1952870000.0;
    cfg.draft            = 23.0;
    cfg.mass             = 450150.0;
    cfg.volumetricWeight = 4355971.5;
    cfg.cg               = {0.691, 0.0, 19.2};
    cfg.cf               = {0.6909, 0.0, -7.09715};
    return cfg;
}

static CurrentForces::Config vlccCurrentCfg()
{
    CurrentForces::Config c{};
    c.model         = ymir::naval::CurrentModel::OBOKATA;
    c.length_BP     = 350.0;
    c.beam          = 63.0;
    c.frontalHeight = 11.5;
    c.lateralHeight = 11.5;
    c.n_sections    = 50;
    c.angles = {  0, 10, 20, 30, 40, 50, 60, 70, 80, 90,
                100,110,120,130,140,150,160,170,180,190,
                200,210,220,230,240,250,260,270,280,290,
                300,310,320,330,340,350,360};
    c.cdx = { 0.0100, 0.0069,-0.0016,-0.0130,-0.0239,-0.0312,-0.0324,-0.0267,-0.0151, 0.0000,
              0.0151, 0.0267, 0.0324, 0.0312, 0.0239, 0.0130, 0.0016,-0.0069,-0.0100,-0.0069,
              0.0016, 0.0130, 0.0239, 0.0312, 0.0324, 0.0267, 0.0151, 0.0000,-0.0151,-0.0267,
             -0.0324,-0.0312,-0.0239,-0.0130,-0.0016, 0.0069, 0.0100};
    c.cdy = { 0.0000, 0.1223, 0.3055, 0.5298, 0.7697, 0.9966, 1.1826, 1.3041, 1.3452, 1.3000,
              1.3452, 1.3041, 1.1826, 0.9966, 0.7697, 0.5298, 0.3055, 0.1223, 0.0000,-0.1223,
             -0.3055,-0.5298,-0.7697,-0.9966,-1.1826,-1.3041,-1.3452,-1.3000,-1.3452,-1.3041,
             -1.1826,-0.9966,-0.7697,-0.5298,-0.3055,-0.1223, 0.0000};
    return c;
}

TEST_CASE("Actuator integration: VLCC max thrust + max rudder stays stable (no integrator crash)",
          "[actuator][integration][vlcc]")
{
    ymir::World world;
    auto domainOwned = std::make_unique<ymir::NavalDomain>("naval");
    ymir::NavalDomain* domain = domainOwned.get();
    world.addDomain(std::move(domainOwned));

    domain->addBody(0, makeVlccBody(0));
    domain->addNavalForceModel(0, std::make_unique<InertialForces>(vlccInertialCfg()));
    domain->addNavalForceModel(0, std::make_unique<DampingForces>(vlccDampingCfg()));
    domain->addNavalForceModel(0, std::make_unique<RestoringForces>(vlccRestoringCfg()));
    domain->addNavalForceModel(0, std::make_unique<CurrentForces>(vlccCurrentCfg()));

    // Thruster: VLCC propeller
    ThrustForces::Config tCfg{};
    ThrustForces::ThrusterConfig tc{};
    tc.position = {-170.0, 0.0, 4.5}; tc.diameter = 9.2;
    tc.pitchRatio = 0.6145; tc.nominalRPM = 81.0; tc.azimuth_deg = 0.0; tc.initialRPM = 0.0;
    tCfg.thrusters.push_back(tc);
    auto thrust = std::make_unique<ThrustForces>(tCfg);
    ThrustForces* thrustPtr = thrust.get();
    domain->addNavalForceModel(0, std::move(thrust));

    // Rudder: VLCC balanced rudder
    RudderForces::Config rCfg{};
    RudderForces::RudderConfig rc{};
    rc.position = {-171.0, 0.0, 6.0}; rc.area = 60.0; rc.thrusterIdx = 0; rc.coefficients = foilTable();
    rCfg.rudders.push_back(rc);
    auto rudder = std::make_unique<RudderForces>(rCfg, thrustPtr);
    RudderForces* rudderPtr = rudder.get();
    domain->addNavalForceModel(0, std::move(rudder));

    domain->initialize();

    // MAX thrust (81 RPM) + MAX rudder (35°)
    constexpr double kMaxRPM    = 81.0;
    constexpr double kMaxRudder = 35.0 * M_PI / 180.0;
    thrustPtr->setActuatorState(0, ThrustForces::ThrusterCommand{kMaxRPM, 0.0, 0.6145});
    rudderPtr->setActuatorState(0, RudderForces::RudderCommand{kMaxRudder});

    // Step 30 s — must NOT throw; velocities must remain finite
    REQUIRE_NOTHROW([&](){
        for (int i = 0; i < 600; ++i)
            world.step(0.05);
    }());

    auto st = domain->state(0);
    REQUIRE(std::isfinite(st.u()));
    REQUIRE(std::isfinite(st.v()));
    REQUIRE(std::isfinite(st.r()));
    REQUIRE(st.u() > 0.0);   // vessel moving forward
}
