# Data Flow

---

## Simulation Tick Sequence

Each call to `Simulation::tick(dt)` executes the following 12 steps in order. Every step is mandatory and the sequence is deterministic — no module may reorder or skip steps.

```
Step  Module              Action
────  ──────────────────  ────────────────────────────────────────────────────────────
 1    Environment         Update ambient conditions: wind speed/direction, current
                          field, tidal level. Values become available to all modules
                          for the rest of this tick.

 2    Terrain             Apply any pending terrain mutations (bathymetry changes,
                          structure additions). Spatial queries after this step
                          reflect the new geometry.

 3    Vessel (control)    updateControl(time, dt) — if maneuver or berthManeuver
                          mode: compute rudder/RPM demands from waypoint guidance
                          (LOS bearing, PID heading error, PID speed error).

 4    Vessel (states)     updateStates(time, dt) — transform current/wind to body
                          frame; advance actuators one step (1st-order filter for
                          RPM, rate limiter for azimuth and rudder); compute SOG,
                          COG, drift angle, speed-to-water, EMA position.

 5    Hydrodynamics       Compute hydrodynamic forces: DampingForces, CurrentForces,
                          SquatForces, ThrustForces, RudderForces, TugForces.
                          Each module returns a 6-DOF loads vector.

 6    WaveForces          Compute wave excitation (1st-order + drift + slow-drift +
                          RAO motion) per spectral component via WAMIT tables.

 7    InertialForces      Coriolis and gyroscopic coupling forces.

 8    RestoringForces     Hydrostatic spring (Archimedes): heave, roll, pitch.
                          Includes tide correction and cg vs. cf offset.

 9    Mooring             Compute mooring cable tension forces at each fairlead.
                          Publish mooring_ruptured event if break threshold exceeded.

10    Anchoring           Compute anchor catenary retention force.
                          Publish anchor_holding / anchor_dragging events as needed.

11    Physics / CVODE     Accumulate all force vectors from steps 5–10.
                          Solve (M + A)·q̈ = ΣF for accelerations.
                          Integrate motion via CVODE (adaptive BDF sub-steps).
                          Write updated position and velocity back to World.

12    Events              Publish all tick events to internal subscribers and,
                          in real-time mode, to external clients (Manager, Viewer).
```

---

## Force Execution Order (Step 5–10 Detail)

Within the physics accumulation phase, forces are evaluated and summed in this order:

```
1. InertialForces    — Coriolis/gyroscopic coupling (step 7 above)
2. RestoringForces   — hydrostatic spring: heave, roll, pitch (step 8)
3. DampingForces     — potential + linear + quadratic (step 5)
4. SquatForces       — shallow-water effect, ICORELS model (step 5)
5. CurrentForces     — current drag: Obokata or Regular + VIM (step 5)
6. WindForces        — wind drag by projected area, Cd tables (step 5)
7. ThrustForces      — propulsor thrust: open-water Kt/Kq curves (step 5)
8. RudderForces      — rudder lift/drag: foil Cl/Cd model (step 5)
9. TugForces         — tug forces: PUSH/PULL/ESCORTING modes (step 5)
10. WaveForces       — wave excitation + drift + RAO (step 6)
```

Each force module signature:

```
Forces module(const BodyState& state, const NavalContext& ctx, const NavalEnvironment& env);
```

Modules are pure functions of state — no side effects, no heap allocation.

---

## CVODE Integration Loop (Step 11)

CVODE is the SUNDIALS BDF integrator used to advance the rigid-body state.

```
Input:  current state q[12] = [x, y, z, roll, pitch, yaw, ẋ, ẏ, ż, ṙoll, ṗitch, ẏaw]
        output interval dt (e.g. 0.05 – 0.1 s)
        accumulated force vector ΣF[6]

RHS function called by CVODE on each internal sub-step:
    q̈ = invTotalMass · ΣF(t, q, q̇)    where invTotalMass = (M + A)⁻¹ (pre-computed)
    velocity in body frame → inertial frame:
        ẋ_inertial = cos(ψ)·u − sin(ψ)·v
        ẏ_inertial = sin(ψ)·u + cos(ψ)·v
        ṗitch ≈ θ̇,  ṙoll ≈ φ̇           (small-angle approximation)

CVODE parameters:
    method:   BDF (Backward Differentiation Formula) — stable for stiff systems
    linear solver: dense (SUNMatrix_Dense)
    Jacobian: finite-difference approximation
    reltol:   1e-8
    abstol:   1e-10
    maxSteps: 10 000 per output interval

Output: updated q[12] written to World for this body
```

Each body has its own independent CVODE instance — state vectors and solver memory are not shared between bodies. Isolation guarantees that a stiff body does not force smaller sub-steps on other bodies.

---

## Multiple Simulations

| Mode | Isolation unit | Concurrency |
|------|---------------|-------------|
| `apps/server` real-time | Thread per `Simulation + World` | 1–2 live simulations |
| `apps/fast-time` batch | Process per `Simulation + World` | N parallel batch runs |

No global state exists — every simulation is a fully isolated object graph.

---

## Event Propagation

Events generated during a tick (step 12) flow through two channels:

```
Tick event
    │
    ├─► Internal subscribers  (synchronous, same thread, zero-latency)
    │   e.g. Mooring listening to entity_removed
    │
    └─► External clients      (real-time mode only)
            └─► Server → WebSocket → Manager, Viewer
```

In fast-time mode events are propagated internally only; no external push occurs.

| Event | Published by |
|-------|-------------|
| `collision` | Physics |
| `entity_created` / `entity_removed` | World |
| `mooring_ruptured` | Mooring |
| `anchor_holding` / `anchor_dragging` | Anchoring |
| `environment_changed` | Environment |
| `terrain_changed` | Terrain |
| `simulation_started` / `simulation_paused` / `simulation_ended` | Simulation |
| `vessel_state_changed` | VesselState |
