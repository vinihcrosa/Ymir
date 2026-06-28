#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/vessel/NavalContext.h>
#include <ymir/world/Environment.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <ymir/simulation/NavalSimulation.h>
#pragma clang diagnostic pop
#include <ymir/common/PhysicalConstants.h>
#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/common/Types.h>

#include <memory>

using Catch::Approx;
using namespace ymir::naval;

static std::unique_ptr<ymir::RigidBody6DOF> makeTestBody(const ymir::CvodeConfig& cfg = {})
{
    ymir::Matrix6x6 mass{};
    for (int i = 0; i < 6; ++i) mass[i][i] = 1000.0;

    ymir::Matrix6x6 added{};
    ymir::Vector6   q{};
    ymir::Vector6   qdot{};

    return std::make_unique<ymir::RigidBody6DOF>(0, mass, added, q, qdot, cfg);
}

TEST_CASE("NavalContext default construction")
{
    NavalContext ctx{};
    REQUIRE(ctx.waterDepth == Approx(100.0));
    REQUIRE(ctx.tide == Approx(0.0));
    for (int i = 0; i < 6; ++i)
    {
        REQUIRE(ctx.speedToWater[i] == Approx(0.0));
        REQUIRE(ctx.speedToWind[i] == Approx(0.0));
        REQUIRE(ctx.q_avg[i] == Approx(0.0));
    }
}

TEST_CASE("PhysicalConstants values")
{
    REQUIRE(g == Approx(9.81));
    REQUIRE(rho_water == Approx(1025.0));
    REQUIRE(rho_air == Approx(1.225));
}

TEST_CASE("NavalEnvironment default values")
{
    ymir::Environment env{};
    REQUIRE(env.currentSpeed() == Approx(0.0));
    REQUIRE(env.windSpeed() == Approx(0.0));
    REQUIRE(env.waterDepth() == Approx(100.0));
    REQUIRE(env.tide() == Approx(0.0));
}

TEST_CASE("NavalSimulation constructs without crash")
{
    ymir::CvodeConfig cfg{};
    cfg.reltol = 1e-6;
    cfg.abstol = 1e-9;

    NavalSimulation sim;
    sim.addBody(0, makeTestBody(cfg));
    sim.initialize();
    REQUIRE(sim.time() == Approx(0.0));
}

TEST_CASE("NavalSimulation step advances time")
{
    ymir::CvodeConfig cfg{};
    cfg.reltol = 1e-6;
    cfg.abstol = 1e-9;

    NavalSimulation sim;
    sim.addBody(0, makeTestBody(cfg));
    sim.initialize();
    sim.step(0.1);
    REQUIRE(sim.time() == Approx(0.1).margin(1e-10));
}
