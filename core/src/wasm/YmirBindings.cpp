/**
 * YmirBindings.cpp — Emscripten Embind bindings for the Ymir naval physics engine.
 *
 * Exposes a YmirSimulation class to JavaScript that wraps the World + NavalDomain
 * pipeline. State is returned via emscripten::val to avoid the need for additional
 * Embind value-object registrations for nested arrays.
 *
 * Usage from a Web Worker:
 *
 *   const sim = new Module.YmirSimulation();
 *   sim.addVessel(0);
 *   sim.step(0.1);
 *   const state = sim.getState();  // { t, vessels: [{id,x,y,z,phi,theta,psi,u,v,r},...] }
 *   sim.delete();
 */

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <ymir/simulation/NavalDomain.h>
#include <ymir/world/World.h>
#include <ymir/world/Environment.h>
#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/physics/BodyState.h>
#include <ymir/common/Types.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using emscripten::val;

// ---------------------------------------------------------------------------
// Default body parameters used when addVessel() is called without a config.
// A diagonal mass matrix with mass=1e5 kg and zero added mass is sufficient
// for a free-floating body that responds to external forces.
// ---------------------------------------------------------------------------

static std::unique_ptr<ymir::RigidBody6DOF> makeDefaultBody(int id)
{
    ymir::Matrix6x6 mass{};
    constexpr double kDefaultMass = 1.0e5; // 100 tonnes
    for (int i = 0; i < 6; ++i)
        mass[i][i] = kDefaultMass;

    ymir::Matrix6x6 added{};  // zero added mass
    ymir::Vector6   q{};      // at origin, zero heading
    ymir::Vector6   qdot{};   // at rest

    ymir::RK45Config cfg{};
    cfg.reltol   = 1e-6;
    cfg.abstol   = 1e-8;
    cfg.maxSteps = 10000;

    return std::make_unique<ymir::RigidBody6DOF>(id, mass, added, q, qdot, cfg);
}

// ---------------------------------------------------------------------------
// YmirSimulation — thin facade over World + NavalDomain
// ---------------------------------------------------------------------------

class YmirSimulation
{
public:
    YmirSimulation()
        : world_(std::make_unique<ymir::World>())
        , domain_(nullptr)
    {
        // Create a NavalDomain and add it to the World.
        // World::addDomain() calls domain.onAddedToWorld(env, coupling) which
        // injects the shared Environment and CouplingRegistry pointers required
        // before any step() call.
        auto domainPtr = std::make_unique<ymir::NavalDomain>("naval");
        domain_ = domainPtr.get();
        world_->addDomain(std::move(domainPtr));
    }

    /**
     * Add a vessel with a default free-floating rigid body.
     *
     * Must be called before the first step(). Calling addVessel() after step()
     * is undefined behaviour (the integrator is already initialised for the
     * existing bodies; the new body will be initialised on its first step).
     *
     * @param id  Integer vessel identifier. Must be unique within this simulation.
     */
    void addVessel(int id)
    {
        domain_->addBody(id, makeDefaultBody(id));
        // No NavalForceModel registered — body is force-free by default.
        // No DynamicVessel registered — actuators are not wired up in this
        // minimal binding. Extend here to wire thrust/rudder force models.

        if (!initialized_)
        {
            domain_->initialize();
            initialized_ = true;
        }
        else
        {
            // Re-initialise so the newly added body gets its integrator set up.
            domain_->initialize();
        }
    }

    /**
     * Advance the simulation by dt seconds.
     *
     * Delegates to World::step() which drives all registered domains and
     * resolves Jacobi coupling forces.
     *
     * @param dt  Time step in seconds. Typical values: 0.05 – 1.0 s.
     */
    void step(double dt)
    {
        world_->step(dt);
    }

    /**
     * Return the current simulation state as a JS value object.
     *
     * Format:
     *   {
     *     t: number,
     *     vessels: [
     *       { id, x, y, z, phi, theta, psi, u, v, r },
     *       ...
     *     ]
     *   }
     *
     * Field semantics (from BodyState):
     *   x, y, z      — inertial-frame position (m)
     *   phi, theta, psi — roll, pitch, yaw (rad)
     *   u, v, r      — surge, sway, yaw rate in body frame (m/s, m/s, rad/s)
     *
     * Uses NavalDomain::serializeStateJson() to obtain the JSON string, then
     * parses it via JSON.parse() in the JS runtime to produce a native JS
     * object without manual field-by-field assembly overhead.
     */
    val getState() const
    {
        const std::string json = domain_->serializeStateJson();
        // Delegate JSON parsing to the JS runtime — fastest path and avoids
        // linking a C++ JSON library into the WASM module.
        val JSON = val::global("JSON");
        return JSON.call<val>("parse", val(json));
    }

    /**
     * Reset the simulation to t=0.
     *
     * Resets the NavalDomain (CVODE time, EMA, force model states).
     * World time is NOT directly resettable via the public API; it is managed
     * by World::step(). Resetting via NavalDomain alone is sufficient for the
     * domain state — the World time counter will continue from the last value.
     *
     * If a full world-time reset is required, reconstruct the YmirSimulation.
     */
    void reset()
    {
        domain_->reset();
    }

    /** Return current simulation time in seconds (from the World clock). */
    double getTime() const
    {
        return world_->time();
    }

private:
    std::unique_ptr<ymir::World> world_;
    ymir::NavalDomain*           domain_;      // non-owning; owned by world_
    bool                         initialized_ = false;
};

// ---------------------------------------------------------------------------
// Embind registration
// ---------------------------------------------------------------------------

EMSCRIPTEN_BINDINGS(ymir)
{
    emscripten::class_<YmirSimulation>("YmirSimulation")
        .constructor<>()
        .function("addVessel", &YmirSimulation::addVessel)
        .function("step",      &YmirSimulation::step)
        .function("getState",  &YmirSimulation::getState)
        .function("reset",     &YmirSimulation::reset)
        .function("getTime",   &YmirSimulation::getTime);
}
