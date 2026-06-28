---
status: completed
title: CouplingForceModel
type: backend
complexity: low
dependencies:
    - task_03
---

# Task 4: CouplingForceModel

## Overview

Implement `CouplingForceModel` — a `NavalForceModel` that reads the resolved
coupling force from `CouplingRegistry` for a specific consumer body id and returns
it during force accumulation. This is the bridge that delivers Jacobi coupling forces
into the CVODE integration step of a consumer body. The implementation is header-only.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `CouplingForceModel` MUST be declared in `libs/simulation/include/ymir/simulation/CouplingForceModel.h` in namespace `ymir`
- `CouplingForceModel` MUST inherit from `NavalForceModel` (from `libs/physics/`)
- Constructor MUST accept `int consumerBodyId` and `const CouplingRegistry& registry`
- `computeNaval(const BodyState&, const NavalContext&)` MUST return `registry_.consumedForce(consumerBodyId_)`
- `name()` MUST return `"CouplingForceModel"`
- `CouplingForceModel` MUST NOT store state beyond `consumerBodyId_` and a reference to `registry_`
- `CouplingRegistry` reference MUST outlive the `CouplingForceModel` instance — no ownership
- Implementation MUST be inline in the header (no `.cpp` needed)
- No changes to `CMakeLists.txt` required (header-only, `CouplingRegistry.h` already reachable)
</requirements>

## Subtasks

- [x] 4.1 Create `libs/simulation/include/ymir/simulation/CouplingForceModel.h` with full inline implementation
- [x] 4.2 Write unit tests in `tests/simulation/TestCouplingForceModel.cpp` using a minimal `CouplingRegistry` setup

## Implementation Details

See TechSpec "Core Interfaces — CouplingForceModel" section for the exact class shape.

`NavalForceModel::computeNaval()` is the pure virtual to override. `bindContext()` is
inherited and called by `NavalDomain` before the force accumulation step — `CouplingForceModel`
ignores the context since it reads from the registry, not from the body state. The `NavalContext*`
passed to `computeNaval()` is unused.

Note: `NavalForceModel::compute()` (the base of `ForceModel`) asserts that the context is
bound before calling `computeNaval()`. `CouplingForceModel` must be registered via
`NavalDomain::addNavalForceModel()` so `bindContext()` is called correctly during
`NavalDomain::initialize()`.

### Relevant Files

- `libs/physics/include/ymir/physics/NavalForceModel.h` — base class (`NavalForceModel`)
- `libs/physics/include/ymir/physics/ForceModel.h` — root base class
- `libs/simulation/include/ymir/simulation/CouplingRegistry.h` (task_03) — provides `consumedForce()`
- `libs/physics/include/ymir/physics/Forces.h` — return type

### Dependent Files

- `libs/simulation/include/ymir/simulation/NavalDomain.h` (task_05) — registers CouplingForceModel on consumer bodies
- `libs/world/include/ymir/world/World.h` (task_07) — creates and passes CouplingForceModel to domains

### Related ADRs

- [ADR-004: Acoplamento Jacobi com CouplingRegistry](adrs/adr-004.md) — One-tick delay semantics: this model reads resolved forces from the previous tick

## Deliverables

- `libs/simulation/include/ymir/simulation/CouplingForceModel.h` — header-only class
- `tests/simulation/TestCouplingForceModel.cpp` with unit tests **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `CouplingForceModel` registered as NavalForceModel: `compute()` returns `Forces::zero()` when no force written to registry
  - [x] After `registry.addLink(1, 0); registry.writeForce(1, F); registry.resolve()`: `CouplingForceModel(0, registry).compute(state)` returns `F`
  - [x] After `registry.reset()` (without new writeForce + resolve): force from previous tick remains (Jacobi invariant)
  - [x] `name()` returns `"CouplingForceModel"`
  - [x] Multiple `CouplingForceModel` instances with different consumer ids on the same registry return independent forces
- Integration tests:
  - [ ] (Covered by task_07 integration test — ship body receives tug coupling force in its DOF response)
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Header-only — no `.cpp` file required
- `computeNaval()` contains exactly one statement: `return registry_.consumedForce(consumerBodyId_)`
