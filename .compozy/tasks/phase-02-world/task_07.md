---
status: completed
title: WorldSnapshot + World orchestrator + integration tests
type: backend
complexity: medium
dependencies:
  - task_01
  - task_02
  - task_03
  - task_05
  - task_06
---

# Task 7: WorldSnapshot + World orchestrator + integration tests

## Overview

Implement `WorldSnapshot` data types and the `World` class in `libs/world/` — the thin
orchestrator that owns `Environment`, a list of `IDomain` instances, and `CouplingRegistry`,
and advances time by delegating to each domain then resolving coupling. Write unit tests
for `World` and the definitive integration test: one ship + two physical tug bodies in a
single `NavalDomain`, driven by `World::step()`, validating that tug coupling forces reach
the ship's DOF response.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `WorldSnapshot`, `DomainSnapshot`, `BodySnapshot`, and `EnvironmentSnapshot` MUST be declared in `libs/world/include/ymir/world/WorldSnapshot.h` in namespace `ymir`
- `World` MUST be declared in `libs/world/include/ymir/world/World.h` in namespace `ymir`
- `World::step(dt)` MUST execute exactly: `time_ += dt; coupling_.reset(); for (auto& d : domains_) d->step(dt); coupling_.resolve();`
- `World::addDomain(unique_ptr<IDomain>)` MUST call `domain->onAddedToWorld(env_, coupling_)` before storing the domain
- `World::addDomain()` MUST assert that no existing domain has the same `name()` — no duplicate domain names
- `World::domain(const std::string& name)` MUST throw `std::out_of_range` when name not found
- `World::snapshot()` MUST return `WorldSnapshot` with `simTime = time_`, `environment` populated from `env_`, and one `DomainSnapshot` per domain with all body positions
- `World` MUST be non-copyable; move semantics optional
- `libs/world/CMakeLists.txt` MUST add `World.cpp` to sources; `WorldSnapshot.h` is header-only
- Integration test: ship (id=0) + tug1 (id=1) + tug2 (id=2) in one `NavalDomain`; after 100 ticks, ship sway DOF (index 1 of `qdot`) MUST differ from a baseline run without coupling by at least 1e-3 (coupling effect is measurable)
- Integration test: `world.snapshot()` after each tick returns correct `simTime` and all 3 body positions
</requirements>

## Subtasks

- [x] 7.1 Create `libs/world/include/ymir/world/WorldSnapshot.h` with `EnvironmentSnapshot`, `BodySnapshot`, `DomainSnapshot`, `WorldSnapshot` structs
- [x] 7.2 Create `libs/world/include/ymir/world/World.h` with `World` class declaration
- [x] 7.3 Create `libs/world/src/World.cpp` with `step()`, `addDomain()`, `domain()`, `snapshot()` implementations
- [x] 7.4 Update `libs/world/CMakeLists.txt` to add `World.cpp` as source and `ymir_simulation` to `target_link_libraries` (needed for `IDomain` virtual dispatch and `CouplingRegistry` if not in world)
- [x] 7.5 Write unit tests in `tests/world/TestWorld.cpp`
- [x] 7.6 Write integration test in `tests/simulation/TestNavalDomainCoupling.cpp`: 3-body scenario with coupling, validate coupling effect on ship DOF

## Implementation Details

See TechSpec "Core Interfaces — World", "Core Interfaces — WorldSnapshot", and
"Data Models — WorldSnapshot" sections for exact shapes.

**CMakeLists dependency note**: `World.cpp` includes `World.h` which includes `IDomain.h`
(from libs/world) and instantiates `CouplingRegistry` (from libs/simulation). This means
`libs/world/CMakeLists.txt` must link `ymir_simulation`. Check current dep direction:
if `ymir_simulation` already links `ymir_world`, adding the reverse creates a circular
dep. **Resolution**: `CouplingRegistry` belongs to `libs/simulation/`. `World.h` forward-
declares or includes `CouplingRegistry.h` from `libs/simulation/`. `libs/world/` must
link `ymir_simulation`. Since `ymir_simulation` (task_03) already links `ymir_world`,
this IS a circular CMake dep.

**Alternative resolution**: Move `CouplingRegistry` to `libs/world/` instead of
`libs/simulation/`. `CouplingRegistry` has no naval-specific types (only `Forces`
from `ymir_physics`). `libs/world/` already links `ymir_physics`. Moving `CouplingRegistry`
to `libs/world/` breaks the circular dep:

```
ymir_world ← ymir_physics, ymir_common   (+ CouplingRegistry now here)
ymir_simulation ← ymir_world, ymir_physics, ymir_vessel, ymir_common
```

**Decision for this task**: Move `CouplingRegistry.h/.cpp` from `libs/simulation/` to
`libs/world/` and update `libs/simulation/CMakeLists.txt` to remove the source entry
there. Update task_03's include path accordingly (`ymir/world/CouplingRegistry.h`). This
requires coordinating with task_03 deliverables — if task_03 was already implemented with
the simulation path, move the files and update includes.

`WorldSnapshot::snapshot()` implementation:
```cpp
WorldSnapshot World::snapshot() const {
    WorldSnapshot s;
    s.simTime = time_;
    s.environment = { env_.windSpeed(), env_.windDirectionNaut(),
                      env_.currentSpeed(), env_.currentDirectionNaut(),
                      env_.waterDepth(), env_.tide() };
    for (const auto& d : domains_) {
        DomainSnapshot ds;
        ds.name = d->name();
        for (const auto& bp : d->allBodyPositions())
            ds.bodies.push_back({ bp.id, d->bodyState(bp.id) });
        s.domains.push_back(std::move(ds));
    }
    return s;
}
```

**Integration test setup** (ship + 2 tugs):
1. Create 3 `RigidBody6DOF` instances (ship, tug1, tug2) with representative configs
2. Create `NavalDomain`; add all 3 bodies; register ship with `DynamicVessel` (BSM controller)
3. Add `CouplingForceModel(0, registry)` to ship's force models
4. Register tug body ids: `registry.addLink(1, 0); registry.addLink(2, 0)`
5. Construct `World`; `world.addDomain(std::move(domain))`
6. Call `bsm.setCouplingRegistry(&world.couplingRegistry(), 0, {1, 2})`
7. Run 100 steps, compare ship sway DOF to baseline (same setup, no coupling)

### Relevant Files

- `libs/world/include/ymir/world/IDomain.h` (task_02) — stored in World
- `libs/world/include/ymir/world/Environment.h` (task_01) — owned by World
- `libs/simulation/include/ymir/simulation/CouplingRegistry.h` (task_03) — may move to libs/world
- `libs/simulation/include/ymir/simulation/NavalDomain.h` (task_05) — concrete domain used in tests
- `libs/simulation/include/ymir/simulation/CouplingForceModel.h` (task_04) — added to ship in integration test
- `libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h` (task_06) — calls `setCouplingRegistry()` in integration test
- `libs/world/CMakeLists.txt` — add World.cpp source
- `tests/world/CMakeLists.txt` (or `tests/CMakeLists.txt`) — add TestWorld and TestNavalDomainCoupling

### Dependent Files

- Phase 3 (future): WebSocket server will consume `world.snapshot()`
- `tests/simulation/TestNavalDomainRegression.cpp` — uses `NavalDomain` without `World`; not affected

### Related ADRs

- [ADR-001: World como Thin Orchestrator com DomainRegistry](adrs/adr-001.md) — World step() sequence defined here
- [ADR-004: Acoplamento Jacobi com CouplingRegistry](adrs/adr-004.md) — reset() before domain steps; resolve() after

## Deliverables

- `libs/world/include/ymir/world/WorldSnapshot.h`
- `libs/world/include/ymir/world/World.h`
- `libs/world/src/World.cpp`
- `libs/world/CMakeLists.txt` updated
- `tests/world/TestWorld.cpp` with unit tests **(REQUIRED)**
- `tests/simulation/TestNavalDomainCoupling.cpp` with integration test **(REQUIRED)**

## Tests

- Unit tests (TestWorld.cpp):
  - [x] `World` default-constructed: `time() == 0.0`
  - [x] `world.step(0.5)` three times: `time() == 1.5`
  - [x] `addDomain()` calls `onAddedToWorld()` on the domain (verify via stub that records the call)
  - [x] `domain("naval")` returns reference to the added domain; `domain("unknown")` throws `std::out_of_range`
  - [x] `addDomain()` with duplicate name asserts
  - [x] `world.step()` calls `coupling_.reset()` before `d->step()` and `coupling_.resolve()` after (verify via coupling state observable in CouplingForceModel)
  - [x] `snapshot().simTime` equals `world.time()` after N steps
  - [x] `snapshot().domains` has one entry per added domain with correct name
  - [x] `snapshot().environment.windSpeed_ms` reflects value set via `world.environment().setWind(5.0, ...)`
- Integration tests (TestNavalDomainCoupling.cpp):
  - [x] 3-body scenario (ship id=0, tug1 id=1, tug2 id=2) runs 100 ticks without error or NaN in any DOF
  - [x] Ship sway DOF (`state(0).qdot()[1]`) after 100 ticks with coupling differs from uncoupled baseline by at least 1e-3
  - [x] `world.snapshot()` at tick 100 returns 3 body positions; tug positions differ from ship position
  - [x] `world.time()` after 100 steps equals `100 * dt` exactly
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `World::step()` body is ≤ 6 lines (thin orchestrator invariant)
- Coupling effect on ship sway DOF is measurable (≥ 1e-3 difference from uncoupled)
- `WorldSnapshot` is nested by domain — `snapshot().domains[0].bodies` contains ship and tug states
- No circular CMake dependency between `libs/world/` and `libs/simulation/`
