#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/simulation/Simulation.h>

#include <memory>

using Catch::Matchers::WithinAbs;
using namespace ymir;

static std::unique_ptr<RigidBody6DOF> makeRestingBody(int id, double mass = 1000.0)
{
    Matrix6x6 M{};
    for (int i = 0; i < 6; ++i) M[i][i] = mass;
    Matrix6x6 A{};
    Vector6   q{}, qdot{};
    return std::make_unique<RigidBody6DOF>(id, M, A, q, qdot);
}

// ---------------------------------------------------------------------------

TEST_CASE("Simulation: single body at rest with no forces stays at rest", "[integration]")
{
    Simulation sim;
    sim.addBody(0, makeRestingBody(0));

    for (int i = 0; i < 100; ++i)
        sim.step(0.1);

    BodyState s = sim.state(0);
    for (int i = 0; i < 6; ++i)
    {
        REQUIRE_THAT(s.q()[i],    WithinAbs(0.0, 1e-10));
        REQUIRE_THAT(s.qdot()[i], WithinAbs(0.0, 1e-10));
    }
}

TEST_CASE("Simulation: multiple bodies at rest with no forces all stay at rest", "[integration]")
{
    Simulation sim;
    sim.addBody(0, makeRestingBody(0));
    sim.addBody(1, makeRestingBody(1));
    sim.addBody(2, makeRestingBody(2));

    for (int i = 0; i < 100; ++i)
        sim.step(0.1);

    for (int id = 0; id < 3; ++id)
    {
        BodyState s = sim.state(id);
        for (int i = 0; i < 6; ++i)
        {
            REQUIRE_THAT(s.q()[i],    WithinAbs(0.0, 1e-10));
            REQUIRE_THAT(s.qdot()[i], WithinAbs(0.0, 1e-10));
        }
    }
}
