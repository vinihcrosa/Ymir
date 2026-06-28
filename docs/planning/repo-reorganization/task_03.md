---
status: completed
title: Move `libs/physics/` — bodies, integrator, all force implementations
type: refactor
complexity: high
dependencies:
  - task_01
  - task_02
---

# Task 3: Move `libs/physics/` — bodies, integrator, all force implementations

## Overview

Moves the largest set of files in the reorganization: all body/force/integrator code from `core/` and all nine force implementations from `naval/forces/`. Updates every cross-header `#include` to use the new `ymir/physics/` namespace. This is the highest-impact task; all other lib moves depend on it being complete.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST move all headers and sources listed in TechSpec "File Mapping — core/ → libs/" (physics rows) and "File Mapping — naval/ → libs/" (forces rows and NavalForceModel)
- MUST update every `#include <ymir/core/...>` (non-math, non-Simulation) to `#include <ymir/physics/...>`
- MUST update every `#include <ymir/naval/forces/...>` to `#include <ymir/physics/forces/...>`
- MUST update every `#include <ymir/naval/NavalForceModel.h>` to `#include <ymir/physics/NavalForceModel.h>`
- Headers that previously included `ymir/core/math/` MUST now use `ymir/common/math/` (completed in task_02)
- MUST NOT move Simulation.h/.cpp (task_04), VesselConfig/NavalContext (task_05), wave files (task_06)
- MUST NOT add CMakeLists.txt target yet (task_08)
</requirements>

## Subtasks

- [x] 3.1 Move all `core/include/ymir/core/` headers (AbstractBody, BodyState, ForceModel, Forces, RigidBody6DOF, Types, integrator/) to `libs/physics/include/ymir/physics/`
- [x] 3.2 Move all `core/src/` files (excluding Simulation.cpp) to `libs/physics/src/`
- [x] 3.3 Move all `naval/include/ymir/naval/forces/*.h` to `libs/physics/include/ymir/physics/forces/`
- [x] 3.4 Move all `naval/src/forces/*.cpp` to `libs/physics/src/forces/`
- [x] 3.5 Move `naval/include/ymir/naval/NavalForceModel.h` and `naval/src/NavalForceModel.cpp` to `libs/physics/`
- [x] 3.6 Move `naval/src/naval.cpp` to `libs/physics/src/naval_physics.cpp`
- [x] 3.7 Update all `#include` paths in moved headers, source files, and test files

## Implementation Details

See TechSpec "File Mapping — core/ → libs/" and "File Mapping — naval/ → libs/" tables for the complete file-by-file mapping.

Key include updates in test files (see TechSpec "Impact Analysis"):
- `tests/core/TestBody.cpp`: `ymir/core/RigidBody6DOF.h` → `ymir/physics/RigidBody6DOF.h`
- `tests/core/TestForces.cpp`: `ymir/core/Forces.h` → `ymir/physics/Forces.h`
- `tests/naval/Test*Forces.cpp` (9 files): `ymir/naval/forces/...` → `ymir/physics/forces/...`

### Relevant Files

- `core/include/ymir/core/` — 8 headers to move (excluding math/ and Simulation.h)
- `core/src/` — 3 source files to move (excluding Simulation.cpp)
- `naval/include/ymir/naval/forces/` — 9 force headers to move
- `naval/src/forces/` — 9 force sources to move
- `naval/include/ymir/naval/NavalForceModel.h` + `naval/src/NavalForceModel.cpp` — move to physics
- `naval/src/naval.cpp` — move to `libs/physics/src/naval_physics.cpp`
- `tests/core/TestBody.cpp`, `TestForces.cpp`, `TestForceModel.cpp`, `TestIntegrator.cpp` — update includes
- `tests/naval/TestNavalInfra.cpp` + 9 force test files — update includes

### Dependent Files

- `naval/include/ymir/naval/NavalSimulation.h` — includes core types; update in task_04
- `naval/include/ymir/naval/wave/*.h` — include NavalContext which is in task_05/06; update there
- `core/include/ymir/core/Simulation.h` — includes core types; update in task_04

### Related ADRs

- [ADR-001: Atomic Split Reorganization](adrs/adr-001.md) — largest single move in the atomic split
- [ADR-002: New Include Namespaces](adrs/adr-002.md) — `ymir/core/` and `ymir/naval/forces/` → `ymir/physics/`
- [ADR-003: One CMake Target Per Bounded Context](adrs/adr-003.md) — all these files will compile into `ymir_physics`

## Deliverables

- All physics headers in `libs/physics/include/ymir/physics/` (8 core + 9 forces + 1 NavalForceModel + integrator/)
- All physics sources in `libs/physics/src/` (core + forces)
- Zero grep hits for `ymir/core/` (non-math) and `ymir/naval/forces/` across all files
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `grep -r "ymir/core/[^m]" . --include="*.h" --include="*.cpp"` returns zero results (math excluded)
  - [x] `grep -r "ymir/naval/forces" . --include="*.h" --include="*.cpp"` returns zero results
  - [x] All 9 force test files compile with updated includes
  - [x] `TestBody.cpp`, `TestForces.cpp`, `TestForceModel.cpp`, `TestIntegrator.cpp` compile
- Integration tests:
  - [x] `cmake --build build` succeeds
  - [x] `ctest --test-dir build` passes all core and naval force tests
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Zero grep hits for `ymir/core/` (excluding `ymir/core/math`) and `ymir/naval/forces/`
- All 9 force test executables pass
