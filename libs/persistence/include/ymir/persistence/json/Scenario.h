#pragma once

#include <map>
#include <string>
#include <vector>
#include <ymir/persistence/json/BodyDefinition.h>
#include <ymir/common/Types.h>

namespace ymir::persistence {

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

} // namespace ymir::persistence
