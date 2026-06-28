#pragma once

#include <ymir/world/IDomain.h>
#include <ymir/physics/BodyState.h>
#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/physics/NavalForceModel.h>
#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/physics/forces/RudderForces.h>
#include <ymir/simulation/Simulation.h>
#include <ymir/world/CouplingRegistry.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/vessel/DynamicVessel.h>
#include <ymir/world/Environment.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ymir {

/**
 * N-body naval physics domain implementing IDomain.
 *
 * Drop-in replacement for NavalSimulation. Accepts Environment and CouplingRegistry
 * injected by World::addDomain() via onAddedToWorld(). Supports spatial queries
 * (allBodyPositions, distanceBetween) and Jacobi coupling force injection.
 *
 * Non-copyable and non-movable.
 */
class NavalDomain final : public IDomain {
public:
    explicit NavalDomain(std::string name = "naval");

    /** Register a rigid body under the given ID. */
    void addBody(int id, std::unique_ptr<ymir::RigidBody6DOF> body);

    /** Register a naval force model bound to a specific body. */
    void addNavalForceModel(int bodyId, std::unique_ptr<naval::NavalForceModel> model);

    /**
     * Register a DynamicVessel for a body.
     * tf and rf are non-owning; pass nullptr to skip sync for that actuator type.
     */
    void registerVessel(int bodyId, naval::DynamicVessel& vessel,
                        naval::ThrustForces* tf = nullptr,
                        naval::RudderForces* rf = nullptr);

    /** Bind contexts and initialise EMA state for all entries. */
    void initialize();

    /** Reset CVODE time, EMA, and force model states for all entries. */
    void reset();

    /**
     * Return body state for the given ID.
     * @throws std::out_of_range if bodyId was not registered.
     */
    BodyState state(int bodyId) const;

    double time() const noexcept;

    // IDomain interface

    /**
     * Called once by World::addDomain() before the first step.
     * Stores pointers to the injected Environment and CouplingRegistry.
     * Asserts if called more than once.
     */
    void onAddedToWorld(Environment& env, CouplingRegistry& coupling) override;

    /**
     * Advance the domain by dt seconds.
     * Asserts env_ != nullptr — guards against use outside World.
     */
    void step(double dt) override;

    /** Return (x, y, z) position of every body currently in this domain. */
    std::vector<BodyPosition> allBodyPositions() const override;

    /** Return full kinematic state of the body identified by id. */
    BodyState bodyState(int id) const override;

    /** Return the domain name set in the constructor (default: "naval"). */
    std::string name() const override;

    /** Return Euclidean distance between two registered bodies. Asserts both ids exist. */
    double distanceBetween(int idA, int idB) const;

    /**
     * Serialize all body states to a JSON string.
     *
     * Format: {"t":<sim_time>,"vessels":[{"id":<n>,"x":...,"y":...,"z":...,"phi":...,"theta":...,"psi":...,"u":...,"v":...,"r":...},...]}
     * Used by the WASM binding to transfer state to JavaScript.
     */
    std::string serializeStateJson() const;

    NavalDomain(const NavalDomain&)            = delete;
    NavalDomain& operator=(const NavalDomain&) = delete;
    NavalDomain(NavalDomain&&)                 = delete;
    NavalDomain& operator=(NavalDomain&&)      = delete;

private:
    struct BodyEntry
    {
        ymir::RigidBody6DOF*               body   = nullptr;
        std::vector<naval::NavalForceModel*> models;
        naval::NavalContext                 ctx{};
        ymir::Vector6                       q_avg{};
        naval::DynamicVessel*               vessel = nullptr;
        naval::ThrustForces*                thrust = nullptr;
        naval::RudderForces*                rudder = nullptr;
    };

    naval::NavalContext buildContext(int id, const BodyEntry& entry, double dt) const;

    ymir::Simulation         sim_;
    const Environment*       env_      = nullptr;
    CouplingRegistry*        coupling_ = nullptr;
    std::map<int, BodyEntry> entries_;
    std::string              name_;
    bool                     addedToWorld_ = false;
};

} // namespace ymir
