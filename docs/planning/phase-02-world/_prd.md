# PRD: World + Multi-Domain Naval Architecture — Phase 2

## Overview

Phase 2 transforms Ymir from a single-vessel physics engine into a multi-body naval world.
Before this phase, vessels and tugs exist as isolated bodies — tugs are parametric force
sources, environment is embedded inside NavalSimulation, and there is no shared world state.
After this phase, all naval bodies coexist in a single `World` with proper force coupling,
a globally shared environment, and spatial awareness. Real multi-vessel scenarios — ship
navigating into a berth while two physical tugs push and pull — become possible.

Primary consumers: simulation engineers building multi-vessel studies, platform developers
integrating Ymir as a simulation backend for Phase 3 (WebSocket API), and CI pipelines
validating scenario reproducibility across body configurations.

---

## Goals

- A single `World` object encapsulates the complete simulation state — clock, environment,
  all bodies — and is the sole entry point for advancing time.
- N naval bodies (ships and tugs) coexist in one `NavalDomain` and exchange forces via
  weak Jacobi coupling each tick, without tight integration dependencies.
- Tugs are promoted from static parametric forces to independent physics bodies with their
  own heading dynamics and thruster response.
- Wind, current, waves, and tide are configured once on `World` and apply to all bodies
  without duplication.
- Basic spatial queries expose position of all bodies and distance between pairs,
  enabling proximity-based scenario logic and laying the foundation for collision broadphase
  in Phase 4.
- Single-vessel scenarios migrated from `NavalSimulation` to `NavalDomain` produce
  identical numerical output (no behavioral regression).
- Test coverage ≥ 80% per module; zero tick failures with any coupling configuration.

---

## User Stories

### Simulation Engineer

- As a simulation engineer, I want to define a scenario with one ship and two physical
  tugs as independent bodies, so I can study realistic tug-assisted maneuvering dynamics
  where tug heading lag and thruster response affect the force delivered to the ship.
- As a simulation engineer, I want to set environment conditions (wind speed/direction,
  current, wave state, tide) once on the World and have them apply to all bodies
  automatically, so multi-body scenarios do not diverge due to inconsistent environment
  references.
- As a simulation engineer, I want to query the distance between any two bodies at any
  tick, so I can implement proximity-based triggers (berth entry, tug repositioning)
  without maintaining custom bookkeeping external to the simulator.
- As a simulation engineer, I want the World clock to be the authoritative time source,
  so scenario events are synchronized across all bodies with no per-body time drift.

### Platform Developer (Phase 3 consumer)

- As a platform developer, I want to read a complete snapshot of all body states with a
  single call to `world.snapshot()`, so a WebSocket server can serialize the world state
  without iterating internal domain structures.
- As a platform developer, I want to inject a `NavalDomain` into `World` and drive it
  identically to the legacy `NavalSimulation`, so the Phase 3 server works with the new
  architecture without a parallel migration effort.

### CI Pipeline

- As a CI pipeline, I want identical tick output given identical `World` configuration
  and `dt` sequence, so regression tests do not require physics expertise to validate.
- As a CI pipeline, I want to configure a 3-body scenario (ship + 2 tugs) via the
  existing JSON reader and run it to completion, so integration tests cover multi-body
  coupling end-to-end.

---

## Core Features

### 1. World — Domain Orchestrator

`World` is the single entry point for running a simulation. It owns the global clock,
the shared `Environment`, a registry of domains, and the `CouplingRegistry`. It contains
no naval-specific logic.

`World::step(dt)` executes:
1. Advance the global clock by `dt`.
2. Call `domain.step(dt)` for each registered domain.
3. Resolve all `CouplingPort` pairs in `CouplingRegistry` (Jacobi: forces from tick `t`
   applied at `t+dt`).
4. Flush resolved coupling forces into each domain for the next tick.

`World` exposes `WorldSnapshot` — the complete state of all bodies and the environment
at a point in time — as a nested, domain-organized structure suitable for serialization.

### 2. NavalDomain — Naval Body Manager

`NavalDomain` replaces `NavalSimulation` as the class responsible for driving N naval
bodies through the physics tick. Each body owns a `RigidBody6DOF`, a `NavalContext`, and
a `DynamicVessel` (Phase 1).

`NavalDomain` drives the existing naval tick sequence: control update, actuator state
update, hydrodynamic force accumulation, wave forces, inertial forces, restoring forces,
and CVODE integration. Mooring and anchoring steps remain stubs. `NavalDomain` receives
`Environment` by reference from `World` at construction and must not hold a back-pointer
to `World`.

`NavalSimulation` is kept as a thin single-body wrapper around `NavalDomain` for one
phase to avoid breaking existing callers. It is deprecated and targeted for removal in
Phase 3.

### 3. CouplingRegistry — Weak Jacobi Force Coupling

`CouplingRegistry` enables force exchange between bodies without tight integration
coupling. Each `CouplingPort` carries:
- A 6-DOF force vector applied at a named interface point.
- The position and velocity of that interface point (read by the provider after its step).

The registry resolves all port pairs once per `World::step(dt)` after all domains have
stepped. Forces from the previous tick are used during domain steps (Jacobi splitting).
This is physically adequate for naval timescales (dt ≈ 0.05–1.0 s) where coupling
dynamics are much slower than the integration step.

A ship registers a `CouplingPort` as force consumer; a tug registers as force provider.
`BerthManeuverSystem` is updated to write tug force demands into provider ports rather
than into `TugParametricForces`.

### 4. Tug Promotion — Parametric to Independent Body

`TugParametricForces` is deprecated. Tugs become `NavalDomain` entities — each a
`RigidBody6DOF` with its own heading dynamics and thruster response. In Phase 2, tugs
use a simplified `DynamicVessel` (no waypoint controller; they accept direct force
commands from `BerthManeuverSystem` via `CouplingPort`).

The force a tug delivers to a ship now reflects the tug's actual state — heading lag
and thruster dynamics affect the effective force, not just a parameter table. This is
the primary fidelity increase of Phase 2 for maneuvering studies.

`TugParametricForces` remains compilable for one phase as an opt-in fallback; tests
that depend on its deterministic output are not broken immediately.

### 5. Environment Promotion — Local to Global

`NavalEnvironment` is refactored as `Environment`, owned by `World`. All `NavalDomain`
bodies read from the same instance. Setters — `setWind`, `setCurrent`, `setTide`,
`setSeaState` — mutate the shared state; changes take effect on the next tick. This
replaces per-simulation environment configuration that Phase 1 required.

### 6. Spatial Queries

`NavalDomain` exposes:
- `allBodyPositions()` → list of (id, x, y, z) for all bodies.
- `distanceBetween(id_a, id_b)` → scalar Euclidean distance.

These are consumed by `BerthManeuverSystem` for proximity-based tug positioning and
serve as the spatial index foundation for collision broadphase (Phase 4). Initial
implementation is a linear scan; N bodies in naval scenarios is small enough that
a spatial tree is not needed.

---

## User Experience

### Flow: Multi-Vessel Scenario

1. User constructs `World` and configures `Environment` (wind, current, wave spectrum, tide).
2. User creates a `NavalDomain` with one ship and two tugs as independent bodies.
3. User registers `CouplingPort` pairs linking each tug as a force provider for the ship.
4. User calls `world.step(dt)` in a loop.
5. After each tick, user calls `world.snapshot()` to read all body states.
6. User observes tug force contribution in ship DOF response — heading error due to
   tug heading lag is measurable.

### Flow: Migration from NavalSimulation

1. User replaces `NavalSimulation` with `NavalDomain` in their scenario setup code.
2. User wraps `NavalDomain` in `World` and calls `world.step(dt)` instead of
   `simulation.step(dt)`.
3. Single-vessel scenario produces identical numerical output — no behavioral regression.
4. User can continue using `NavalSimulation` as a temporary wrapper without any change.

### Flow: Proximity-Based Scenario Logic

1. User polls `navalDomain.distanceBetween(shipId, berthId)` each tick.
2. When distance drops below a threshold, user triggers `BerthManeuverSystem` transition.
3. FSM transitions are based on real spatial positions, not pre-scripted time offsets.

---

## High-Level Technical Constraints

- `World` resides in `libs/world/`; `NavalDomain` and `CouplingRegistry` in
  `libs/simulation/`.
- Zero heap allocation per tick — all body state and coupling buffers are pre-allocated
  at construction.
- `World::step(dt)` is deterministic: given the same initial state and `dt` sequence,
  output is bitwise identical across platforms.
- Jacobi coupling requires dt ≤ 1.0 s; `NavalDomain` enforces this as a precondition.
- `NavalDomain` is non-copyable and non-movable after bodies are registered.
- `Environment` is not thread-safe; single-threaded execution is the only supported mode
  in Phase 2.
- Test coverage ≥ 80% per module (YMIR_ENABLE_COVERAGE).

---

## Non-Goals (Out of Scope)

- **Terrain and batimetry**: Depth remains a scalar field on `Environment`; batimetry
  as a spatial grid is explicitly out of roadmap scope.
- **EventBus pub/sub**: Deferred to Phase 3, where an external server consumer exists
  to justify the added complexity.
- **WebSocket / Protobuf**: Phase 3.
- **Mooring and Anchoring**: Out of roadmap scope.
- **Collision response**: Phase 4 — broadphase foundations are laid here via spatial
  queries but no narrowphase or impulse response.
- **Gauss-Seidel iterative coupling**: Jacobi splitting is sufficient at naval timescales;
  tighter convergence is deferred until a specific scenario demonstrates divergence.
- **Multi-threaded tick**: Single-threaded only; parallelism is not a Phase 2 requirement.
- **Fast-time batch runner**: Out of roadmap scope.

---

## Phased Rollout Plan

### MVP — Phase 2 (this PRD)

- `World` (GlobalClock + DomainRegistry + CouplingRegistry)
- `NavalDomain` (replaces NavalSimulation for N bodies)
- `Environment` promoted to `World` level
- `CouplingPort` + `CouplingRegistry`
- Tug promotion: parametric → independent physics body
- Spatial queries: `allBodyPositions`, `distanceBetween`
- `BerthManeuverSystem` updated to use `CouplingPort` for tug force commands
- `NavalSimulation` kept as deprecated thin wrapper

**Criteria to proceed to Phase 3:**
- `world.step(dt)` on ship + 2 independent tugs runs 1 000 ticks without error.
- Ship trajectory with coupled tugs differs from parametric-forces baseline — coupling
  effect is measurable (tug heading lag visible in ship DOF response).
- Single-vessel `NavalDomain` output matches legacy `NavalSimulation` within 1e-8 per
  DOF per tick.
- `world.snapshot()` returns correct state for all bodies after each tick.
- Coverage ≥ 80% per module.

### Phase 3 — Control API

- WebSocket + Protobuf server consuming `world.snapshot()`.
- `EventBus` added to `World`; environment and domain state changes publish events.
- JSON scenario reader extended for multi-body configs and `CouplingPort` definitions.
- `NavalSimulation` wrapper removed.

### Phase 4 — Collision

- Collision broadphase using `NavalDomain.allBodyPositions()` for AABB candidate pairs.
- Narrowphase GJK for convex hull intersection.
- Collision impulse applied via `CouplingPort` — no new coupling mechanism needed.

---

## Success Metrics

- **Multi-body correctness**: Ship + 2 tugs scenario produces physically plausible
  trajectory; tug heading lag is measurable as a change in ship sway DOF response.
- **Coupling overhead**: `CouplingRegistry` resolution adds < 0.1 ms per tick for a
  10-body scenario on the reference test machine.
- **Regression**: Single-vessel `NavalDomain` output matches `NavalSimulation` within
  1e-8 per DOF across a 1 000-tick run.
- **Stability**: Zero tick failures across all test scenarios including max-force tug
  configurations.
- **Coverage**: ≥ 80% per module measured by YMIR_ENABLE_COVERAGE.

---

## Risks and Mitigations

| Risk | Mitigation |
|------|-----------|
| Jacobi coupling diverges for high-force tug scenarios | Bound maximum coupling force per tick; enforce dt ≤ 1.0 s as a NavalDomain precondition |
| NavalSimulation migration breaks existing test scenarios | Run NavalSimulation and NavalDomain in parallel during transition; diff outputs; hold green bar before removing NavalSimulation |
| BerthManeuverSystem behavior changes when tugs gain own physics | Keep TugParametricForces as opt-in; parametric tug mode is the BerthManeuverSystem regression baseline |
| World orchestration overhead is non-trivial | Benchmark `world.step` vs bare `NavalDomain.step`; World overhead must be < 5% for 10-body scenarios |
| Tug RigidBody6DOF config is underparameterized | Use simplified tug model (drag only, no waves, no squat) with representative mass and damping; exact values are not critical in Phase 2 |

---

## Architecture Decision Records

- [ADR-001: World como Thin Orchestrator com DomainRegistry](adrs/adr-001.md) — World não executa física; orquestra domínios e resolve coupling; NavalDomain é o primeiro domínio e substitui NavalSimulation.
- [ADR-002: Tugs Promovidos de Forças Paramétricas a Corpos Físicos Independentes com CouplingPort](adrs/adr-002.md) — TugParametricForces deprecated; tugs tornam-se corpos NavalDomain trocando força via CouplingPort.

---

## Open Questions

- Should `NavalSimulation` be deprecated with a compiler warning immediately, or silently
  kept for Phase 2 with removal announced in Phase 3 release notes? Recommendation:
  silent keep + removal in Phase 3 to avoid noise in CI output during the transition.
- `WorldSnapshot` structure: flat list of all bodies or nested by domain? Recommendation:
  nested by domain — aligns with Phase 3 server serialization requirements and avoids
  a flattening step when the server needs per-domain grouping.
- Should tug `DynamicVessel` support `ManeuverController` in Phase 2, enabling autonomous
  tug positioning, or is direct force command via `CouplingPort` sufficient? Recommend
  direct command only for Phase 2; autonomous tug positioning is a Phase 3+ feature.
