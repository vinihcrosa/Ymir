---
status: completed
title: Update Doxyfile + wire docs CMake target
type: infra
complexity: low
dependencies:
    - task_08
---

# Task 12: Update Doxyfile + wire docs CMake target

## Overview

Updates the existing Doxyfile (or creates one if absent) so that Doxygen scans the new `libs/` directory structure instead of `core/` and `naval/`. Adds a `docs` CMake custom target to the root CMakeLists.txt. Low complexity — mechanical path updates plus one CMake custom_target.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST update (or create) `Doxyfile` with `INPUT = libs/ apps/ docs/architecture/`
- MUST remove any Doxyfile `INPUT` references to `core/`, `naval/`, `adapters/`, `applications/`
- MUST set `RECURSIVE = YES` in Doxyfile
- MUST add `cmake --build build --target docs` custom target to root `CMakeLists.txt` using `find_package(Doxygen OPTIONAL_COMPONENTS dot)`
- MUST NOT block the default build if Doxygen is not installed (use `OPTIONAL` or guard with `if(DOXYGEN_FOUND)`)
</requirements>

## Subtasks

- [x] 12.1 Read existing `Doxyfile` (or create from `doxygen -g` template if absent)
- [x] 12.2 Update `INPUT` to `libs/ apps/ docs/architecture/`
- [x] 12.3 Remove old `INPUT` paths (`core/`, `naval/`, `adapters/`, `applications/`)
- [x] 12.4 Set `RECURSIVE = YES`, `OUTPUT_DIRECTORY = docs/api`
- [x] 12.5 Add `docs` custom target to root `CMakeLists.txt` guarded with `if(DOXYGEN_FOUND)`

## Implementation Details

See TechSpec "Doxyfile INPUT Update" section for exact INPUT configuration.

If no Doxyfile exists at repo root, generate a minimal one with `doxygen -g Doxyfile` and update the key fields. Do not commit a 3000-line default Doxyfile — strip to only the fields that differ from defaults.

Doxyfile key fields:
```
PROJECT_NAME = Ymir
INPUT = libs/ apps/ docs/architecture/
RECURSIVE = YES
OUTPUT_DIRECTORY = docs/api
GENERATE_HTML = YES
GENERATE_LATEX = NO
```

### Relevant Files

- `Doxyfile` (root) — update or create
- `CMakeLists.txt` (root) — add `docs` custom target

### Dependent Files

- `docs/architecture/` (task_11) — Doxyfile INPUT includes this directory

### Related ADRs

None

## Deliverables

- Updated `Doxyfile` with correct `INPUT` paths
- `docs` CMake custom target in root `CMakeLists.txt`
- `cmake --build build --target docs` generates HTML in `docs/api/`
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `grep "INPUT" Doxyfile` shows `libs/ apps/ docs/architecture/` and no legacy dirs
  - [x] `grep "core\|naval\|adapters\|applications" Doxyfile` returns zero results in INPUT line
- Integration tests:
  - [x] `cmake -B build && cmake --build build --target docs` exits 0 when Doxygen is installed
  - [x] `docs/api/html/index.html` is created
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `cmake --build build --target docs` exits 0
- Doxyfile INPUT references only new bounded-context paths
