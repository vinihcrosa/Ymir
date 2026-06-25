---
status: completed
title: Move `libs/persistence/` — JSON scenario reader
type: refactor
complexity: medium
dependencies:
  - task_01
  - task_03
  - task_05
  - task_06
---

# Task 7: Move `libs/persistence/` — JSON scenario reader

## Overview

Moves the JSON scenario reader (currently in `adapters/`) into `libs/persistence/`, updating all include paths. Medium complexity because `ScenarioReader` touches types from multiple bounded contexts (physics, vessel, world, simulation) and all those namespaces must be settled before this move.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST move `adapters/include/ymir/adapters/ScenarioReader.h` → `libs/persistence/include/ymir/persistence/ScenarioReader.h`
- MUST move `adapters/src/ScenarioReader.cpp` → `libs/persistence/src/ScenarioReader.cpp`
- MUST update all `#include <ymir/adapters/ScenarioReader.h>` to `#include <ymir/persistence/ScenarioReader.h>`
- All internal includes inside `ScenarioReader.h/.cpp` must use finalized namespaces: `ymir/physics/`, `ymir/vessel/`, `ymir/world/`, `ymir/simulation/`
- MUST NOT add CMakeLists.txt target yet (task_08)
</requirements>

## Subtasks

- [x] 7.1 Move `adapters/include/ymir/adapters/ScenarioReader.h` → `libs/persistence/include/ymir/persistence/ScenarioReader.h`
- [x] 7.2 Move `adapters/src/ScenarioReader.cpp` → `libs/persistence/src/ScenarioReader.cpp`
- [x] 7.3 Update all internal includes inside `ScenarioReader.h` and `ScenarioReader.cpp` to new namespace paths
- [x] 7.4 Update all files that included `ymir/adapters/ScenarioReader.h`

## Implementation Details

See TechSpec "File Mapping — adapters/ → libs/" for the exact source → destination mapping.

`ScenarioReader` depends on the complete set of bounded-context types. Verify that tasks 03–06 have settled all referenced namespaces before starting this task.

`applications/` (future `apps/server/`) likely includes `ScenarioReader.h` — update in this task since `apps/` is not restructured until task_08.

### Relevant Files

- `adapters/include/ymir/adapters/ScenarioReader.h` — move to `libs/persistence/`
- `adapters/src/ScenarioReader.cpp` — move to `libs/persistence/src/`
- `applications/` main source (if it includes `ScenarioReader.h`) — update include
- `tests/adapters/TestScenarioReader.cpp` (if it exists) — update includes

### Dependent Files

- `apps/server/` (task_08) — will need `ymir/persistence/ScenarioReader.h` in final CMakeLists

### Related ADRs

- [ADR-002: New Include Namespaces](adrs/adr-002.md) — `ymir/adapters/` → `ymir/persistence/`

## Deliverables

- `ScenarioReader.h/.cpp` in `libs/persistence/`
- All `#include <ymir/adapters/ScenarioReader.h>` updated
- Internal includes using new bounded-context namespace paths
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `grep -r "ymir/adapters" . --include="*.h" --include="*.cpp"` returns zero results
  - [x] `ScenarioReader.h` includes only `ymir/physics/`, `ymir/vessel/`, `ymir/world/`, `ymir/simulation/` namespaces
- Integration tests:
  - [x] `cmake --build build` succeeds
  - [x] `ctest --test-dir build` passes adapter/persistence tests
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Zero grep hits for `ymir/adapters`
