#pragma once

#include <cstddef>
#include <vector>

#include <ymir/common/Types.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/vessel/Thruster.h>
#include <ymir/vessel/Rudder.h>
#include <ymir/vessel/controllers/ManeuverController.h>

// Forward-declare to avoid including simulation headers from vessel layer.
namespace ymir { class CouplingRegistry; }

namespace ymir::naval
{

/**
 * Finite-state-machine controller for harbour berthing operations.
 *
 * Manages three sequential phases:
 *  - Navigating : LOS + PID heading/speed approach (delegates to internal ManeuverController)
 *  - Sideway    : fixed heading (PID) + lateral push via parametric tugs (PD)
 *  - TurnROTTUG : Rate-of-Turn PID via rudder + tugs for lateral hold
 *
 * Parametric tug forces are accumulated in tugForces_ each tick and exposed via
 * tugForces().  A TugParametricForces : NavalForceModel reads this vector and injects
 * the forces into the body before the CVODE integration step.
 *
 * Transitions are one-way:
 *  Navigating → Sideway   when dist to active waypoint < transitionDist_m
 *  Sideway    → TurnROTTUG when |lateral error| < 2 m AND |heading error| < 5°
 *
 * Satisfies the implicit VesselController concept (ADR-002):
 *   void update(double t, double dt, const NavalContext&,
 *               std::vector<Thruster>&, std::vector<Rudder>&).
 */
class BerthManeuverSystem
{
public:
    enum class Phase { Navigating, Sideway, TurnROTTUG };

    /** Per-tug parametric configuration used to compute escort and push forces. */
    struct TugForceConfig
    {
        double escortForce_N = 0.0; ///< force magnitude in ESCORTING mode [N]
        double pushForce_N   = 0.0; ///< maximum force magnitude in PUSH mode [N]
        double bearing_deg   = 0.0; ///< bearing from ship stern in body frame [deg]
        double arm_m         = 0.0; ///< moment arm for yaw torque calculation [m]
    };

    /** Berthing waypoint with FSM transition metadata. */
    struct BerthWaypoint
    {
        double x_m;               ///< inertial x position [m]
        double y_m;               ///< inertial y position [m]
        Phase  targetPhase;       ///< phase to enter when this waypoint is activated
        double transitionDist_m;  ///< trigger Navigating→Sideway when dist < this [m]
        double headingTarget_rad; ///< desired heading at waypoint [rad]
    };

    struct Config
    {
        std::vector<BerthWaypoint>  waypoints;
        std::vector<TugForceConfig> tugs;

        double headingKp = 0.0; ///< heading PID proportional gain
        double headingKi = 0.0; ///< heading PID integral gain
        double headingKd = 0.0; ///< heading PID derivative gain

        double rotRateKp = 0.0; ///< ROT PID proportional gain
        double rotRateKi = 0.0; ///< ROT PID integral gain
        double rotRateKd = 0.0; ///< ROT PID derivative gain

        double lateralKp = 0.0; ///< lateral PD proportional gain [N/m]
        double lateralKd = 0.0; ///< lateral PD derivative gain [N·s/m]

        double captureRadius_m    = 5.0;  ///< legacy capture radius (not used for FSM) [m]
        double maxRudderAngle_rad = 0.35; ///< rudder demand clamp [rad] (~20 deg)
    };

    explicit BerthManeuverSystem(Config cfg);

    /**
     * Advance FSM: evaluate transitions, compute actuator demands and tug forces.
     *
     * Resets tugForces_ to zero at the start of every call. When no registry is set,
     * accumulates tug force contributions into tugForces_. When a registry is set,
     * writes each tug's force individually via CouplingRegistry::writeForce().
     */
    void update(double t, double dt, const NavalContext& ctx,
                std::vector<Thruster>& thrusters, std::vector<Rudder>& rudders);

    /**
     * Attach a CouplingRegistry so tug forces are written to the Jacobi coupling path.
     *
     * After this call, update() writes per-tug forces via registry->writeForce()
     * instead of accumulating into tugForces_. tugForces_() returns zero in this mode.
     * Pass registry=nullptr to restore non-registry (parametric) behavior.
     *
     * @param registry   Non-owning pointer to the shared CouplingRegistry.
     * @param shipBodyId Body id of the vessel receiving tug forces (unused by BSM,
     *                   stored for caller convenience / future use).
     * @param tugBodyIds Producer body ids for each tug, indexed by cfg_.tugs position.
     *                   Size must match cfg_.tugs.size().
     */
    void setCouplingRegistry(ymir::CouplingRegistry* registry,
                             int shipBodyId,
                             const std::vector<int>& tugBodyIds);

    /** Current FSM phase. */
    Phase currentPhase() const noexcept;

    /**
     * Accumulated parametric tug forces from the last update() call.
     *
     * Layout: [Fx, Fy, Fz, Mx, My, Mz] in body frame.
     * Read by TugParametricForces::computeNaval() before each CVODE step.
     */
    const Vector6& tugForces() const noexcept;

private:
    struct PID
    {
        double kp, ki, kd;
        double integral_  = 0.0;
        double prevError_ = 0.0;

        double update(double error, double dt) noexcept;
        void   reset() noexcept;
    };

    void updateNavigating(double t, double dt, const NavalContext& ctx,
                          std::vector<Thruster>& thrusters, std::vector<Rudder>& rudders);
    void updateSideway   (double t, double dt, const NavalContext& ctx,
                          std::vector<Thruster>& thrusters, std::vector<Rudder>& rudders);
    void updateTurnROTTUG(double t, double dt, const NavalContext& ctx,
                          std::vector<Thruster>& thrusters, std::vector<Rudder>& rudders);

    /** Evaluate and apply one-way FSM transitions based on current ctx. */
    void checkTransitions(const NavalContext& ctx);

    /** Signed lateral error: projection of (waypoint - pos) perpendicular to headingTarget. */
    double computeLateralError(const NavalContext& ctx) const;

    /** Route one tug's body-frame force to the active coupling path. */
    void writeTugForce(std::size_t tugIndex, const Vector6& tugForce);

    Config            cfg_;
    Phase             phase_     = Phase::Navigating;
    std::size_t       activeWpt_ = 0;

    ManeuverController navMc_;   ///< internal MC for Navigating phase (captureRadius = 0)

    PID headingPid_;
    PID rotRatePid_;

    Vector6 tugForces_{};

    // Coupling path — null by default (parametric / backward-compat mode).
    ymir::CouplingRegistry* registry_   = nullptr;
    int                     shipBodyId_ = 0;
    std::vector<int>        tugBodyIds_;
};

} // namespace ymir::naval
