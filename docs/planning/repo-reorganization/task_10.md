---
status: completed
title: Delete legacy dirs, update .gitignore, verify build
type: chore
complexity: medium
dependencies:
  - task_09
---

# Task 10: Delete legacy dirs, update .gitignore, verify build

## Overview

Removes the now-empty legacy directories (`core/`, `naval/`, `adapters/`, `applications/`, `tests/core/`, `tests/naval/`), updates `.gitignore` if needed, and runs a full clean build to confirm the reorganization is complete. This is the cleanup/verification step that closes the atomic split.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST verify each legacy directory is empty (no source files remain) before deleting it
- MUST delete: `core/`, `naval/`, `adapters/`, `applications/`, `tests/core/`, `tests/naval/`
- MUST run `grep -r "ymir/core\|ymir/naval\|ymir/adapters" . --include="*.h" --include="*.cpp"` and confirm zero results before deletion
- MUST run full clean build after deletion: `rm -rf build && cmake -B build && cmake --build build`
- MUST run `ctest --test-dir build` and confirm all tests pass
- MUST update `.gitignore` if any build artifact paths reference deleted directories
</requirements>

## Subtasks

- [x] 10.1 Run include-path grep to confirm zero legacy references across all source files
- [x] 10.2 Confirm each legacy directory contains no source files (only CMakeLists.txt stubs or `.gitkeep`)
- [x] 10.3 Delete `core/`, `naval/`, `adapters/`, `applications/` directories
- [x] 10.4 Delete `tests/core/` and `tests/naval/` directories
- [x] 10.5 Run full clean build: `rm -rf build && cmake -B build && cmake --build build && ctest --test-dir build`
- [x] 10.6 Update `.gitignore` if needed

## Implementation Details

See TechSpec "Verification Commands" section for the exact grep and build commands used to validate the reorganization.

Do NOT delete a directory if any `.h` or `.cpp` files remain — investigate why they were not moved and resolve before proceeding.

### Relevant Files

- `core/` — legacy dir; delete after confirming empty of source
- `naval/` — legacy dir; delete after confirming empty of source
- `adapters/` — legacy dir; delete after confirming empty of source
- `applications/` — legacy dir; delete after confirming empty of source
- `tests/core/` — legacy test dir; delete after confirming empty
- `tests/naval/` — legacy test dir; delete after confirming empty
- `.gitignore` — update if build path references removed dirs

### Dependent Files

None — this is the final cleanup step.

### Related ADRs

- [ADR-001: Atomic Split Reorganization](adrs/adr-001.md) — deletion of legacy dirs is the final step of the atomic split

## Deliverables

- All six legacy directories removed from repo
- Clean build passes (`cmake -B build && cmake --build build` exits 0)
- All tests pass (`ctest --test-dir build` exits 0)
- Zero grep hits for legacy include prefixes
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `grep -r "ymir/core\|ymir/naval\|ymir/adapters" . --include="*.h" --include="*.cpp"` returns zero results
  - [x] `ls core/ naval/ adapters/ applications/ tests/core/ tests/naval/` — all return "No such file or directory"
- Integration tests:
  - [x] `rm -rf build && cmake -B build && cmake --build build` exits 0
  - [x] `ctest --test-dir build` exits 0 with all tests passing
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `rm -rf build && cmake -B build && cmake --build build && ctest --test-dir build` exits 0
- No legacy directories present in repo
- Zero grep hits for old include namespaces
