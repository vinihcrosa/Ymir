#pragma once

#include <ymir/world/Environment.h>
#include <ymir/physics/BodyState.h>

#include <string>
#include <vector>

namespace ymir {

// Forward declaration — full definition in libs/world/include/ymir/world/CouplingRegistry.h
class CouplingRegistry;

/** Plain spatial snapshot for a single body — used by World observers. */
struct BodyPosition {
    int    id;
    double x;
    double y;
    double z;
};

/**
 * Abstract interface for a physics domain registered in World.
 *
 * A domain owns a set of bodies and their dynamics. World calls step() each tick
 * and uses allBodyPositions() / bodyState() for observation. The interface is
 * domain-agnostic — no naval-specific types may appear here.
 *
 * Lifecycle:
 *   1. Construct the concrete domain.
 *   2. Register it via World::addDomain() — this calls onAddedToWorld() once.
 *   3. World::step(dt) calls step(dt) each tick.
 */
class IDomain {
public:
    IDomain()                          = default;
    virtual ~IDomain()                 = default;

    /**
     * Called once by World::addDomain() before the first step.
     *
     * The domain must cache the pointers it needs; env and coupling outlive
     * any registered domain.
     */
    virtual void onAddedToWorld(Environment& env, CouplingRegistry& coupling) = 0;

    /** Advance the domain by dt seconds. */
    virtual void step(double dt) = 0;

    /** Return the (x, y, z) position of every body currently in this domain. */
    virtual std::vector<BodyPosition> allBodyPositions() const = 0;

    /** Return the full kinematic state of the body identified by id. */
    virtual BodyState bodyState(int id) const = 0;

    /** Return the domain's name — used by World for lookup and snapshot labelling. */
    virtual std::string name() const = 0;

    IDomain(const IDomain&)            = delete;
    IDomain& operator=(const IDomain&) = delete;
};

} // namespace ymir
