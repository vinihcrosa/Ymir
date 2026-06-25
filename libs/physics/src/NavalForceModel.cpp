#include <ymir/physics/NavalForceModel.h>

#include <cassert>

namespace ymir::naval
{

Forces NavalForceModel::compute(const BodyState& state)
{
    assert(ctx_ != nullptr && "NavalForceModel: context not bound — call bindContext() before step");
    return computeNaval(state, *ctx_);
}

} // namespace ymir::naval
