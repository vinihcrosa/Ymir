---
status: completed
title: Move `libs/simulation/` — Simulation + NavalSimulation
type: refactor
complexity: low
dependencies:
    - task_01
    - task_03
---

# Task 4: Move `libs/simulation/` — Simulation + NavalSimulation

## Overview

Moves the simulation orchestration files from `core/` and `naval/` into `libs/simulation/`, updating includes to use the `ymir/simulation/` namespace. Small scope (2 headers, 2 source files) but depends on `libs/physics/` headers being in place first.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST move `core/include/ymir/core/Simulation.h` → `libs/simulation/include/ymir/simulation/Simulation.h`
- MUST move `core/src/Simulation.cpp` → `libs/simulation/src/Simulation.cpp`
- MUST move `naval/include/ymir/naval/NavalSimulation.h` → `libs/simulation/include/ymir/simulation/NavalSimulation.h`
- MUST move `naval/src/NavalSimulation.cpp` → `libs/simulation/src/NavalSimulation.cpp`
- MUST update all `#include <ymir/core/Simulation.h>` to `#include <ymir/simulation/Simulation.h>`
- MUST update all `#include <ymir/naval/NavalSimulation.h>` to `#include <ymir/simulation/NavalSimulation.h>`
- MUST NOT add CMakeLists.txt target yet (task_08)
</requirements>

## Subtasks

- [x] 4.1 Move `core/include/ymir/core/Simulation.h` → `libs/simulation/include/ymir/simulation/Simulation.h`
- [x] 4.2 Move `core/src/Simulation.cpp` → `libs/simulation/src/Simulation.cpp`
- [x] 4.3 Move `naval/include/ymir/naval/NavalSimulation.h` → `libs/simulation/include/ymir/simulation/NavalSimulation.h`
- [x] 4.4 Move `naval/src/NavalSimulation.cpp` → `libs/simulation/src/NavalSimulation.cpp`
- [x] 4.5 Update all include paths in moved files, test files, and any file that included them

## Implementation Details

See TechSpec "File Mapping — core/ → libs/" (Simulation rows) and "File Mapping — naval/ → libs/" (NavalSimulation row).

`Simulation.h` includes `RigidBody6DOF.h` (now `ymir/physics/`) — this internal include must also be updated.
`NavalSimulation.h` includes core and force headers (now `ymir/physics/`) — update accordingly.

### Relevant Files

- `core/include/ymir/core/Simulation.h` — move to `libs/simulation/`
- `core/src/Simulation.cpp` — move to `libs/simulation/src/`
- `naval/include/ymir/naval/NavalSimulation.h` — move to `libs/simulation/`
- `naval/src/NavalSimulation.cpp` — move to `libs/simulation/src/`
- `tests/core/TestIntegrator.cpp` — includes `Simulation.h`; update
- `tests/core/TestSimulationIntegration.cpp` — includes `Simulation.h`; update
- `tests/naval/TestNavalInfra.cpp` — includes `NavalSimulation.h`; update
- `tests/naval/TestNavalIntegration.cpp` — includes `NavalSimulation.h`; update

### Dependent Files

- `libs/persistence/` (task_07) — `ScenarioReader` may include `NavalSimulation.h`; update there

### Related ADRs

- [ADR-002: New Include Namespaces](adrs/adr-002.md) — `ymir/core/Simulation.h` and `ymir/naval/NavalSimulation.h` → `ymir/simulation/`

## Deliverables

- `Simulation.h/.cpp` and `NavalSimulation.h/.cpp` in `libs/simulation/`
- All includes updated to `ymir/simulation/`
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [ ] `grep -r "ymir/core/Simulation" . --include="*.h" --include="*.cpp"` returns zero results
  - [ ] `grep -r "ymir/naval/NavalSimulation" . --include="*.h" --include="*.cpp"` returns zero results
  - [ ] `TestIntegrator.cpp` and `TestSimulationIntegration.cpp` compile with updated includes
- Integration tests:
  - [ ] `cmake --build build` succeeds
  - [ ] `ctest --test-dir build` passes simulation-related tests
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Zero grep hits for `ymir/core/Simulation` and `ymir/naval/NavalSimulation`

## Verification Notes

- `grep -r "ymir/core/Simulation" . --include="*.h" --include="*.cpp"` returned no matches.
- `grep -r "ymir/naval/NavalSimulation" . --include="*.h" --include="*.cpp"` returned no matches.
- `cmake --build build` succeeded.
- `ctest --test-dir build` passed 72/72 tests.
- Coverage could not be measured in the current build setup: no coverage flags/data files are present, and `cmake --build build --target ExperimentalCoverage` fails because `build/tests/CMakeFiles/CTestScript.cmake` is missing.
