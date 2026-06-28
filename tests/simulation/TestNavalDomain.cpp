#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/simulation/NavalDomain.h>
#include <ymir/simulation/CouplingForceModel.h>
#include <ymir/world/Environment.h>
#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/common/Types.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <ymir/simulation/NavalSimulation.h>
#pragma clang diagnostic pop

#include <cmath>
#include <memory>

using Catch::Approx;

static std::unique_ptr<ymir::RigidBody6DOF> makeBody(
    int id = 0, double m = 1e6,
    ymir::Vector6 q = {}, ymir::Vector6 qdot = {},
    const ymir::CvodeConfig& cfg = {})
{
    ymir::Matrix6x6 mass{};
    for (int i = 0; i < 6; ++i) mass[i][i] = m;
    ymir::Matrix6x6 added{};
    return std::make_unique<ymir::RigidBody6DOF>(id, mass, added, q, qdot, cfg);
}

static ymir::CvodeConfig defaultCfg()
{
    ymir::CvodeConfig cfg{};
    cfg.reltol = 1e-6;
    cfg.abstol = 1e-9;
    return cfg;
}

// -----------------------------------------------------------------------
// name()
// -----------------------------------------------------------------------

TEST_CASE("NavalDomain: name() returns string passed to constructor")
{
    ymir::NavalDomain d("my_domain");
    REQUIRE(d.name() == "my_domain");
}

TEST_CASE("NavalDomain: name() default is 'naval'")
{
    ymir::NavalDomain d;
    REQUIRE(d.name() == "naval");
}

// -----------------------------------------------------------------------
// onAddedToWorld / step without crash
// -----------------------------------------------------------------------

TEST_CASE("NavalDomain: step(0.1) completes without crash after onAddedToWorld (zero-force body)")
{
    ymir::Environment env;
    ymir::CouplingRegistry coupling;

    ymir::NavalDomain domain;
    domain.addBody(0, makeBody(0, 1e6, {}, {}, defaultCfg()));
    domain.onAddedToWorld(env, coupling);
    domain.initialize();
    domain.step(0.1);

    REQUIRE(domain.time() == Approx(0.1).margin(1e-10));
}

// -----------------------------------------------------------------------
// allBodyPositions
// -----------------------------------------------------------------------

TEST_CASE("NavalDomain: allBodyPositions returns 2 entries for 2 bodies")
{
    ymir::Environment env;
    ymir::CouplingRegistry coupling;

    ymir::NavalDomain domain;
    domain.addBody(0, makeBody(0, 1e6, {}, {}, defaultCfg()));
    domain.addBody(1, makeBody(1, 1e6, {}, {}, defaultCfg()));
    domain.onAddedToWorld(env, coupling);
    domain.initialize();

    auto positions = domain.allBodyPositions();
    REQUIRE(positions.size() == 2);
}

TEST_CASE("NavalDomain: allBodyPositions ids match registered body ids")
{
    ymir::Environment env;
    ymir::CouplingRegistry coupling;

    ymir::NavalDomain domain;
    domain.addBody(3, makeBody(3, 1e6, {}, {}, defaultCfg()));
    domain.addBody(7, makeBody(7, 1e6, {}, {}, defaultCfg()));
    domain.onAddedToWorld(env, coupling);
    domain.initialize();

    auto positions = domain.allBodyPositions();
    REQUIRE(positions.size() == 2);

    bool found3 = false, found7 = false;
    for (const auto& bp : positions)
    {
        if (bp.id == 3) found3 = true;
        if (bp.id == 7) found7 = true;
    }
    REQUIRE(found3);
    REQUIRE(found7);
}

TEST_CASE("NavalDomain: allBodyPositions xyz match state(id).q()[0..2]")
{
    ymir::Environment env;
    ymir::CouplingRegistry coupling;

    ymir::Vector6 q0{}; q0[0] = 10.0; q0[1] = 20.0; q0[2] = -5.0;
    ymir::Vector6 q1{}; q1[0] = -3.0; q1[1] =  7.0; q1[2] =  2.0;

    ymir::NavalDomain domain;
    domain.addBody(0, makeBody(0, 1e6, q0, {}, defaultCfg()));
    domain.addBody(1, makeBody(1, 1e6, q1, {}, defaultCfg()));
    domain.onAddedToWorld(env, coupling);
    domain.initialize();

    auto positions = domain.allBodyPositions();

    for (const auto& bp : positions)
    {
        auto st = domain.state(bp.id);
        REQUIRE(bp.x == Approx(st.q()[0]));
        REQUIRE(bp.y == Approx(st.q()[1]));
        REQUIRE(bp.z == Approx(st.q()[2]));
    }
}

// -----------------------------------------------------------------------
// distanceBetween
// -----------------------------------------------------------------------

TEST_CASE("NavalDomain: distanceBetween returns correct Euclidean distance")
{
    ymir::Environment env;
    ymir::CouplingRegistry coupling;

    ymir::Vector6 q0{}; q0[0] = 0.0; q0[1] = 0.0; q0[2] = 0.0;
    ymir::Vector6 q1{}; q1[0] = 3.0; q1[1] = 4.0; q1[2] = 0.0;

    ymir::NavalDomain domain;
    domain.addBody(0, makeBody(0, 1e6, q0, {}, defaultCfg()));
    domain.addBody(1, makeBody(1, 1e6, q1, {}, defaultCfg()));
    domain.onAddedToWorld(env, coupling);
    domain.initialize();

    // Pythagorean triple: 3-4-5
    REQUIRE(domain.distanceBetween(0, 1) == Approx(5.0).margin(1e-10));
}

TEST_CASE("NavalDomain: distanceBetween 3D distance matches expected")
{
    ymir::Environment env;
    ymir::CouplingRegistry coupling;

    ymir::Vector6 q0{}; q0[0] = 1.0; q0[1] = 2.0; q0[2] = 3.0;
    ymir::Vector6 q1{}; q1[0] = 4.0; q1[1] = 6.0; q1[2] = 3.0;

    ymir::NavalDomain domain;
    domain.addBody(0, makeBody(0, 1e6, q0, {}, defaultCfg()));
    domain.addBody(1, makeBody(1, 1e6, q1, {}, defaultCfg()));
    domain.onAddedToWorld(env, coupling);
    domain.initialize();

    // distance = sqrt(9 + 16 + 0) = 5
    REQUIRE(domain.distanceBetween(0, 1) == Approx(5.0).margin(1e-10));
}

// -----------------------------------------------------------------------
// bodyState / state
// -----------------------------------------------------------------------

TEST_CASE("NavalDomain: bodyState matches state for registered body")
{
    ymir::Environment env;
    ymir::CouplingRegistry coupling;

    ymir::NavalDomain domain;
    domain.addBody(0, makeBody(0, 1e6, {}, {}, defaultCfg()));
    domain.onAddedToWorld(env, coupling);
    domain.initialize();

    auto s1 = domain.state(0);
    auto s2 = domain.bodyState(0);

    for (int i = 0; i < 6; ++i)
    {
        REQUIRE(s1.q()[i] == Approx(s2.q()[i]));
        REQUIRE(s1.qdot()[i] == Approx(s2.qdot()[i]));
    }
}

TEST_CASE("NavalDomain: state throws for unknown bodyId")
{
    ymir::NavalDomain domain;
    REQUIRE_THROWS_AS(domain.state(999), std::out_of_range);
}

// -----------------------------------------------------------------------
// Coupling force injection
// -----------------------------------------------------------------------

TEST_CASE("NavalDomain: body with CouplingForceModel receives resolved force after step")
{
    ymir::Environment env;
    ymir::CouplingRegistry coupling;
    coupling.addLink(1, 0);  // producer=1, consumer=0

    const ymir::Forces injected = []() {
        ymir::Forces f = ymir::Forces::zero();
        f.f[0] = 500.0;  // surge force
        return f;
    }();

    coupling.writeForce(1, injected);
    coupling.resolve();

    ymir::NavalDomain domain;
    domain.addBody(0, makeBody(0, 1e6, {}, {}, defaultCfg()));
    domain.addNavalForceModel(0, std::make_unique<ymir::CouplingForceModel>(0, coupling));
    domain.onAddedToWorld(env, coupling);
    domain.initialize();
    domain.step(0.1);

    // Body 0 received a surge force of 500 N, mass = 1e6 kg -> tiny but nonzero acceleration
    // After step(0.1), surge velocity should be slightly above 0
    auto st = domain.state(0);
    REQUIRE(st.u() > 0.0);
}

// -----------------------------------------------------------------------
// Regression: NavalDomain vs NavalSimulation
// -----------------------------------------------------------------------

TEST_CASE("NavalDomain regression: matches NavalSimulation within 1e-8 over 1000 steps")
{
    const int    STEPS = 1000;
    const double DT    = 0.1;

    ymir::CvodeConfig cfg = defaultCfg();

    // --- NavalSimulation setup ---
    ymir::naval::NavalSimulation sim;
    sim.addBody(0, makeBody(0, 1e6, {}, {}, cfg));
    sim.initialize();

    // --- NavalDomain setup ---
    ymir::Environment env;         // default: zero wind/current, waterDepth=100
    ymir::CouplingRegistry coupling;

    ymir::NavalDomain domain;
    domain.addBody(0, makeBody(0, 1e6, {}, {}, cfg));
    domain.onAddedToWorld(env, coupling);
    domain.initialize();

    for (int i = 0; i < STEPS; ++i)
    {
        sim.step(DT);
        domain.step(DT);

        auto qs  = sim.state(0).q();
        auto qd  = domain.state(0).q();
        auto qds = sim.state(0).qdot();
        auto qdd = domain.state(0).qdot();

        for (int dof = 0; dof < 6; ++dof)
        {
            REQUIRE(std::abs(qd[dof]  - qs[dof])  < 1e-8);
            REQUIRE(std::abs(qdd[dof] - qds[dof]) < 1e-8);
        }
    }
}

// ---------------------------------------------------------------------------
// serializeStateJson
// ---------------------------------------------------------------------------

TEST_CASE("NavalDomain: serializeStateJson produces valid JSON with expected keys", "[naval_domain]")
{
    ymir::Environment      env;
    ymir::CouplingRegistry coupling;
    ymir::NavalDomain      domain;

    ymir::Vector6 q0{}, qd0{};
    q0[0] = 5.0; q0[5] = 0.1;
    domain.addBody(0, makeBody(0, 1e6, q0, qd0));
    domain.onAddedToWorld(env, coupling);
    domain.initialize();
    domain.step(0.1);

    const std::string json = domain.serializeStateJson();

    REQUIRE(json.find("\"t\":") != std::string::npos);
    REQUIRE(json.find("\"vessels\":") != std::string::npos);
    REQUIRE(json.find("\"id\":0") != std::string::npos);
    REQUIRE(json.find("\"x\":") != std::string::npos);
    REQUIRE(json.find("\"psi\":") != std::string::npos);
    REQUIRE(json.find("\"u\":") != std::string::npos);
    REQUIRE(json.find("\"r\":") != std::string::npos);
    REQUIRE(json.front() == '{');
    REQUIRE(json.back()  == '}');
}

TEST_CASE("NavalDomain: serializeStateJson t advances with steps", "[naval_domain]")
{
    ymir::Environment      env;
    ymir::CouplingRegistry coupling;
    ymir::NavalDomain      domain;

    domain.addBody(0, makeBody(0));
    domain.onAddedToWorld(env, coupling);
    domain.initialize();
    domain.step(0.5);
    domain.step(0.5);

    const std::string json = domain.serializeStateJson();
    const std::size_t tpos = json.find("\"t\":") + 4;
    const double      t    = std::stod(json.substr(tpos));
    REQUIRE(t == Approx(1.0).margin(1e-9));
}
