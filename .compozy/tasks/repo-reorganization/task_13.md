---
status: completed
title: Rewrite AGENTS.md
type: docs
complexity: medium
dependencies:
  - task_11
---

# Task 13: Rewrite AGENTS.md

## Overview

Rewrites `AGENTS.md` to reflect the new bounded-context structure, updated folder layout, new include namespaces, and links to the `docs/architecture/` wiki. The current `AGENTS.md` references the old `core/`/`naval/`/`adapters/` layout — all those references become stale after the reorganization. Medium complexity: requires thorough review of existing content and accurate mapping to the new structure.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST update all directory references from `core/`, `naval/`, `adapters/`, `applications/` to `libs/<context>/`, `apps/`
- MUST update all `#include` examples to use new `ymir/<context>/` namespaces
- MUST add a Bounded Contexts section describing each lib's responsibility and CMake target name
- MUST add links to `docs/architecture/README.md` and `docs/architecture/bounded-contexts.md`
- MUST preserve all coding conventions, commit standards, and testing rules from the existing `AGENTS.md`
- MUST NOT remove the SUNDIALS CVODE, Catch2, or other tool/library references
</requirements>

## Subtasks

- [x] 13.1 Read current `AGENTS.md` in full
- [x] 13.2 Update all folder path references to new bounded-context layout
- [x] 13.3 Update all `#include` examples to new namespace prefixes
- [x] 13.4 Add Bounded Contexts section with lib names, targets, and include prefixes
- [x] 13.5 Add links to `docs/architecture/` wiki pages
- [x] 13.6 Verify all preserved rules (commits, testing, coding conventions) remain intact

## Implementation Details

Read the current `AGENTS.md` before writing. Preserve section headings unless they reference the old layout. Do not add new rules not present in PRD or TechSpec — this is a documentation update, not a rule expansion.

Reference `docs/architecture/bounded-contexts.md` (task_11) for the authoritative bounded context table.

### Relevant Files

- `AGENTS.md` — primary file to rewrite
- `docs/architecture/bounded-contexts.md` (task_11) — source for bounded context descriptions
- `docs/architecture/README.md` (task_11) — link target

### Dependent Files

- `README.md` (task_14) — references `AGENTS.md` for contributor rules

### Related ADRs

- [ADR-002: New Include Namespaces](adrs/adr-002.md) — namespace prefix changes documented here

## Deliverables

- Rewritten `AGENTS.md` with no references to old directory layout
- Bounded Contexts section with 6 entries
- Links to `docs/architecture/` wiki
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `grep -n "ymir/core\|ymir/naval\|ymir/adapters" AGENTS.md` returns zero results (non-wiki prose)
  - [x] `grep -n "docs/architecture/bounded-contexts.md" AGENTS.md` returns at least one result
  - [x] `grep -n "libs/physics\|libs/common\|libs/simulation\|libs/world\|libs/vessel\|libs/persistence" AGENTS.md` returns results for all six libs
- Integration tests:
  - [x] Markdown renders without broken links relative to repo root
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `AGENTS.md` contains zero references to `core/`, `naval/`, `adapters/` as directory paths
- All six bounded contexts documented in AGENTS.md
