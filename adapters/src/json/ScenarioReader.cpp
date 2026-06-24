#include <ymir/adapters/json/ScenarioReader.h>
#include <ymir/core/RigidBody6DOF.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

namespace ymir::adapters {

// ---------------------------------------------------------------------------
// helpers

static Vector6 parseVector6(const json& arr, const std::string& context)
{
    if (!arr.is_array() || arr.size() != 6)
        throw std::runtime_error("Expected 6-element array for " + context);
    Vector6 v{};
    for (int i = 0; i < 6; ++i)
        v[i] = arr[i].get<double>();
    return v;
}

static Matrix6x6 parseMatrix6x6(const json& arr, const std::string& context)
{
    if (!arr.is_array() || arr.size() != 6)
        throw std::runtime_error("Expected 6x6 array for " + context);
    Matrix6x6 m{};
    for (int i = 0; i < 6; ++i)
    {
        if (!arr[i].is_array() || arr[i].size() != 6)
            throw std::runtime_error("Row " + std::to_string(i) + " of " + context + " must have 6 elements");
        for (int j = 0; j < 6; ++j)
            m[i][j] = arr[i][j].get<double>();
    }
    return m;
}

static BodyDefinition parseBodyDefinition(const std::string& name, const json& j)
{
    BodyDefinition def;
    def.name       = name;
    def.massMatrix = parseMatrix6x6(j.at("massMatrix"), name + ".massMatrix");
    def.addedMass  = parseMatrix6x6(j.at("addedMass"),  name + ".addedMass");
    return def;
}

static BodyDefinition loadBodyDefinitionFile(const std::filesystem::path& path)
{
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Cannot open body definition file: " + path.string());
    json j = json::parse(f);
    std::string name = j.value("name", path.stem().string());
    return parseBodyDefinition(name, j);
}

// ---------------------------------------------------------------------------

Scenario ScenarioReader::parse(const std::filesystem::path& scenarioPath,
                                const std::filesystem::path& bodyTypesDir)
{
    std::ifstream f(scenarioPath);
    if (!f.is_open())
        throw std::runtime_error("Cannot open scenario file: " + scenarioPath.string());

    json root = json::parse(f);

    Scenario scenario;

    // inline body type definitions
    if (root.contains("bodyTypes"))
    {
        for (auto& [name, jDef] : root["bodyTypes"].items())
            scenario.bodyTypes[name] = parseBodyDefinition(name, jDef);
    }

    // body instances
    for (auto& [key, jBody] : root.at("bodies").items())
    {
        BodyInstanceConfig inst;
        inst.id           = std::stoi(key);
        inst.position     = parseVector6(jBody.at("position"),     "bodies." + key + ".position");
        inst.velocity     = parseVector6(jBody.at("velocity"),     "bodies." + key + ".velocity");
        inst.acceleration = parseVector6(jBody.at("acceleration"), "bodies." + key + ".acceleration");
        inst.bodyType     = jBody.at("body").get<std::string>();

        // resolve body type from file if not already in inline map
        if (scenario.bodyTypes.find(inst.bodyType) == scenario.bodyTypes.end())
        {
            if (bodyTypesDir.empty())
                throw std::runtime_error(
                    "Body type '" + inst.bodyType + "' not found inline and no bodyTypesDir provided");

            std::filesystem::path defPath = bodyTypesDir / (inst.bodyType + ".json");
            scenario.bodyTypes[inst.bodyType] = loadBodyDefinitionFile(defPath);
        }

        scenario.bodies.push_back(std::move(inst));
    }

    return scenario;
}

Simulation ScenarioReader::buildSimulation(const Scenario& scenario)
{
    Simulation sim;

    for (const auto& inst : scenario.bodies)
    {
        auto it = scenario.bodyTypes.find(inst.bodyType);
        if (it == scenario.bodyTypes.end())
            throw std::runtime_error("Body type '" + inst.bodyType + "' missing from scenario");

        const BodyDefinition& def = it->second;

        sim.addBody(inst.id,
                    std::make_unique<RigidBody6DOF>(inst.id,
                                                    def.massMatrix,
                                                    def.addedMass,
                                                    inst.position,
                                                    inst.velocity));
    }

    return sim;
}

Simulation ScenarioReader::load(const std::filesystem::path& scenarioPath,
                                 const std::filesystem::path& bodyTypesDir)
{
    return buildSimulation(parse(scenarioPath, bodyTypesDir));
}

} // namespace ymir::adapters
