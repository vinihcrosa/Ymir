#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/naval/NavalContext.h>
#include <ymir/naval/NavalEnvironment.h>
#include <ymir/naval/NavalSimulation.h>
#include <ymir/naval/PhysicalConstants.h>
#include <ymir/core/Body.h>
#include <ymir/core/Types.h>

#include <memory>

using Catch::Approx;
using namespace ymir::naval;

static std::unique_ptr<ymir::Body> makeTestBody()
{
    ymir::Matrix6x6 mass{};
    for (int i = 0; i < 6; ++i) mass[i][i] = 1000.0;

    ymir::Matrix6x6 damp{};

    ymir::Vector6 q{};
    ymir::Vector6 qdot{};

    return std::make_unique<ymir::Body>(mass, damp, q, qdot);
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
    NavalEnvironment env{};
    REQUIRE(env.currentSpeed == Approx(0.0));
    REQUIRE(env.windSpeed == Approx(0.0));
    REQUIRE(env.waterDepth == Approx(100.0));
    REQUIRE(env.tide == Approx(0.0));
}

TEST_CASE("NavalSimulation constructs without crash")
{
    ymir::CvodeConfig cfg{};
    cfg.reltol = 1e-6;
    cfg.abstol = 1e-9;

    NavalSimulation sim(makeTestBody(), cfg);
    sim.initialize();
    REQUIRE(sim.time() == Approx(0.0));
}

TEST_CASE("NavalSimulation step advances time")
{
    ymir::CvodeConfig cfg{};
    cfg.reltol = 1e-6;
    cfg.abstol = 1e-9;

    NavalSimulation sim(makeTestBody(), cfg);
    sim.initialize();
    sim.step(0.1);
    REQUIRE(sim.time() == Approx(0.1).margin(1e-10));
}
