#pragma once

#include <ymir/world/CouplingRegistry.h>
#include <ymir/world/Environment.h>
#include <ymir/world/IDomain.h>
#include <ymir/world/WorldSnapshot.h>

#include <memory>
#include <string>
#include <vector>

namespace ymir {

/** Thin domain orchestrator owning clock, environment, domains, and coupling. */
class World {
public:
    World() = default;
    ~World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    /** Advance all domains by dt seconds and resolve Jacobi coupling. */
    void step(double dt);

    /** Register a domain after injecting shared world services. */
    void addDomain(std::unique_ptr<IDomain> domain);

    /** Return the domain named by name.
     *  @throws std::out_of_range when no domain with that name exists.
     */
    IDomain& domain(const std::string& name);

    /** Return the domain named by name.
     *  @throws std::out_of_range when no domain with that name exists.
     */
    const IDomain& domain(const std::string& name) const;

    /** Return a nested snapshot of the current world state. */
    [[nodiscard]] WorldSnapshot snapshot() const;

    /** Current authoritative world time in seconds. */
    double time() const noexcept { return time_; }

    /** Mutable shared environment configured by scenario code. */
    Environment& environment() noexcept { return env_; }

    /** Read-only shared environment. */
    const Environment& environment() const noexcept { return env_; }

    /** Shared coupling registry owned by this world. */
    CouplingRegistry& couplingRegistry() noexcept { return coupling_; }

    /** Read-only shared coupling registry. */
    const CouplingRegistry& couplingRegistry() const noexcept { return coupling_; }

private:
    double                             time_ = 0.0;
    Environment                        env_;
    CouplingRegistry                   coupling_;
    std::vector<std::unique_ptr<IDomain>> domains_;
};

} // namespace ymir
