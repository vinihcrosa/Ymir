---
status: completed
title: BerthManeuverSystem coupling update + TugParametricForces deprecated
type: refactor
complexity: medium
dependencies:
  - task_03
---

# Task 6: BerthManeuverSystem coupling update + TugParametricForces deprecated

## Overview

Add optional `CouplingRegistry` support to `BerthManeuverSystem` so it can write
tug forces directly to the registry (Jacobi coupling path) in addition to its existing
`tugForces_` accumulation (parametric path). When the registry pointer is null, behavior
is unchanged — full backward compatibility. Mark `TugParametricForces` as `[[deprecated]]`
with a migration message pointing to `CouplingRegistry + CouplingForceModel`.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `BerthManeuverSystem` MUST gain a new method `void setCouplingRegistry(CouplingRegistry* registry, int shipBodyId, const std::vector<int>& tugBodyIds)`
- When `setCouplingRegistry()` has been called with a non-null registry, `update()` MUST call `registry_->writeForce(tugBodyIds_[i], tugForce_i)` for each tug instead of accumulating in `tugForces_`
- When `registry_` is null (default), `update()` MUST behave identically to its current implementation — `tugForces_` is populated as before
- `tugForces_()` accessor MUST continue to work in both modes for observability
- `TugParametricForces.h` MUST add `[[deprecated("Use CouplingRegistry + CouplingForceModel. TugParametricForces will be removed in Phase 3.")]]` to the class declaration
- `BerthManeuverSystem.h` MUST include `CouplingRegistry.h` only via forward declaration in the header; full include in the `.cpp`
- No change to `BerthManeuverSystem::Config` or `BerthManeuverSystem::BerthWaypoint` structs
- Existing unit tests for `BerthManeuverSystem` MUST pass unchanged (registry_ null by default)
</requirements>

## Subtasks

- [x] 6.1 Add `void setCouplingRegistry(CouplingRegistry*, int shipBodyId, const std::vector<int>& tugBodyIds)` to `BerthManeuverSystem.h`; forward-declare `CouplingRegistry` in header
- [x] 6.2 Add private members `CouplingRegistry* registry_ = nullptr`, `int shipBodyId_ = 0`, `std::vector<int> tugBodyIds_` to `BerthManeuverSystem`
- [x] 6.3 Update `BerthManeuverSystem.cpp`: implement `setCouplingRegistry()`; add conditional branch in tug force write path
- [x] 6.4 Add `[[deprecated]]` to `TugParametricForces` class declaration in `TugParametricForces.h`
- [x] 6.5 Write/update unit tests: verify BSM in registry mode writes forces to registry; verify BSM without registry writes to `tugForces_`

## Implementation Details

See TechSpec "BerthManeuverSystem — updated signature" section for the exact new method
signature and the conditional branch description.

The tug force distribution logic inside `update()` iterates over `cfg_.tugs` and computes
one `Forces` per tug (in body frame). Currently these are accumulated into `tugForces_`
(a `Vector6` sum). In registry mode, each tug's individual force must be written to the
registry with that tug's body id. This requires keeping the per-tug force computation
as an intermediate value before the accumulation step:

Before (current): `tugForces_ += tugForce_i`
After (registry mode): `registry_->writeForce(tugBodyIds_[i], tugForce_i)`
After (non-registry mode): `tugForces_ += tugForce_i` (unchanged)

The `tugBodyIds_` vector is indexed by tug position in `cfg_.tugs`. If `tugBodyIds_.size()`
does not match `cfg_.tugs.size()`, assert (debug) in `setCouplingRegistry()`.

Forward declaration in header:
```cpp
namespace ymir { class CouplingRegistry; }
```
Full include `#include "CouplingRegistry.h"` only in `BerthManeuverSystem.cpp`.

### Relevant Files

- `libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h` — add method and private members
- `libs/vessel/src/controllers/BerthManeuverSystem.cpp` — implement conditional branch
- `libs/vessel/include/ymir/vessel/controllers/TugParametricForces.h` — add [[deprecated]]
- `libs/simulation/include/ymir/simulation/CouplingRegistry.h` (task_03) — forward-declared in header, included in cpp

### Dependent Files

- `libs/simulation/src/NavalDomain.cpp` (task_05) — calls `vessel.updateControl()` which triggers BSM; BSM must have had `setCouplingRegistry()` called before domain.step()
- `tests/simulation/TestNavalDomainCoupling.cpp` (task_07) — calls `setCouplingRegistry()` in integration test setup
- `libs/vessel/tests/TestBerthManeuverSystem.cpp` — existing tests must remain green

### Related ADRs

- [ADR-002: Tugs Promovidos de Forças Paramétricas a Corpos Físicos Independentes com CouplingPort](adrs/adr-002.md) — Rationale for BerthManeuverSystem writing to CouplingRegistry

## Deliverables

- `libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h` — updated with new method and private members
- `libs/vessel/src/controllers/BerthManeuverSystem.cpp` — conditional tug force write path
- `libs/vessel/include/ymir/vessel/controllers/TugParametricForces.h` — `[[deprecated]]` added
- Unit tests covering both registry and non-registry modes **(REQUIRED)**

## Tests

- Unit tests:
  - [x] BSM with `registry_ = nullptr` (default): `update()` populates `tugForces_` as before (existing test passes unchanged)
  - [x] BSM with registry set and 2 tugs: after `update()`, `registry.consumedForce(tugId_0)` and `registry.consumedForce(tugId_1)` contain non-zero, tug-specific forces (after resolve)
  - [x] BSM with registry set: `tugForces_()` returns zero (forces go to registry, not accumulated internally)
  - [x] `setCouplingRegistry()` with `tugBodyIds.size() != cfg_.tugs.size()` triggers assertion
  - [x] BSM without registry in Navigating phase: `tugForces_` matches expected escorting force (regression)
- Integration tests:
  - [ ] (Covered by task_07 coupling integration test)
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Existing `TestBerthManeuverSystem.cpp` tests pass unchanged
- `TugParametricForces` class declaration contains `[[deprecated]]` attribute
- `CouplingRegistry` appears only as a forward declaration in `BerthManeuverSystem.h`
