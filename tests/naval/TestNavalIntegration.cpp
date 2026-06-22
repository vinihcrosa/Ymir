#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/naval/NavalSimulation.h>
#include <ymir/naval/forces/DampingForces.h>
#include <ymir/naval/forces/RestoringForces.h>
#include <ymir/core/Body.h>
#include <ymir/core/Types.h>

#include <memory>

using Catch::Approx;
using namespace ymir::naval;

static std::unique_ptr<ymir::Body> makeTestBody(double m = 1e6)
{
    ymir::Matrix6x6 mass{};
    for (int i = 0; i < 6; ++i) mass[i][i] = m;
    ymir::Matrix6x6 damp{};
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    return std::make_unique<ymir::Body>(mass, damp, q, qdot);
}

TEST_CASE("NavalSimulation free drift: vessel stays near origin without forces")
{
    ymir::CvodeConfig cfg{};
    cfg.reltol = 1e-6;
    cfg.abstol = 1e-9;

    NavalSimulation sim(makeTestBody(), cfg);
    sim.initialize();

    for (int i = 0; i < 10; ++i)
        sim.step(0.5);

    auto state = sim.state();
    for (int i = 0; i < 6; ++i)
    {
        REQUIRE(std::abs(state.q()[i])    < 1e-8);
        REQUIRE(std::abs(state.qdot()[i]) < 1e-8);
    }
}

TEST_CASE("NavalSimulation damping force model reduces velocity")
{
    ymir::CvodeConfig cfg{};
    cfg.reltol = 1e-6;
    cfg.abstol = 1e-9;

    // Body with initial surge velocity
    ymir::Matrix6x6 mass{};
    for (int i = 0; i < 6; ++i) mass[i][i] = 1e6;
    ymir::Matrix6x6 damp{};
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    qdot[0] = 5.0;  // 5 m/s surge

    auto body = std::make_unique<ymir::Body>(mass, damp, q, qdot);

    NavalSimulation sim(std::move(body), cfg);

    DampingForces::Config dcfg{};
    dcfg.linear[0][0]       = 1e5;  // heavy linear damping
    dcfg.linearDampingCoeff = 0.0;  // disable exponential decay for predictable tau

    sim.addNavalForceModel(std::make_unique<DampingForces>(dcfg));
    sim.initialize();

    // Step 10 seconds
    for (int i = 0; i < 20; ++i)
        sim.step(0.5);

    auto state = sim.state();
    REQUIRE(state.u() < 5.0);  // velocity reduced
}
