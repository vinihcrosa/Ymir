#pragma once

#include <ymir/physics/BodyState.h>

#include <string>
#include <vector>

namespace ymir {

/** Snapshot of shared environmental conditions at a world time. */
struct EnvironmentSnapshot {
    double windSpeed_ms        = 0.0;
    double windDirection_deg   = 0.0;
    double currentSpeed_ms     = 0.0;
    double currentDirection_deg = 0.0;
    double waterDepth_m        = 100.0;
    double tide_m              = 0.0;
};

/** Snapshot of one body inside a domain. */
struct BodySnapshot {
    int       id = 0;
    BodyState state;
};

/** Snapshot of one registered domain and all of its bodies. */
struct DomainSnapshot {
    std::string               name;
    std::vector<BodySnapshot> bodies;
};

/** Complete nested snapshot of world state. */
struct WorldSnapshot {
    double                      simTime = 0.0;
    EnvironmentSnapshot         environment;
    std::vector<DomainSnapshot> domains;
};

} // namespace ymir
