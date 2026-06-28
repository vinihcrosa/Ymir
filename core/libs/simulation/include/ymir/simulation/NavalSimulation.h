#pragma once

#include <ymir/physics/BodyState.h>
#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/physics/NavalForceModel.h>
#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/physics/forces/RudderForces.h>
#include <ymir/simulation/Simulation.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/vessel/DynamicVessel.h>
#include <ymir/world/Environment.h>

#include <map>
#include <memory>
#include <vector>

namespace ymir::naval
{

/**
 * N-body naval orchestrator.
 *
 * Each body is registered via addBody(id, body) and may optionally have
 * naval force models, and a DynamicVessel aggregate root.
 * step(dt) processes all bodies in ascending ID order.
 */
class [[deprecated("Use NavalDomain + World. NavalSimulation will be removed in Phase 3.")]] NavalSimulation
{
public:
    NavalSimulation() = default;

    NavalSimulation(const NavalSimulation&)            = delete;
    NavalSimulation& operator=(const NavalSimulation&) = delete;

    /** Register a rigid body under the given ID. */
    void addBody(int id, std::unique_ptr<ymir::RigidBody6DOF> body);

    /** Register a naval force model bound to a specific body. */
    void addNavalForceModel(int bodyId, std::unique_ptr<NavalForceModel> model);

    /**
     * Register a DynamicVessel for a body.
     * tf and rf are non-owning; pass nullptr to skip sync for that actuator type.
     */
    void registerVessel(int bodyId, DynamicVessel& vessel,
                        ThrustForces* tf = nullptr,
                        RudderForces* rf = nullptr);

    /** Bind contexts and initialise EMA state for all entries. */
    void initialize();

    void step(double dt);

    /** Reset CVODE time, EMA, and force model states for all entries. */
    void reset();

    void setEnvironment(const ymir::Environment& env);

    /**
     * Return body state for the given ID.
     * @throws std::out_of_range if bodyId was not registered.
     */
    ymir::BodyState state(int bodyId) const;

    double time() const noexcept;

private:
    struct BodyEntry
    {
        ymir::RigidBody6DOF*          body   = nullptr;
        std::vector<NavalForceModel*> models;
        NavalContext                  ctx{};
        ymir::Vector6                 q_avg{};
        DynamicVessel*                vessel = nullptr;
        ThrustForces*                 thrust = nullptr;
        RudderForces*                 rudder = nullptr;
    };

    NavalContext buildContext(int id, const BodyEntry& entry, double dt) const;

    ymir::Simulation          sim_;
    ymir::Environment         env_{};
    std::map<int, BodyEntry>  entries_;
};

} // namespace ymir::naval
