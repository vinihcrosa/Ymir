#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <ymir/simulation/NavalSimulation.h>
#pragma clang diagnostic pop
#include <ymir/physics/forces/DampingForces.h>
#include <ymir/physics/forces/RestoringForces.h>
#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/common/Types.h>

#include <memory>

using Catch::Approx;
using namespace ymir::naval;

static std::unique_ptr<ymir::RigidBody6DOF> makeTestBody(
    double m = 1e6, double u0 = 0.0, const ymir::CvodeConfig& cfg = {})
{
    ymir::Matrix6x6 mass{};
    for (int i = 0; i < 6; ++i) mass[i][i] = m;
    ymir::Matrix6x6 added{};
    ymir::Vector6   q{};
    ymir::Vector6   qdot{};
    qdot[0] = u0;
    return std::make_unique<ymir::RigidBody6DOF>(0, mass, added, q, qdot, cfg);
}

TEST_CASE("NavalSimulation free drift: vessel stays near origin without forces")
{
    ymir::CvodeConfig cfg{};
    cfg.reltol = 1e-6;
    cfg.abstol = 1e-9;

    NavalSimulation sim;
    sim.addBody(0, makeTestBody(1e6, 0.0, cfg));
    sim.initialize();

    for (int i = 0; i < 10; ++i)
        sim.step(0.5);

    auto st = sim.state(0);
    for (int i = 0; i < 6; ++i)
    {
        REQUIRE(std::abs(st.q()[i])    < 1e-8);
        REQUIRE(std::abs(st.qdot()[i]) < 1e-8);
    }
}

TEST_CASE("NavalSimulation damping force model reduces velocity")
{
    ymir::CvodeConfig cfg{};
    cfg.reltol = 1e-6;
    cfg.abstol = 1e-9;

    NavalSimulation sim;
    sim.addBody(0, makeTestBody(1e6, 5.0, cfg));

    DampingForces::Config dcfg{};
    dcfg.linear[0][0]       = 1e5;
    dcfg.linearDampingCoeff = 0.0;

    sim.addNavalForceModel(0, std::make_unique<DampingForces>(dcfg));
    sim.initialize();

    for (int i = 0; i < 20; ++i)
        sim.step(0.5);

    auto st = sim.state(0);
    REQUIRE(st.u() < 5.0);
}
