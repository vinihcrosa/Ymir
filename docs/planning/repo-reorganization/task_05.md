---
status: completed
title: Move `libs/vessel/` — VesselConfig + NavalContext
type: refactor
complexity: low
dependencies:
  - task_01
  - task_03
---

# Task 5: Move `libs/vessel/` — VesselConfig + NavalContext

## Overview

Moves vessel-specific configuration headers from `naval/` into `libs/vessel/`, establishing the Vessel bounded context. These two headers are consumed by both the physics force models and the world wave engine, so they must be in place before task_06 (world) runs.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST move `naval/include/ymir/naval/VesselConfig.h` → `libs/vessel/include/ymir/vessel/VesselConfig.h`
- MUST move `naval/include/ymir/naval/NavalContext.h` → `libs/vessel/include/ymir/vessel/NavalContext.h`
- MUST update all `#include <ymir/naval/VesselConfig.h>` to `#include <ymir/vessel/VesselConfig.h>`
- MUST update all `#include <ymir/naval/NavalContext.h>` to `#include <ymir/vessel/NavalContext.h>`
- MUST NOT add CMakeLists.txt target yet (task_08)
- MUST NOT move any wave or environment files (task_06)
</requirements>

## Subtasks

- [x] 5.1 Move `naval/include/ymir/naval/VesselConfig.h` → `libs/vessel/include/ymir/vessel/VesselConfig.h`
- [x] 5.2 Move `naval/include/ymir/naval/NavalContext.h` → `libs/vessel/include/ymir/vessel/NavalContext.h`
- [x] 5.3 Update all `#include <ymir/naval/VesselConfig.h>` and `#include <ymir/naval/NavalContext.h>` across all files
- [x] 5.4 Verify `grep -r "ymir/naval/VesselConfig\|ymir/naval/NavalContext" .` returns zero results

## Implementation Details

See TechSpec "File Mapping — naval/ → libs/" (VesselConfig and NavalContext rows).

`NavalContext.h` is included by every force test file (`tests/naval/Test*Forces.cpp`) as it holds the vessel state struct consumed by force models. All nine force test files must have their includes updated.

`VesselConfig.h` is included by `NavalContext.h` or `NavalSimulation.h` — check internal dependencies and update cross-includes inside the moved headers.

### Relevant Files

- `naval/include/ymir/naval/NavalContext.h` — move to `libs/vessel/`
- `naval/include/ymir/naval/VesselConfig.h` — move to `libs/vessel/`
- `tests/naval/Test*Forces.cpp` (9 files) — all include `NavalContext.h`; update
- `tests/naval/TestNavalInfra.cpp` — includes `NavalContext.h`; update (also in task_04)
- `libs/physics/include/ymir/physics/forces/*.h` — moved in task_03; may include `NavalContext.h`; update

### Dependent Files

- `libs/world/include/ymir/world/wave/*.h` (task_06) — wave headers include `NavalContext.h`; will use new path
- `libs/simulation/include/ymir/simulation/NavalSimulation.h` (task_04) — may include `NavalContext.h`

### Related ADRs

- [ADR-002: New Include Namespaces](adrs/adr-002.md) — `ymir/naval/VesselConfig.h` and `ymir/naval/NavalContext.h` → `ymir/vessel/`

## Deliverables

- `VesselConfig.h` and `NavalContext.h` in `libs/vessel/include/ymir/vessel/`
- All `#include <ymir/naval/NavalContext.h>` and `#include <ymir/naval/VesselConfig.h>` updated
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `grep -r "ymir/naval/NavalContext" . --include="*.h" --include="*.cpp"` returns zero results
  - [x] `grep -r "ymir/naval/VesselConfig" . --include="*.h" --include="*.cpp"` returns zero results
  - [x] All 9 force test files still compile after include update
- Integration tests:
  - [x] `cmake --build build` succeeds
  - [x] `ctest --test-dir build` passes all naval force tests
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Zero grep hits for `ymir/naval/NavalContext` and `ymir/naval/VesselConfig`
