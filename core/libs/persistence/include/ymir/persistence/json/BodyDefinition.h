#pragma once

#include <string>
#include <ymir/common/Types.h>

namespace ymir::persistence {

struct BodyDefinition
{
    std::string name;
    Matrix6x6   massMatrix{};
    Matrix6x6   addedMass{};
};

} // namespace ymir::persistence
