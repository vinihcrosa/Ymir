#pragma once

#include <filesystem>
#include <ymir/persistence/json/Scenario.h>
#include <ymir/simulation/Simulation.h>

namespace ymir::persistence {

/**
 * Reads scenario JSON files into Scenario data + Simulation objects.
 *
 * Body type resolution order per instance:
 *   1. Inline "bodyTypes" section inside the scenario file
 *   2. {bodyTypesDir}/{name}.json (if bodyTypesDir provided)
 */
class ScenarioReader
{
public:
    /**
     * Parse scenario file. Returns Scenario with all body types resolved.
     * Throws std::runtime_error if body type cannot be found or JSON is invalid.
     */
    static Scenario parse(const std::filesystem::path& scenarioPath,
                          const std::filesystem::path& bodyTypesDir = {});

    /** Build Simulation from already-parsed Scenario. */
    static Simulation buildSimulation(const Scenario& scenario);

    /** Convenience: parse + buildSimulation. */
    static Simulation load(const std::filesystem::path& scenarioPath,
                           const std::filesystem::path& bodyTypesDir = {});
};

} // namespace ymir::persistence
