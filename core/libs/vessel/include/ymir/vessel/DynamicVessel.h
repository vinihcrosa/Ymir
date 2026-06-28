#pragma once

#include <cstddef>
#include <variant>
#include <vector>

#include <ymir/vessel/VesselConfig.h>
#include <ymir/vessel/VesselState.h>
#include <ymir/vessel/Thruster.h>
#include <ymir/vessel/Rudder.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/vessel/controllers/ManeuverController.h>
#include <ymir/vessel/controllers/PrescribedController.h>
#include <ymir/vessel/controllers/BerthManeuverSystem.h>
#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/physics/forces/RudderForces.h>

namespace ymir::naval
{

/**
 * Active controller type — dispatched via std::visit at each tick (ADR-002).
 *
 * All three types satisfy the implicit VesselController concept:
 *   void update(double t, double dt, const NavalContext&,
 *               std::vector<Thruster>&, std::vector<Rudder>&);
 */
using VesselController = std::variant<
    ManeuverController,
    PrescribedController,
    BerthManeuverSystem
>;

/**
 * DynamicVessel — aggregate root for a single simulated vessel (ADR-001).
 *
 * Owns Thruster[], Rudder[], VesselState, and the active VesselController.
 * Orchestrates two tick phases before each CVODE integration step:
 *   1. updateControl(t, dt, ctx) — controller pushes demands onto actuators
 *   2. updateStates(dt)          — actuators advance their own dynamics
 * Followed by syncToForceModels() to push actuator state into ThrustForces /
 * RudderForces before CVODE integrates the body.
 *
 * Non-copyable and non-movable: NavalSimulation holds a raw non-owner pointer;
 * moving the vessel would silently invalidate that pointer.
 */
class DynamicVessel
{
public:
    /**
     * Construct from static vessel config and pre-built actuator entities.
     *
     * cfg must outlive this object (non-owner reference).
     * controller_ is initialised to PrescribedController with empty tables
     * (zero demands — drift mode) until setController() is called.
     */
    DynamicVessel(const VesselConfig&   cfg,
                  std::vector<Thruster> thrusters,
                  std::vector<Rudder>   rudders);

    DynamicVessel(const DynamicVessel&)            = delete;
    DynamicVessel& operator=(const DynamicVessel&) = delete;
    DynamicVessel(DynamicVessel&&)                 = delete;
    DynamicVessel& operator=(DynamicVessel&&)      = delete;

    /** Replace the active controller.  Call outside the tick loop — not thread-safe. */
    void setController(VesselController controller);

    /**
     * Phase 1: dispatch to the active controller via std::visit.
     * The controller reads ctx and pushes demands onto thrusters_ and rudders_.
     */
    void updateControl(double t, double dt, const NavalContext& ctx);

    /**
     * Phase 2: advance all actuator dynamics by dt seconds.
     * Calls thruster.update(dt) then rudder.update(dt) in index order.
     */
    void updateStates(double dt) noexcept;

    /**
     * Phase 3: push current actuator state into ThrustForces / RudderForces.
     * No-op when either pointer is null (ADR-003).
     */
    void syncToForceModels(ThrustForces* tf, RudderForces* rf) noexcept;

    /** Mutable access to vessel COLREGS / navigation state. */
    ymir::vessel::VesselState& vesselState() noexcept;

    /** Const access to vessel COLREGS / navigation state. */
    const ymir::vessel::VesselState& vesselState() const noexcept;

    /**
     * Read-only access to thruster at idx.
     * @throws std::out_of_range if idx >= thrusterCount()
     */
    const Thruster& thruster(std::size_t idx) const;

    /**
     * Read-only access to rudder at idx.
     * @throws std::out_of_range if idx >= rudderCount()
     */
    const Rudder& rudder(std::size_t idx) const;

    /** Number of installed thrusters. */
    std::size_t thrusterCount() const noexcept;

    /** Number of installed rudders. */
    std::size_t rudderCount() const noexcept;

private:
    const VesselConfig&           cfg_;
    std::vector<Thruster>         thrusters_;
    std::vector<Rudder>           rudders_;
    VesselController              controller_;
    ymir::vessel::VesselState     vesselState_;
};

} // namespace ymir::naval
