#pragma once

#include <map>
#include <string>
#include <vector>
#include <ymir/adapters/json/BodyDefinition.h>
#include <ymir/core/Types.h>

namespace ymir::adapters {

struct BodyInstanceConfig
{
    int         id;
    Vector6     position{};
    Vector6     velocity{};
    Vector6     acceleration{};
    std::string bodyType;
};

struct Scenario
{
    std::map<std::string, BodyDefinition> bodyTypes;
    std::vector<BodyInstanceConfig>       bodies;
};

} // namespace ymir::adapters
