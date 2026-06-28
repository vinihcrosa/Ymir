---
status: completed
title: Rewrite README.md
type: docs
complexity: low
dependencies:
    - task_11
    - task_13
---

# Task 14: Rewrite README.md

## Overview

Rewrites the root `README.md` to reflect the new repository structure, introduce the bounded-context architecture, and link to the `docs/architecture/` wiki and `AGENTS.md`. Low complexity: content is a synthesis of already-written documents — no new design decisions required.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST update the project description to match the PRD (6-DOF naval physics simulator)
- MUST include a repo structure section showing `libs/`, `apps/`, `docs/`, `tests/` layout
- MUST list all six bounded contexts with one-line descriptions
- MUST include build instructions: prerequisites (C++17, CMake, SUNDIALS), cmake commands
- MUST link to `docs/architecture/README.md` for architecture detail
- MUST link to `AGENTS.md` for contributor rules
- MUST NOT duplicate content already in `AGENTS.md` or `docs/architecture/`
</requirements>

## Subtasks

- [ ] 14.1 Read current `README.md` in full
- [ ] 14.2 Write updated project description and goals (1-2 sentences from PRD)
- [ ] 14.3 Write repo structure section with the new `libs/`/`apps/`/`docs/`/`tests/` tree
- [ ] 14.4 Write bounded contexts table (name, target, one-line description, include prefix)
- [ ] 14.5 Write build prerequisites and CMake commands section
- [ ] 14.6 Add links to `docs/architecture/README.md` and `AGENTS.md`

## Implementation Details

Read current `README.md` before writing. Pull the bounded-context table from `docs/architecture/bounded-contexts.md` (task_11) — do not invent new descriptions.

Build prerequisites:
- C++17 compiler (GCC 9+ or Clang 10+)
- CMake 3.20+
- SUNDIALS 6.x (`SUNDIALS::cvode`)
- Catch2 3.x (tests)
- nlohmann/json (persistence)

Build commands:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

### Relevant Files

- `README.md` — primary file to rewrite
- `docs/architecture/bounded-contexts.md` (task_11) — source for bounded context table
- `AGENTS.md` (task_13) — link target for contributor rules

### Dependent Files

None — README is a leaf in the dependency graph.

### Related ADRs

None

## Deliverables

- Rewritten `README.md` with new structure
- Bounded contexts table with all six entries
- Build instructions with prerequisites and CMake commands
- Links to `docs/architecture/` and `AGENTS.md`
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [ ] `grep -n "core/\|naval/\|adapters/" README.md` returns zero results (as directory paths)
  - [ ] `grep -n "docs/architecture/README.md" README.md` returns at least one result
  - [ ] `grep -n "AGENTS.md" README.md` returns at least one result
  - [ ] `grep -n "libs/physics\|libs/common\|libs/simulation\|libs/world\|libs/vessel\|libs/persistence" README.md` — all six present
- Integration tests:
  - [ ] Markdown renders without broken links relative to repo root
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `README.md` has zero references to old directory layout
- All six bounded contexts listed with include prefix and CMake target
- Build instructions present and accurate
