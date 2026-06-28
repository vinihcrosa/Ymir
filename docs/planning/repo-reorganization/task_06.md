---
status: completed
title: Move `libs/world/` — wave engine + NavalEnvironment
type: refactor
complexity: medium
dependencies:
    - task_01
    - task_03
    - task_05
---

# Task 6: Move `libs/world/` — wave engine + NavalEnvironment

## Overview

Moves the wave engine and environment files from `naval/` into `libs/world/`, updating includes to the `ymir/world/` namespace. Depends on task_05 because wave headers reference `NavalContext.h` (now in `libs/vessel/`). Medium complexity: wave subdirectory has 3-4 headers plus implementation, and environment aggregator pulls in multiple physics types.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST move all `naval/include/ymir/naval/wave/*.h` to `libs/world/include/ymir/world/wave/`
- MUST move all `naval/src/wave/*.cpp` to `libs/world/src/wave/`
- MUST move `naval/include/ymir/naval/NavalEnvironment.h` → `libs/world/include/ymir/world/NavalEnvironment.h`
- MUST move `naval/src/NavalEnvironment.cpp` → `libs/world/src/NavalEnvironment.cpp`
- MUST update all `#include <ymir/naval/wave/...>` to `#include <ymir/world/wave/...>`
- MUST update all `#include <ymir/naval/NavalEnvironment.h>` to `#include <ymir/world/NavalEnvironment.h>`
- Wave headers include `NavalContext.h` — MUST reference the new `ymir/vessel/` path (task_05 complete)
- MUST NOT add CMakeLists.txt target yet (task_08)
</requirements>

## Subtasks

- [x] 6.1 Move all wave headers from `naval/include/ymir/naval/wave/` → `libs/world/include/ymir/world/wave/`
- [x] 6.2 Move all wave source files from `naval/src/wave/` → `libs/world/src/wave/`
- [x] 6.3 Move `naval/include/ymir/naval/NavalEnvironment.h` → `libs/world/include/ymir/world/NavalEnvironment.h`
- [ ] 6.4 Move `naval/src/NavalEnvironment.cpp` → `libs/world/src/NavalEnvironment.cpp`
- [x] 6.5 Update all `#include <ymir/naval/wave/...>` and `#include <ymir/naval/NavalEnvironment.h>` across all files

## Implementation Details

See TechSpec "File Mapping — naval/ → libs/" (wave and NavalEnvironment rows).

Wave headers internally include `NavalContext.h` — after task_05 these must reference `ymir/vessel/NavalContext.h`.
`NavalEnvironment.h` aggregates wave + physics types; its internal includes reference headers now in `ymir/physics/` and `ymir/world/wave/`.

### Relevant Files

- `naval/include/ymir/naval/wave/` — all headers; move to `libs/world/`
- `naval/src/wave/` — all sources; move to `libs/world/`
- `naval/include/ymir/naval/NavalEnvironment.h` — move to `libs/world/`
- `naval/src/NavalEnvironment.cpp` — move to `libs/world/`
- `tests/naval/TestWave*.cpp` — update includes for wave headers
- `tests/naval/TestNavalInfra.cpp` — may include `NavalEnvironment.h`; update

### Dependent Files

- `libs/simulation/include/ymir/simulation/NavalSimulation.h` (task_04) — may include `NavalEnvironment.h`; update if not already done in task_04
- `libs/persistence/` (task_07) — `ScenarioReader` may include `NavalEnvironment.h`; update there

### Related ADRs

- [ADR-002: New Include Namespaces](adrs/adr-002.md) — `ymir/naval/wave/` → `ymir/world/wave/`

## Deliverables

- All wave headers and sources in `libs/world/include/ymir/world/wave/` and `libs/world/src/wave/`
- `NavalEnvironment.h/.cpp` in `libs/world/`
- Zero grep hits for `ymir/naval/wave` and `ymir/naval/NavalEnvironment`
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [ ] `grep -r "ymir/naval/wave" . --include="*.h" --include="*.cpp"` returns zero results
  - [ ] `grep -r "ymir/naval/NavalEnvironment" . --include="*.h" --include="*.cpp"` returns zero results
  - [ ] All wave test files compile with updated includes
- Integration tests:
  - [ ] `cmake --build build` succeeds
  - [ ] `ctest --test-dir build` passes all wave-related tests
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Zero grep hits for `ymir/naval/wave` and `ymir/naval/NavalEnvironment`
