# Task Memory: task_03.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Implement `CouplingRegistry` in `libs/simulation/` — Jacobi force coupling mediator.
Status: **COMPLETE**. All tests pass.

## Important Decisions

- `resolve()` uses two-pass approach: pass 1 resets consumer slots with ready producers, pass 2 accumulates. This correctly handles both single and multiple producers per consumer.
- `resolved_` map is pre-populated per consumer in `addLink()` via `resolved_.emplace(consumerBodyId, Forces::zero())`. Subsequent `resolve()` calls use `at()` (no allocation).
- `assert(false)` in `writeForce()` for unknown producer — fires after linear scan finds no match.
- Assertion tests (duplicate link, unknown producer) are not automatable in Catch2 without death tests — documented in task spec, skipped in test file.

## Learnings

- `resolved_.emplace(id, zero)` is a no-op if key already exists — safe to call from `addLink()` even if consumer appears in multiple links (e.g., two producers → one consumer).
- Two-pass resolve semantics: consumers with no ready producer this tick retain their previous resolved value unchanged (neither pass touches them).

## Files / Surfaces

- `libs/simulation/include/ymir/simulation/CouplingRegistry.h` — new
- `libs/simulation/src/CouplingRegistry.cpp` — new
- `libs/simulation/CMakeLists.txt` — added `CouplingRegistry.cpp` source, `ymir_world` to `target_link_libraries`
- `tests/simulation/TestCouplingRegistry.cpp` — new, 9 tests / 55 assertions
- `tests/CMakeLists.txt` — added `TestCouplingRegistry.cpp` to `ymir_simulation_tests`

## Errors / Corrections

None.

## Ready for Next Run

`CouplingRegistry` fully available in `ymir_simulation`. Downstream tasks (task_04 `CouplingForceModel`, task_05 `NavalDomain`) can `#include <ymir/simulation/CouplingRegistry.h>` and link `ymir_simulation` which now transitively links `ymir_world`.
