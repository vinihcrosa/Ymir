#pragma once

#include <ymir/core/AbstractBody.h>
#include <ymir/core/BodyState.h>

#include <map>
#include <memory>
#include <vector>

namespace ymir
{

/**
 * Multi-body simulation container.
 *
 * Owns a set of AbstractBody instances keyed by caller-assigned int IDs.
 * step(dt) advances all bodies in insertion order (deterministic — D-12).
 * time() is the global simulation clock, incremented after each step().
 *
 * Bodies are always ready to step — no explicit initialize() is required.
 */
class Simulation
{
public:
    Simulation()  = default;
    ~Simulation() = default;

    Simulation(const Simulation&)            = delete;
    Simulation& operator=(const Simulation&) = delete;

    Simulation(Simulation&&)                 = default;
    Simulation& operator=(Simulation&&)      = default;

    /** Register a body. Throws std::invalid_argument on duplicate id. */
    void addBody(int id, std::unique_ptr<AbstractBody> body);

    /** Advance all bodies by dt seconds in insertion order. */
    void step(double dt);

    AbstractBody&       body(int id);
    const AbstractBody& body(int id) const;

    /** Shorthand for body(id).state(). */
    BodyState state(int id) const;

    double time()      const noexcept { return t_; }
    int    bodyCount() const noexcept { return static_cast<int>(order_.size()); }
    bool   hasBody(int id) const noexcept;

private:
    std::map<int, std::unique_ptr<AbstractBody>> bodies_;
    std::vector<AbstractBody*>                   order_;  // insertion order for deterministic step
    double                                        t_ = 0.0;
};

} // namespace ymir
