#pragma once

#include <string>

#include <ymir/physics/NavalForceModel.h>
#include <ymir/world/CouplingRegistry.h>
#include <ymir/vessel/NavalContext.h>

namespace ymir {

class CouplingForceModel final : public ymir::naval::NavalForceModel
{
public:
    CouplingForceModel(int consumerBodyId, const CouplingRegistry& registry)
        : consumerBodyId_(consumerBodyId), registry_(registry) {}

    std::string name() const override { return "CouplingForceModel"; }

protected:
    Forces computeNaval(const BodyState& /*state*/,
                        const ymir::naval::NavalContext& /*ctx*/) override
    {
        return registry_.consumedForce(consumerBodyId_);
    }

private:
    int                     consumerBodyId_;
    const CouplingRegistry& registry_;
};

} // namespace ymir
