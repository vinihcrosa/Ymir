#pragma once

#include <string>
#include <ymir/core/Types.h>

namespace ymir::adapters {

struct BodyDefinition
{
    std::string name;
    Matrix6x6   massMatrix{};
    Matrix6x6   addedMass{};
};

} // namespace ymir::adapters
