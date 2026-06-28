---
status: completed
title: Scaffold `libs/`, `apps/`, `services/` directory structure
type: infra
complexity: low
dependencies: []
---

# Task 1: Scaffold `libs/`, `apps/`, `services/` directory structure

## Overview

Creates the target directory skeleton defined in the TechSpec before any files are moved. Every subsequent task depends on these directories existing. The scaffold uses `.gitkeep` files so empty directories are tracked by git and placeholder `CMakeLists.txt` files so CMake can reference subdirectories immediately.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST create `libs/common/`, `libs/physics/`, `libs/simulation/`, `libs/world/`, `libs/vessel/`, `libs/persistence/` with `include/ymir/<context>/` and `src/` subdirectories each
- MUST create `apps/server/` and `apps/fast-time/` with placeholder `CMakeLists.txt` and `.gitkeep`
- MUST create `services/` with `.gitkeep`
- MUST NOT move any source files in this task
- MUST NOT modify any existing `CMakeLists.txt` files in this task
</requirements>

## Subtasks

- [x] 1.1 Create `libs/<context>/include/ymir/<context>/` and `libs/<context>/src/` for all six contexts
- [x] 1.2 Add `.gitkeep` to every new empty directory
- [x] 1.3 Create stub `libs/<context>/CMakeLists.txt` (empty or with a TODO comment) for each lib
- [x] 1.4 Create `apps/server/` and `apps/fast-time/` with placeholder `CMakeLists.txt` and `.gitkeep`
- [x] 1.5 Create `services/.gitkeep`

## Implementation Details

See TechSpec "System Architecture — Component Overview" for the full target directory tree.

Each `libs/<context>/CMakeLists.txt` is a stub at this stage — it will be populated in task_08.

### Relevant Files

- `CMakeLists.txt` (root) — will reference new subdirectories in task_08; do not modify yet
- `applications/CMakeLists.txt` — source for `apps/server/CMakeLists.txt` content in task_08

### Dependent Files

- All subsequent tasks (02–09) depend on this directory structure existing

### Related ADRs

- [ADR-001: Atomic Split Reorganization](adrs/adr-001.md) — this task is the first step of the atomic split
- [ADR-003: One CMake Target Per Bounded Context](adrs/adr-003.md) — directory names established here must match target names

## Deliverables

- Six `libs/<context>/` directory trees with `include/ymir/<context>/`, `src/`, `.gitkeep`, and stub `CMakeLists.txt`
- `apps/server/` and `apps/fast-time/` with `.gitkeep` and placeholder `CMakeLists.txt`
- `services/.gitkeep`
- Verification that existing build still compiles (no files moved yet)

## Tests

- Unit tests:
  - [x] `ls libs/` lists exactly: `common`, `physics`, `simulation`, `world`, `vessel`, `persistence`
  - [x] Each `libs/<context>/` contains `include/ymir/<context>/`, `src/`, `CMakeLists.txt`
  - [x] `ls apps/` lists `server` and `fast-time`, each containing a `CMakeLists.txt`
  - [x] `ls services/` shows `.gitkeep`
- Integration tests:
  - [x] `cmake -B build` still succeeds (existing `core/`, `naval/`, `adapters/` untouched)
  - [x] `ctest --test-dir build` passes all existing tests (72/72)
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `cmake -B build && cmake --build build && ctest --test-dir build` exits 0
- No existing source files were moved or modified
