---
status: completed
title: Move `libs/common/` — math utilities + PhysicalConstants
type: refactor
complexity: medium
dependencies:
  - task_01
---

# Task 2: Move `libs/common/` — math utilities + PhysicalConstants

## Overview

Moves the math utility headers and `PhysicalConstants.h` from their current locations into `libs/common/`, updating all `#include` paths to use the new `ymir/common/` namespace. This is the first file move and establishes the shared foundation that `libs/physics/` depends on.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST move all three math headers from `core/include/ymir/core/math/` to `libs/common/include/ymir/common/math/`
- MUST move `naval/include/ymir/naval/PhysicalConstants.h` to `libs/common/include/ymir/common/PhysicalConstants.h`
- MUST update every `#include <ymir/core/math/...>` to `#include <ymir/common/math/...>` across all source and test files
- MUST update every `#include <ymir/naval/PhysicalConstants.h>` to `#include <ymir/common/PhysicalConstants.h>`
- MUST NOT add a `libs/common/CMakeLists.txt` target yet — that is task_08
- MUST NOT move any other files in this task
</requirements>

## Subtasks

- [x] 2.1 Move `core/include/ymir/core/math/LinearAlgebra.h` → `libs/common/include/ymir/common/math/LinearAlgebra.h`
- [x] 2.2 Move `core/include/ymir/core/math/AngleUtils.h` → `libs/common/include/ymir/common/math/AngleUtils.h`
- [x] 2.3 Move `core/include/ymir/core/math/Interpolation.h` → `libs/common/include/ymir/common/math/Interpolation.h`
- [x] 2.4 Move `naval/include/ymir/naval/PhysicalConstants.h` → `libs/common/include/ymir/common/PhysicalConstants.h`
- [x] 2.5 Update all `#include` paths referencing moved headers (see Relevant Files)
- [x] 2.6 Verify `grep -r "ymir/core/math" .` returns zero results

## Implementation Details

See TechSpec "File Mapping — core/ → libs/" and "File Mapping — naval/ → libs/" tables for the exact source → destination mappings and new include paths.

At this stage `libs/common/` has no `CMakeLists.txt` target. Headers are physically present but not yet compiled by any CMake target (math utilities are header-only).

### Relevant Files

- `core/include/ymir/core/math/LinearAlgebra.h` — source; move to `libs/common/`
- `core/include/ymir/core/math/AngleUtils.h` — source; move to `libs/common/`
- `core/include/ymir/core/math/Interpolation.h` — source; move to `libs/common/`
- `naval/include/ymir/naval/PhysicalConstants.h` — source; move to `libs/common/`
- `tests/core/TestMath.cpp` — includes all three math headers; update includes

### Dependent Files

- `core/include/ymir/core/RigidBody6DOF.h` — likely includes math headers; update in task_03
- `naval/include/ymir/naval/forces/*.h` — may include `PhysicalConstants.h`; update in task_03
- `naval/src/forces/*.cpp` — may include `PhysicalConstants.h`; update in task_03

### Related ADRs

- [ADR-002: New Include Namespaces](adrs/adr-002.md) — include prefix changes from `ymir/core/math/` to `ymir/common/math/`

## Deliverables

- Four headers in `libs/common/include/ymir/common/` (three math + PhysicalConstants)
- All `#include <ymir/core/math/...>` updated to `ymir/common/math/...`
- All `#include <ymir/naval/PhysicalConstants.h>` updated to `ymir/common/PhysicalConstants.h`
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `grep -r "ymir/core/math" . --include="*.h" --include="*.cpp"` returns zero results
  - [x] `grep -r "ymir/naval/PhysicalConstants" . --include="*.h" --include="*.cpp"` returns zero results
  - [x] `tests/core/TestMath.cpp` compiles with updated includes
- Integration tests:
  - [x] `cmake --build build` succeeds after include updates (existing `core/CMakeLists.txt` still active)
  - [x] `ctest --test-dir build` passes `TestMath` tests
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Zero grep hits for `ymir/core/math` and `ymir/naval/PhysicalConstants` across all source files
- Build and tests pass
