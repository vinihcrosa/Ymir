---
status: completed
title: IDomain interface + BodyPosition struct
type: backend
complexity: low
dependencies:
    - task_01
---

# Task 2: IDomain interface + BodyPosition struct

## Overview

Define the `IDomain` abstract interface and `BodyPosition` POD struct in
`libs/world/include/ymir/world/`. These two types are the polymorphic contract
that `World` uses to store and step domains without knowing their concrete type.
`IDomain` must not reference any naval-specific type; `BodyPosition` is a plain
data carrier for spatial queries.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `IDomain` MUST be declared in `libs/world/include/ymir/world/IDomain.h` in namespace `ymir`
- `IDomain` MUST have exactly four pure virtuals: `onAddedToWorld(Environment&, CouplingRegistry&)`, `step(double dt)`, `allBodyPositions() const`, `bodyState(int id) const`, plus `name() const`
- `IDomain` MUST delete copy constructor and copy assignment; move semantics optional
- `IDomain` MUST have a virtual destructor
- `BodyPosition` MUST be a plain struct with `int id`, `double x`, `double y`, `double z`
- `CouplingRegistry` MUST be forward-declared in `IDomain.h` — not included (avoids circular header dep until CouplingRegistry.h is written in task_03)
- No naval-specific types (NavalContext, RigidBody6DOF, DynamicVessel) may appear in IDomain.h
- `libs/world/CMakeLists.txt` does NOT need to change (header-only additions)
</requirements>

## Subtasks

- [x] 2.1 Create `libs/world/include/ymir/world/IDomain.h` with `IDomain` abstract class and `BodyPosition` struct
- [x] 2.2 Add forward declaration `class CouplingRegistry;` in `IDomain.h` (from namespace `ymir`)
- [x] 2.3 Verify that `IDomain.h` compiles in isolation (no transitive includes beyond `Environment.h` and `BodyState.h`)

## Implementation Details

See TechSpec "Core Interfaces — IDomain" section for the exact virtual method signatures.

`BodyState` is already defined in `libs/physics/include/ymir/physics/BodyState.h`. Include
it in `IDomain.h` since `bodyState(int id) const` returns it.

`allBodyPositions()` returns `std::vector<BodyPosition>`. This is the only heap allocation
that escapes per-tick — acceptable since it is called for observation, not in the hot path.

`onAddedToWorld` is called once at domain registration time, not per tick. It receives
mutable references so the domain can cache pointers.

### Relevant Files

- `libs/world/include/ymir/world/Environment.h` (task_01) — referenced in `onAddedToWorld` signature
- `libs/physics/include/ymir/physics/BodyState.h` — returned by `bodyState()`
- `libs/world/CMakeLists.txt` — no change needed (header-only)

### Dependent Files

- `libs/simulation/include/ymir/simulation/NavalDomain.h` (task_05) — implements IDomain
- `libs/world/include/ymir/world/World.h` (task_07) — stores `std::vector<std::unique_ptr<IDomain>>`

### Related ADRs

- [ADR-001: World como Thin Orchestrator com DomainRegistry](adrs/adr-001.md) — IDomain is the registry contract
- [ADR-003: IDomain Interface em libs/world para Evitar Dependência Circular](adrs/adr-003.md) — Placement rationale

## Deliverables

- `libs/world/include/ymir/world/IDomain.h` — `IDomain` + `BodyPosition`
- Compiles cleanly in isolation (no warnings, no naval-specific includes)
- Unit tests **(REQUIRED)**

## Tests

- Unit tests:
  - [x] Concrete stub implementing all IDomain virtuals compiles and links
  - [x] `IDomain` is not default-constructible (abstract class)
  - [x] `IDomain` is not copy-constructible (deleted)
  - [x] `BodyPosition{1, 1.0, 2.0, 3.0}` aggregate-initializes correctly
  - [x] `std::unique_ptr<IDomain>` holding a stub calls the virtual destructor correctly
- Integration tests:
  - [x] (Covered by task_07 World tests — no standalone integration test needed for this task)
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `IDomain.h` includes no naval or simulation headers — only `Environment.h`, `BodyState.h`, and standard library headers
- `CouplingRegistry` appears only as a forward declaration
