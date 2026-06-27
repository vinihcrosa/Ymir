#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/world/Environment.h>
#include <ymir/world/NavalEnvironment.h>

using Catch::Approx;

TEST_CASE("Environment default construction")
{
    ymir::Environment env{};
    REQUIRE(env.waterDepth()           == Approx(100.0));
    REQUIRE(env.tide()                 == Approx(0.0));
    REQUIRE(env.windSpeed()            == Approx(0.0));
    REQUIRE(env.windDirectionNaut()    == Approx(0.0));
    REQUIRE(env.currentSpeed()         == Approx(0.0));
    REQUIRE(env.currentDirectionNaut() == Approx(0.0));
}

TEST_CASE("Environment setWind round-trips via getters")
{
    ymir::Environment env{};
    env.setWind(5.0, 270.0);
    REQUIRE(env.windSpeed()         == Approx(5.0));
    REQUIRE(env.windDirectionNaut() == Approx(270.0));
}

TEST_CASE("Environment setCurrent round-trips via getters")
{
    ymir::Environment env{};
    env.setCurrent(2.0, 90.0);
    REQUIRE(env.currentSpeed()         == Approx(2.0));
    REQUIRE(env.currentDirectionNaut() == Approx(90.0));
}

TEST_CASE("Environment setTide round-trips via getter")
{
    ymir::Environment env{};
    env.setTide(1.5);
    REQUIRE(env.tide() == Approx(1.5));
}

TEST_CASE("Environment setWaterDepth round-trips via getter")
{
    ymir::Environment env{};
    env.setWaterDepth(50.0);
    REQUIRE(env.waterDepth() == Approx(50.0));
}

TEST_CASE("Environment zero wind speed is valid")
{
    ymir::Environment env{};
    env.setWind(0.0, 0.0);
    REQUIRE(env.windSpeed() == Approx(0.0));
}

TEST_CASE("Environment zero current speed is valid")
{
    ymir::Environment env{};
    env.setCurrent(0.0, 0.0);
    REQUIRE(env.currentSpeed() == Approx(0.0));
}

TEST_CASE("Environment setWind does not affect current or depth")
{
    ymir::Environment env{};
    env.setWind(3.0, 45.0);
    REQUIRE(env.currentSpeed() == Approx(0.0));
    REQUIRE(env.waterDepth()   == Approx(100.0));
}

TEST_CASE("NavalEnvironment alias: setters and getters compile and work")
{
    // Verify the deprecated type alias is still functional (removed in Phase 3).
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    ymir::naval::NavalEnvironment env{};
    env.setWind(2.5, 180.0);
    REQUIRE(env.windSpeed()         == Approx(2.5));
    REQUIRE(env.windDirectionNaut() == Approx(180.0));
    REQUIRE(env.waterDepth()        == Approx(100.0));
#pragma clang diagnostic pop
}

TEST_CASE("Environment setSeaState stores without assertion")
{
    ymir::Environment env{};
    // setSeaState must not assert or throw for valid inputs
    env.setSeaState(2.0, 8.0, 315.0);
    // no public getters for sea state in Phase 2 — verify no crash
    REQUIRE(env.waterDepth() == Approx(100.0));  // unrelated field unchanged
}
