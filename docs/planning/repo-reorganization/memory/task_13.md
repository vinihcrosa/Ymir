# Task Memory: task_13.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Rewrite AGENTS.md to reflect bounded-context structure: update dir refs, include namespaces, add Bounded Contexts section, link docs/architecture/ wiki, preserve all coding/commit/testing rules.

## Important Decisions

- The task spec test `grep "ymir/core\|ymir/naval\|ymir/adapters" AGENTS.md` must return zero. A "never use X" line naming the old prefixes fails the test. Solution: removed that explicit prohibition line; the bounded-context table makes the correct prefixes self-evident.
- Old commit scopes (`core`, `naval`, `adapters`) replaced with new bounded-context scopes (`common`, `physics`, `simulation`, `world`, `vessel`, `persistence`).
- Test file path example updated: `tests/core/forces/TestCurrentForces.cpp` → `tests/physics/forces/TestCurrentForces.cpp`.

## Learnings

- task_11 (docs/architecture/) was completed before this task — both `README.md` and `bounded-contexts.md` exist and links are valid from repo root.

## Files / Surfaces

- `AGENTS.md` — fully rewritten

## Errors / Corrections

- None

## Ready for Next Run

Task complete. No commit made (--auto-commit=false). diff staged and ready for manual review.
