---
status: completed
title: Reorganize tests — move files + rewrite tests/CMakeLists.txt
type: refactor
complexity: high
dependencies:
  - task_08
---

# Task 9: Reorganize tests — move files + rewrite tests/CMakeLists.txt

## Overview

Moves all test files from `tests/core/` and `tests/naval/` into `tests/<context>/` directories matching the new bounded contexts, and rewrites `tests/CMakeLists.txt` to link each test executable against the new `Ymir::*` targets. High complexity because it touches all test files and rebuilds the test cmake structure.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST move `tests/core/*.cpp` to appropriate `tests/<context>/` per the TechSpec test reorganization table
- MUST move `tests/naval/*.cpp` to appropriate `tests/<context>/` directories
- MUST rewrite `tests/CMakeLists.txt` to define one test executable per bounded context (or one per test file per TechSpec guidance) linking against `Ymir::*` targets
- MUST NOT use the old `core` or `naval` CMake targets in any test `target_link_libraries`
- All moved test files MUST have their `#include` paths already updated (tasks 02–07 preconditions)
</requirements>

## Subtasks

- [x] 9.1 Move `tests/core/TestMath.cpp` → `tests/common/TestMath.cpp`
- [x] 9.2 Move `tests/core/TestBody.cpp`, `TestForces.cpp`, `TestForceModel.cpp`, `TestIntegrator.cpp` → `tests/physics/`
- [x] 9.3 Move `tests/core/TestSimulationIntegration.cpp` → `tests/simulation/`
- [x] 9.4 Move all `tests/naval/Test*Forces.cpp` (9 files) → `tests/physics/forces/`
- [x] 9.5 Move `tests/naval/TestWave*.cpp` → `tests/world/`
- [x] 9.6 Move `tests/naval/TestNavalInfra.cpp` and `TestNavalIntegration.cpp` → `tests/simulation/`
- [x] 9.7 Rewrite `tests/CMakeLists.txt` with new test executables linking `Ymir::*` targets

## Implementation Details

See TechSpec "Test Reorganization Table" for the complete source → destination mapping.

Each new test executable must use `target_link_libraries` against the appropriate `Ymir::*` alias target plus `Catch2::Catch2WithMain`.

Remove `tests/core/` and `tests/naval/` directories after all files are moved.

### Relevant Files

- `tests/core/` — all test files; move per TechSpec table
- `tests/naval/` — all test files; move per TechSpec table
- `tests/CMakeLists.txt` — full rewrite required

### Dependent Files

- `task_10.md` — deletes old `tests/core/` and `tests/naval/` empty dirs
- Doxyfile (task_12) — test dirs not in INPUT but confirm after reorganization

### Related ADRs

- [ADR-003: One CMake Target Per Bounded Context](adrs/adr-003.md) — test executables mirror lib target structure

## Deliverables

- Test files in `tests/<context>/` directories matching bounded contexts
- `tests/CMakeLists.txt` rewritten to use `Ymir::*` targets
- `tests/core/` and `tests/naval/` empty and ready for deletion (task_10)
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `ls tests/core/` and `ls tests/naval/` — both directories empty
  - [x] Each moved test file compiles in its new location
- Integration tests:
  - [x] `cmake --build build` succeeds with new test structure
  - [x] `ctest --test-dir build` passes all tests
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `ctest --test-dir build` exits 0 with all tests listed under new bounded-context names
- No CMake test target links old `core` or `naval` library
