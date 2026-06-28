---
status: completed
title: Create docs/architecture/ wiki
type: docs
complexity: medium
dependencies: []
---

# Task 11: Create docs/architecture/ wiki

## Overview

Creates the `docs/architecture/` directory and writes the initial architecture wiki pages: an overview page, a bounded-contexts page, and a data-flow page. No implementation dependency — can run in parallel with any task. Content is derived from the PRD and TechSpec, not from code.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST create `docs/architecture/` directory
- MUST create `docs/architecture/README.md` — entry point, overview of simulator architecture, links to sub-pages
- MUST create `docs/architecture/bounded-contexts.md` — one section per bounded context (common, physics, simulation, world, vessel, persistence) with responsibilities and dependencies
- MUST create `docs/architecture/data-flow.md` — simulation tick sequence (12-step order from PRD), force execution order, CVODE integration loop
- Content MUST be consistent with `.prds/prd.md` and `.prds/modules/*.md`
- MUST NOT contain implementation details not yet in the TechSpec or PRD
</requirements>

## Subtasks

- [x] 11.1 Create `docs/architecture/` directory
- [x] 11.2 Write `docs/architecture/README.md` — simulator overview and link table
- [x] 11.3 Write `docs/architecture/bounded-contexts.md` — one section per context with responsibilities and CMake target name
- [x] 11.4 Write `docs/architecture/data-flow.md` — tick sequence, force execution order, integrator loop

## Implementation Details

Reference `.prds/prd.md` (bounded context definitions), `.prds/modules/physics.md` (force execution order, 12-step tick), and `.prds/modules/simulation.md` (tick sequence) for content.

`bounded-contexts.md` should note the CMake target name (`ymir_common`, etc.) and the include namespace prefix (`ymir/common/`, etc.) for each context so developers can orient quickly.

`data-flow.md` should include the 12-step tick order and the CVODE adaptive sub-step loop description from the simulation module spec.

### Relevant Files

- `.prds/prd.md` — primary source for bounded context definitions and goals
- `.prds/modules/physics.md` — force execution order, equation of motion, CVODE details
- `.prds/modules/simulation.md` — 12-step tick order, event table
- `.prds/modules/world.md` — wave engine, JONSWAP/Pierson spectra
- `.prds/modules/vessel.md` — maneuver modes, actuator models

### Dependent Files

- `AGENTS.md` (task_13) — will reference `docs/architecture/` wiki
- `README.md` (task_14) — will link to `docs/architecture/README.md`
- Doxyfile (task_12) — may reference `docs/` for mainpage

### Related ADRs

None

## Deliverables

- `docs/architecture/README.md`
- `docs/architecture/bounded-contexts.md`
- `docs/architecture/data-flow.md`
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for build verification **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `ls docs/architecture/` lists `README.md`, `bounded-contexts.md`, `data-flow.md`
  - [x] Each file contains at least one section per bounded context (6 contexts)
  - [x] `bounded-contexts.md` mentions all six CMake target names
- Integration tests:
  - [x] Markdown renders without broken links (check relative links manually or with a linter)
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Three wiki files created in `docs/architecture/`
- Content consistent with PRD and module specs (no contradictions)
