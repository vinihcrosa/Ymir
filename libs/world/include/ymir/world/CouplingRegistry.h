#pragma once

#include <map>
#include <vector>

#include <ymir/physics/Forces.h>

namespace ymir {

/** Mediates weak Jacobi force coupling between simulation bodies.
 *  Links are pre-allocated via addLink(); no heap allocation occurs per tick. */
class CouplingRegistry {
public:
    /** Register a (producer, consumer) force link.
     *  Must be called before the first step.
     *  Asserts in debug if the pair (producerBodyId, consumerBodyId) already exists. */
    void addLink(int producerBodyId, int consumerBodyId);

    /** Record a force written by a producer during domain.step().
     *  Asserts in debug if producerBodyId has no registered link. */
    void writeForce(int producerBodyId, const Forces& f);

    /** Flush all ready producer writes into consumer resolved slots.
     *  Called by World::step() after all domain steps complete. */
    void resolve();

    /** Return the force last resolved for consumerBodyId.
     *  Returns Forces::zero() when no link exists for consumerBodyId. */
    [[nodiscard]] Forces consumedForce(int consumerBodyId) const;

    /** Clear ready flags on all links without clearing resolved force values.
     *  Called by World::step() at the start of each tick. */
    void reset();

private:
    struct Link {
        int    producerBodyId;
        int    consumerBodyId;
        Forces force = Forces::zero();
        bool   ready = false;
    };

    std::vector<Link>     links_;
    std::map<int, Forces> resolved_;
};

} // namespace ymir
