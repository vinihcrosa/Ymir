---
status: completed
title: NavalDomain + NavalSimulation deprecation
type: refactor
complexity: high
dependencies:
    - task_01
    - task_02
    - task_03
    - task_04
---

# Task 5: NavalDomain + NavalSimulation deprecation

## Overview

Introduce `NavalDomain` in `libs/simulation/` as a copy-refactor of `NavalSimulation`
that implements `IDomain`, accepts `Environment` by reference (injected at domain
registration time by `World`), supports coupling force injection via `CouplingRegistry`,
and exposes spatial queries (`allBodyPositions`, `distanceBetween`). Mark `NavalSimulation`
with `[[deprecated]]`. A regression test validates that single-vessel `NavalDomain`
output matches `NavalSimulation` output within 1e-8 per DOF per tick across 1 000 steps.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `NavalDomain` MUST be declared in `libs/simulation/include/ymir/simulation/NavalDomain.h` in namespace `ymir`
- `NavalDomain` MUST implement `IDomain`: `onAddedToWorld(Environment&, CouplingRegistry&)`, `step(double dt)`, `allBodyPositions() const`, `bodyState(int id) const`, `name() const`
- `NavalDomain` MUST provide the same setup API as `NavalSimulation`: `addBody`, `addNavalForceModel`, `registerVessel`, `initialize`, `reset`, `state`, `time`
- `NavalDomain::step(dt)` MUST assert `env_ != nullptr` (debug) — guards against use outside World
- `NavalDomain::buildContext()` MUST read from `env_->windSpeed()`, `env_->currentSpeed()`, etc. — NOT from a local struct
- `allBodyPositions()` MUST return a vector of `BodyPosition` with `{id, q[0], q[1], q[2]}` for each registered body
- `distanceBetween(idA, idB)` MUST return Euclidean distance `sqrt((xA-xB)² + (yA-yB)² + (zA-zB)²)`; MUST assert both ids exist
- `NavalDomain` MUST be non-copyable AND non-movable (same contract as `NavalSimulation`)
- `NavalSimulation` MUST be marked with `[[deprecated("Use NavalDomain + World. NavalSimulation will be removed in Phase 3.")]]` on the class declaration
- EMA logic (τ=16.5 s), body stepping order (ascending id), CVODE integration — MUST be identical to NavalSimulation
- Single-vessel regression: NavalDomain output MUST match NavalSimulation within 1e-8 per DOF per tick over 1 000 steps
- `libs/simulation/CMakeLists.txt` MUST add `NavalDomain.cpp` to sources
</requirements>

## Subtasks

- [ ] 5.1 Create `libs/simulation/include/ymir/simulation/NavalDomain.h` with class declaration, `BodyEntry` private struct, and all public methods
- [ ] 5.2 Create `libs/simulation/src/NavalDomain.cpp` — copy-refactor of `NavalSimulation.cpp`; replace `env_.field` accesses with `env_->field()` getter calls
- [ ] 5.3 Implement `onAddedToWorld(Environment& env, CouplingRegistry& coupling)`: store `&env` and `&coupling`; assert not already initialized
- [ ] 5.4 Implement `allBodyPositions()`: iterate `entries_` map, return `BodyPosition{id, body->state().q()[0], ...}` for each
- [ ] 5.5 Implement `distanceBetween(idA, idB)`: look up both entries, compute Euclidean distance from current `state()` values
- [ ] 5.6 Add `[[deprecated]]` attribute to `NavalSimulation` class declaration in `NavalSimulation.h`
- [ ] 5.7 Write unit tests in `tests/simulation/TestNavalDomain.cpp` including regression test against NavalSimulation
- [ ] 5.8 Add `NavalDomain.cpp` to `libs/simulation/CMakeLists.txt`

## Implementation Details

See TechSpec "Core Interfaces — NavalDomain" section for the exact class shape, `BodyEntry`
struct layout, and the `buildContext()` note.

The implementation of `step(dt)` is a direct copy-refactor of `NavalSimulation::step(dt)`.
The only structural differences from NavalSimulation are:

1. `NavalEnvironment env_` → `const Environment* env_` (ptr, set by `onAddedToWorld`)
2. `CouplingRegistry* coupling_` added (set by `onAddedToWorld`)
3. `pending coupling force` injected into force accumulation: after accumulating all
   `NavalForceModel` forces, add `coupling_->consumedForce(id)` to `ΣF` before calling
   `body->step(dt)`. When `coupling_` is null (NavalDomain used without World in tests),
   skip this addition.
4. `buildContext()` reads from `env_->*()` getters instead of `env_.*` fields

`std::string name_` is set by the constructor argument (default: "naval"). `name()` returns it.

For the regression test: instantiate one `NavalSimulation` and one `NavalDomain` with
identical configs, run 1 000 steps with dt=0.1, compare `state(bodyId).q()` and
`state(bodyId).qdot()` element-wise with tolerance 1e-8. Use a simple single-thruster
vessel config with `ZeroForceModel` for non-naval forces to ensure determinism.

Note: `NavalDomain` used in unit tests WITHOUT calling `onAddedToWorld` must not crash
on `step()` — the assertion `env_ != nullptr` protects this. Tests that exercise naval
context building must call `onAddedToWorld` with a local `Environment` and `CouplingRegistry`.

### Relevant Files

- `libs/simulation/include/ymir/simulation/NavalSimulation.h` — source of truth for copy-refactor
- `libs/simulation/src/NavalSimulation.cpp` — implementation to copy-refactor
- `libs/world/include/ymir/world/IDomain.h` (task_02) — interface to implement
- `libs/world/include/ymir/world/Environment.h` (task_01) — injected environment
- `libs/simulation/include/ymir/simulation/CouplingRegistry.h` (task_03) — injected registry
- `libs/simulation/include/ymir/simulation/CouplingForceModel.h` (task_04) — registered on consumer bodies
- `libs/vessel/include/ymir/vessel/NavalContext.h` — built by `buildContext()`
- `libs/simulation/include/ymir/simulation/Simulation.h` — internal body container
- `libs/simulation/CMakeLists.txt` — add NavalDomain.cpp

### Dependent Files

- `libs/world/include/ymir/world/World.h` (task_07) — calls `domain.onAddedToWorld()` and `domain.step()`
- `tests/simulation/TestNavalDomainCoupling.cpp` (task_07) — exercises NavalDomain with coupling
- `tests/simulation/TestNavalDomainRegression.cpp` — regression vs NavalSimulation

### Related ADRs

- [ADR-001: World como Thin Orchestrator com DomainRegistry](adrs/adr-001.md) — NavalDomain is the first domain implementing IDomain
- [ADR-003: IDomain Interface em libs/world para Evitar Dependência Circular](adrs/adr-003.md) — placement and dep graph
- [ADR-004: Acoplamento Jacobi com CouplingRegistry](adrs/adr-004.md) — coupling force injected before CVODE step

## Deliverables

- `libs/simulation/include/ymir/simulation/NavalDomain.h`
- `libs/simulation/src/NavalDomain.cpp`
- `libs/simulation/include/ymir/simulation/NavalSimulation.h` updated with `[[deprecated]]`
- `libs/simulation/CMakeLists.txt` updated
- `tests/simulation/TestNavalDomain.cpp` with unit + regression tests **(REQUIRED)**

## Tests

- Unit tests:
  - [ ] `NavalDomain` with no bodies: `step(dt)` asserts (`env_` null guard)
  - [ ] `NavalDomain` with `onAddedToWorld(env, coupling)`: `step(0.1)` completes without crash (zero-force body)
  - [ ] `allBodyPositions()` on domain with 2 bodies returns vector of 2 `BodyPosition` with correct ids
  - [ ] `allBodyPositions()` positions match `state(id).q()[0..2]` exactly
  - [ ] `distanceBetween(0, 1)` with known positions returns correct Euclidean distance (tolerance 1e-10)
  - [ ] `distanceBetween(0, 999)` asserts (unknown id)
  - [ ] `name()` returns the string passed to constructor
  - [ ] Coupling force injection: body with `CouplingForceModel` registered receives non-zero force in DOF after registry resolves force
- Regression tests:
  - [ ] Single-vessel NavalDomain vs NavalSimulation: |q_domain - q_sim| < 1e-8 per DOF for all 1 000 ticks (dt=0.1, same config, same initial state)
  - [ ] Single-vessel NavalDomain vs NavalSimulation: |qdot_domain - qdot_sim| < 1e-8 per DOF for all 1 000 ticks
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Regression tolerance < 1e-8 per DOF per tick over 1 000 steps
- `NavalSimulation` class declaration contains `[[deprecated]]` attribute
- `NavalDomain::step()` asserts on null `env_` pointer
- `CouplingRegistry` pointer null-check allows NavalDomain to be used without World in tests
