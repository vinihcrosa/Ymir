# Task Memory: task_14.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Rewrite `README.md` to reflect new `libs/`-based repo layout, list all six bounded contexts, include build instructions, and link to `docs/architecture/README.md` and `AGENTS.md`.

## Important Decisions

- Kept README minimal (73 lines) — no duplication of AGENTS.md content or docs/architecture detail
- Used one-line descriptions per context inline rather than a full table column (readability)
- Linked to `docs/architecture/bounded-contexts.md` for full dependency detail

## Learnings

- All six contexts (common, physics, simulation, world, vessel, persistence) pull from `bounded-contexts.md` which was written in task_11
- AGENTS.md (task_13) already has the full bounded-context table; README must not repeat it

## Files / Surfaces

- `README.md` — rewritten

## Errors / Corrections

None.

## Ready for Next Run

Task complete. All test assertions pass. No commit made (auto-commit disabled).
