# PRD: Ymir Repository Reorganization

## Overview

The Ymir codebase was built during early prototyping with a directory structure (`core/`, `naval/`, `adapters/`) that no longer reflects the bounded-context architecture defined in the project's PRD. This creates friction for developers and AI agents navigating the code, as folder names do not match the conceptual model.

This effort reorganizes the repository into its target structure, deletes legacy reference material that has been superseded, and establishes a living documentation layer. No new functionality is introduced. The goal is to clear the path for future implementation phases.

**Who it is for**: developers working on Ymir, AI agents assisting development, and future contributors onboarding to the project.

---

## Goals

- Repository structure matches the bounded-context architecture in `.prds/prd.md` exactly
- Every folder name maps unambiguously to a single module or context
- Developers can determine where to add new code without consulting external documents
- AI agents can read `docs/` and `AGENTS.md` to understand the codebase structure and contribute accurately
- The CMake build passes all tests after reorganization
- Doxygen generates API documentation from a single `make docs` command

---

## User Stories

**As a developer adding a new force model**, I want to find the correct directory immediately based on its name, so I do not have to guess whether it belongs in `core/` or `naval/`.

**As an AI agent starting a new session**, I want to read `docs/` and find a clear, up-to-date description of every module and its responsibilities, so I can contribute without reading all source files.

**As a developer onboarding to the project**, I want `README.md` and `AGENTS.md` to reflect the current structure with accurate file paths and conventions, so I can be productive without discovering outdated information.

**As a developer running the build**, I want `make docs` to produce API documentation from source, so I can browse generated docs instead of reading header files.

**As a developer reviewing a PR**, I want the module a change belongs to to be obvious from its path, so I can assess scope and impact at a glance.

---

## Core Features

### 1. Atomic File Reorganization

Move all source files from the legacy structure to their final bounded context locations under `libs/`.

Mapping:

| Source | Destination | Context |
|--------|-------------|---------|
| `core/` (bodies, forces, integrator) | `libs/physics/` | Physics |
| `core/Simulation.h/.cpp` | `libs/simulation/` | Simulation |
| `core/math/` | `libs/common/` | Common |
| `naval/forces/` | `libs/physics/` | Physics |
| `naval/wave/`, `NavalEnvironment.h` | `libs/world/` | World |
| `naval/VesselConfig.h`, `NavalContext.h` | `libs/vessel/` | Vessel |
| `adapters/` | `libs/persistence/` | Infrastructure |
| `applications/` | `apps/server/` | Infrastructure |

Create empty placeholder directories with `CMakeLists.txt` and `.gitkeep`:
- `apps/fast-time/`
- `services/`
- `libs/vessel/` (if no files map there yet, placeholder only)

Update all `#include` paths and `CMakeLists.txt` files to reflect the new layout. Run full build and all tests to verify nothing broke.

### 2. Legacy Documentation Removal

Delete `.docs/` in its entirety. Its content described the architecture of a previous project. All relevant domain knowledge has been absorbed and rewritten into `.prds/modules/`. Keeping it risks misleading future contributors about which documentation is authoritative.

### 3. Markdown Wiki in `docs/`

Create a human- and AI-readable wiki under `docs/` with the following structure:

```
docs/
├── README.md           — index of all docs
├── architecture/
│   ├── overview.md     — bounded contexts, module map, data flow
│   ├── physics.md      — from .prds/modules/physics.md
│   ├── world.md        — from .prds/modules/world.md
│   ├── vessel.md       — from .prds/modules/vessel.md
│   ├── simulation.md   — from .prds/modules/simulation.md
│   └── infrastructure.md — from .prds/modules/infrastructure.md
├── guides/
│   ├── building.md     — how to build, run tests, generate docs
│   └── contributing.md — coding standards, PR conventions
└── adr/                — Architecture Decision Records (already exists)
```

Content in `docs/architecture/` is the semantic wiki for AI agents and developers. It describes what each module does, its inputs/outputs, and how it fits in the system — not how to implement it.

### 4. Updated `AGENTS.md`

Rewrite `AGENTS.md` to reflect:
- New folder paths (`libs/physics/` instead of `core/`)
- Bounded context responsibilities (linking to `docs/architecture/`)
- Coding standards and conventions (unchanged, just with correct paths)
- Where AI agents should look for context before writing code

`AGENTS.md` is the single entry point for AI agents starting a session. It must stay accurate as the codebase evolves.

### 5. Updated `README.md`

Rewrite `README.md` to include:
- What Ymir is (one paragraph)
- Current project status
- Directory structure with one-line descriptions per folder
- How to build (prerequisites, CMake commands)
- How to run tests
- How to generate documentation
- Link to `docs/` for deeper context

### 6. Doxygen Wired into CMake

Enable `YMIR_BUILD_DOCS=ON` by default (or provide a clear `make docs` target). The existing `Doxyfile` must be updated to point to the new source paths under `libs/`. Running `make docs` from the build directory generates HTML API documentation into `docs/generated/` (gitignored).

---

## User Experience

The "user" here is a developer or AI agent opening this repository.

**Before**: Opening `core/` reveals a mix of physics primitives, the integrator, and simulation orchestration. `naval/` contains force implementations, wave models, and vessel config — no clear separation. Finding where to add a new wave model requires reading multiple files.

**After**: Opening `libs/` shows six folders, each named after a bounded context. Opening `libs/world/` contains everything related to environment and terrain. `docs/architecture/world.md` explains exactly what belongs there and why.

**Onboarding flow**:
1. Developer reads `README.md` → understands project, how to build
2. Developer reads `AGENTS.md` → understands architecture, coding standards, where to look
3. Developer opens `docs/architecture/` → finds module-level documentation
4. Developer opens `libs/<module>/` → finds code that matches the docs

---

## High-Level Technical Constraints

- The CMake build must pass all existing tests after reorganization — no regressions
- `#include` paths change from `ymir/core/` and `ymir/naval/` to the new module paths — all consumers must be updated atomically
- `git log --follow` is needed to trace file history across the move — document this in `AGENTS.md`
- The `Doxyfile` is updated to scan `libs/` instead of `core/` and `naval/`
- `.prds/` remains as the product requirements layer — `docs/` is the developer/agent wiki layer; they serve different purposes and both are kept

---

## Non-Goals

- No new C++ code is written
- No new features are implemented
- No changes to algorithms, physics models, or integration logic
- `services/` is created as a placeholder only — no implementation
- `apps/fast-time/` is created as a placeholder only — no implementation
- `libs/persistence/` receives the moved `adapters/` code — no new persistence logic
- No changes to `.github/workflows/ci.yml` beyond updating paths if needed
- No migration to C++ Modules (toolchain support is still inconsistent)
- No new test cases (existing tests move with their source)

---

## Phased Rollout Plan

### Phase 1 — Atomic Reorganization

Move all files, update all `#include` paths, update all `CMakeLists.txt`, verify build and tests pass. Single commit.

**Success criteria**: `cmake --build build && ctest --test-dir build` exits with zero errors.

### Phase 2 — Documentation Layer

Create `docs/architecture/` wiki, update `AGENTS.md`, update `README.md`, wire Doxygen into CMake.

**Success criteria**: A developer new to the project can understand the module structure, build the project, and run tests using only `README.md` and `docs/`.

### Phase 3 — Cleanup

Delete `.docs/`. Delete `include/` if it remains empty. Confirm `.gitignore` excludes `build/` and `docs/generated/`. Close this task.

**Success criteria**: No legacy artifacts remain. `git status` on a clean checkout shows only the intended structure.

---

## Success Metrics

- All existing tests pass after reorganization (zero regressions)
- `make docs` produces Doxygen output without errors
- `README.md`, `AGENTS.md`, and `docs/architecture/` are consistent with each other and with the actual folder structure
- No folder named `core/`, `naval/`, or `applications/` remains in the repository root
- A developer unfamiliar with the project can identify the correct `libs/` subdirectory for any new file in under 60 seconds

---

## Risks and Mitigations

| Risk | Likelihood | Mitigation |
|------|-----------|-----------|
| Missed `#include` path causes build failure | Medium | Run `grep -r "ymir/core\|ymir/naval"` after moving files; fix before committing |
| CI pipeline fails due to path changes | Low | Update `CMakeLists.txt` atomically with file moves; test locally first |
| `docs/` content diverges from code over time | Medium | `AGENTS.md` instructs agents to update `docs/architecture/` whenever a module changes |
| Merge conflicts if branches diverge during reorganization | Low | Reorganization happens on main in a single commit; no parallel feature branches active |

---

## Architecture Decision Records

- [ADR-001: Atomic Split Reorganization](adrs/adr-001.md) — Reorganize in a single commit rather than a phased rename, moving all files to final bounded context locations

---

## Open Questions

- Should `libs/vessel/` be populated now with the files from `naval/` that map to it (`VesselConfig.h`, `NavalContext.h`), or should it start as a placeholder pending the full Vessel module implementation? (Recommendation: move them — they belong there.)
- Should `NavalForceModel.h` and `NavalSimulation.h` be treated as transitional files (moved to `libs/physics/` and `libs/simulation/` respectively) or removed? (Needs inspection of what they contain vs. what `libs/physics/` and `libs/simulation/` already have.)
