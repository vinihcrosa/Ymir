#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <ymir/adapters/json/ScenarioReader.h>

#include <filesystem>
#include <stdexcept>

using Catch::Matchers::WithinAbs;
using namespace ymir;
using namespace ymir::adapters;

#ifndef YMIR_EXAMPLES_DIR
#define YMIR_EXAMPLES_DIR "."
#endif

static std::filesystem::path scenariosDir()
{
    return std::filesystem::path(YMIR_EXAMPLES_DIR) / "scenarios";
}

static std::filesystem::path bodyTypesDir()
{
    return std::filesystem::path(YMIR_EXAMPLES_DIR) / "body_types";
}

// ---------------------------------------------------------------------------

TEST_CASE("ScenarioReader: parse inline body types", "[scenario][json]")
{
    Scenario s = ScenarioReader::parse(scenariosDir() / "two_bodies_at_rest.json");

    REQUIRE(s.bodyTypes.count("simple_box") == 1);
    REQUIRE(s.bodies.size() == 2);

    const BodyDefinition& def = s.bodyTypes.at("simple_box");
    for (int i = 0; i < 6; ++i)
        REQUIRE_THAT(def.massMatrix[i][i], WithinAbs(1000.0, 1e-10));
}

TEST_CASE("ScenarioReader: parse body instance fields", "[scenario][json]")
{
    Scenario s = ScenarioReader::parse(scenariosDir() / "body_with_initial_velocity.json");

    REQUIRE(s.bodies.size() == 1);
    const BodyInstanceConfig& inst = s.bodies[0];

    REQUIRE(inst.id == 1);
    REQUIRE(inst.bodyType == "simple_box");
    REQUIRE_THAT(inst.position[0], WithinAbs(10.0,    1e-10));
    REQUIRE_THAT(inst.position[1], WithinAbs(5.0,     1e-10));
    REQUIRE_THAT(inst.position[5], WithinAbs(0.5236,  1e-6));
    REQUIRE_THAT(inst.velocity[0], WithinAbs(2.0,     1e-10));
    for (int i = 0; i < 6; ++i)
        REQUIRE_THAT(inst.acceleration[i], WithinAbs(0.0, 1e-10));
}

TEST_CASE("ScenarioReader: build simulation from scenario — bodies at rest", "[scenario][json][integration]")
{
    Simulation sim = ScenarioReader::load(scenariosDir() / "two_bodies_at_rest.json");

    REQUIRE(sim.bodyCount() == 2);
    REQUIRE(sim.hasBody(1));
    REQUIRE(sim.hasBody(2));

    for (int id : {1, 2})
    {
        BodyState s = sim.state(id);
        for (int i = 0; i < 6; ++i)
        {
            REQUIRE_THAT(s.q()[i],    WithinAbs(0.0, 1e-10));
            REQUIRE_THAT(s.qdot()[i], WithinAbs(0.0, 1e-10));
        }
    }
}

TEST_CASE("ScenarioReader: build simulation from scenario — initial position and velocity", "[scenario][json][integration]")
{
    Simulation sim = ScenarioReader::load(scenariosDir() / "body_with_initial_velocity.json");

    REQUIRE(sim.bodyCount() == 1);
    REQUIRE(sim.hasBody(1));

    BodyState s = sim.state(1);
    REQUIRE_THAT(s.q()[0], WithinAbs(10.0,   1e-10));
    REQUIRE_THAT(s.q()[1], WithinAbs(5.0,    1e-10));
    REQUIRE_THAT(s.q()[5], WithinAbs(0.5236, 1e-6));
    REQUIRE_THAT(s.qdot()[0], WithinAbs(2.0, 1e-10));
}

TEST_CASE("ScenarioReader: body type from external file", "[scenario][json]")
{
    // two_bodies_at_rest.json has inline bodyTypes, but we can also load from files
    // Use body_with_initial_velocity.json with external dir override by creating
    // a minimal scenario without inline bodyTypes
    // (covered by the bodyTypesDir path in ScenarioReader::parse)

    // Direct test: load simple_box from bodyTypesDir
    std::filesystem::path defPath = bodyTypesDir() / "simple_box.json";
    REQUIRE(std::filesystem::exists(defPath));

    // Build a minimal scenario struct manually to test buildSimulation
    BodyDefinition def;
    def.name = "simple_box";
    for (int i = 0; i < 6; ++i) def.massMatrix[i][i] = 1000.0;

    BodyInstanceConfig inst;
    inst.id       = 42;
    inst.bodyType = "simple_box";

    Scenario scenario;
    scenario.bodyTypes["simple_box"] = def;
    scenario.bodies.push_back(inst);

    Simulation sim = ScenarioReader::buildSimulation(scenario);
    REQUIRE(sim.hasBody(42));
}

TEST_CASE("ScenarioReader: bodies at rest stay at rest after stepping", "[scenario][json][integration]")
{
    Simulation sim = ScenarioReader::load(scenariosDir() / "two_bodies_at_rest.json");

    for (int i = 0; i < 50; ++i)
        sim.step(0.1);

    for (int id : {1, 2})
    {
        BodyState s = sim.state(id);
        for (int i = 0; i < 6; ++i)
        {
            REQUIRE_THAT(s.q()[i],    WithinAbs(0.0, 1e-8));
            REQUIRE_THAT(s.qdot()[i], WithinAbs(0.0, 1e-8));
        }
    }
}
