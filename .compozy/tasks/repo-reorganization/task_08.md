---
status: completed
title: Write CMakeLists.txt for 6 libs + update root
type: infra
complexity: high
dependencies:
  - task_02
  - task_03
  - task_04
  - task_05
  - task_06
  - task_07
---

# Task 8: Write CMakeLists.txt for 6 libs + update root

## Overview

Creates the real CMake targets for all six bounded-context libraries and wires them into the root build. This is the CMake pivot: until this task, all files live in their new locations but nothing compiles because the old `core/`, `naval/`, `adapters/` CMakeLists still control the build. After this task, the new targets own the build. High complexity because six library targets must be authored, dependency graph must be correct, and root CMakeLists must be rewritten.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST create `libs/<context>/CMakeLists.txt` for all six contexts with `add_library` + `target_include_directories` + `target_link_libraries`
- MUST define CMake alias targets: `Ymir::Common`, `Ymir::Physics`, `Ymir::Simulation`, `Ymir::World`, `Ymir::Vessel`, `Ymir::Persistence`
- MUST implement the CMake dependency graph from TechSpec "CMake Dependency Graph" section exactly
- MUST update root `CMakeLists.txt` to `add_subdirectory` for all six libs (and remove or disable old `core/`, `naval/`, `adapters/` subdirs)
- MUST create `apps/server/CMakeLists.txt` and `apps/fast-time/CMakeLists.txt` with stubs that link against the new targets
- MUST NOT introduce circular dependencies (see ADR-003)
- `ymir_persistence` target MUST link `nlohmann_json` as a dependency
- `ymir_physics` target MUST link `SUNDIALS::cvode`
</requirements>

## Subtasks

- [x] 8.1 Write `libs/common/CMakeLists.txt` — header-only interface library, `Ymir::Common` alias
- [x] 8.2 Write `libs/physics/CMakeLists.txt` — links `ymir_common` + `SUNDIALS::cvode`, `Ymir::Physics` alias
- [x] 8.3 Write `libs/simulation/CMakeLists.txt` — links `ymir_physics` + `ymir_common`, `Ymir::Simulation` alias
- [x] 8.4 Write `libs/vessel/CMakeLists.txt` — links `ymir_physics` + `ymir_common`, `Ymir::Vessel` alias
- [x] 8.5 Write `libs/world/CMakeLists.txt` — links `ymir_physics` + `ymir_common`, `Ymir::World` alias
- [x] 8.6 Write `libs/persistence/CMakeLists.txt` — links `ymir_physics` + `ymir_world` + `ymir_vessel` + `nlohmann_json`, `Ymir::Persistence` alias
- [x] 8.7 Update root `CMakeLists.txt`: add all six `add_subdirectory(libs/...)`, add `apps/`, disable/remove `core/`, `naval/`, `adapters/`
- [x] 8.8 Write stub `apps/server/CMakeLists.txt` and `apps/fast-time/CMakeLists.txt`

## Implementation Details

See TechSpec "CMake Dependency Graph" and "22-step Build Order" for exact dependency chain and target definitions.

Dependency graph:
```
ymir_common (no deps)
ymir_physics → ymir_common + SUNDIALS::cvode
ymir_simulation → ymir_physics + ymir_common
ymir_world → ymir_physics + ymir_common
ymir_vessel → ymir_physics + ymir_common
ymir_persistence → ymir_physics + ymir_world + ymir_vessel + nlohmann_json
```

`libs/common/` is header-only — use `INTERFACE` library type, no sources.

Root `CMakeLists.txt` currently calls `add_subdirectory(core)`, `add_subdirectory(naval)`, `add_subdirectory(adapters)` — these must be replaced/removed once all source files are moved.

### Relevant Files

- `CMakeLists.txt` (root) — must add six new lib subdirs and apps/
- `core/CMakeLists.txt` — reference for existing target definition before removal
- `naval/CMakeLists.txt` — reference for existing target definition before removal
- `adapters/CMakeLists.txt` — reference for existing target definition before removal
- `applications/CMakeLists.txt` — reference for `apps/server/CMakeLists.txt` content
- All six stub `libs/<context>/CMakeLists.txt` (created in task_01) — replace stubs with real targets

### Dependent Files

- `tests/CMakeLists.txt` (task_09) — will link against new `Ymir::*` targets; depends on this task completing
- `docs/CMakeLists.txt` (task_12) — will reference targets for Doxygen

### Related ADRs

- [ADR-003: One CMake Target Per Bounded Context](adrs/adr-003.md) — one lib, one target, alias pattern

## Deliverables

- Six `libs/<context>/CMakeLists.txt` files with real targets
- `apps/server/CMakeLists.txt` and `apps/fast-time/CMakeLists.txt` stubs
- Updated root `CMakeLists.txt`
- `cmake -B build && cmake --build build` exits 0
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `cmake -B build` exits 0 with no errors or undefined-target warnings
  - [x] `cmake --build build` compiles all six lib targets without error
  - [x] `cmake --build build -- --verbose` shows each `.cpp` compiled with correct include paths
  - [x] `grep -r "add_subdirectory(core)" CMakeLists.txt` returns zero results
  - [x] `grep -r "add_subdirectory(naval)" CMakeLists.txt` returns zero results
- Integration tests:
  - [x] `ctest --test-dir build` still passes (tests not yet reorganized but existing test targets link against new libs)
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `cmake -B build && cmake --build build` exits 0
- No undefined-target or circular-dependency CMake errors
- Six `Ymir::*` alias targets available in the build graph
