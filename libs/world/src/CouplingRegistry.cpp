#include <ymir/world/CouplingRegistry.h>

#include <cassert>

namespace ymir {

void CouplingRegistry::addLink(int producerBodyId, int consumerBodyId) {
    for ([[maybe_unused]] const auto& l : links_)
        assert(!(l.producerBodyId == producerBodyId && l.consumerBodyId == consumerBodyId));
    links_.push_back({producerBodyId, consumerBodyId, Forces::zero(), false});
    resolved_.emplace(consumerBodyId, Forces::zero());
}

void CouplingRegistry::writeForce(int producerBodyId, const Forces& f) {
    for (auto& l : links_) {
        if (l.producerBodyId == producerBodyId) {
            l.force = f;
            l.ready = true;
            return;
        }
    }
    assert(false); // producerBodyId not registered — addLink must be called first
}

void CouplingRegistry::resolve() {
    // First pass: reset consumer slots for consumers with at least one ready producer
    for (const auto& l : links_) {
        if (l.ready)
            resolved_.at(l.consumerBodyId) = Forces::zero();
    }
    // Second pass: accumulate ready forces into consumer slots
    for (const auto& l : links_) {
        if (l.ready)
            resolved_.at(l.consumerBodyId) += l.force;
    }
}

Forces CouplingRegistry::consumedForce(int consumerBodyId) const {
    const auto it = resolved_.find(consumerBodyId);
    if (it == resolved_.end())
        return Forces::zero();
    return it->second;
}

void CouplingRegistry::reset() {
    for (auto& l : links_)
        l.ready = false;
}

} // namespace ymir
