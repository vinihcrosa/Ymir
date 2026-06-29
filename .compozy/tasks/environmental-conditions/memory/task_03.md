# Task Memory: task_03.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Integrate `EnvironmentTimeline` into `World::step()`. Add `timeline_` member + getters to `World.h`. Call `timeline_.advanceStep(time_, env_)` in `World::step()` after `time_ += dt` and before `coupling_.reset()`.

## Important Decisions

- Getters defined inline in `World.h` (same pattern as existing `time()`, `environment()`, `couplingRegistry()` — no separate `.cpp` entry needed).
- New tests placed in `TestWorldTimeline.cpp` (new file) rather than appended to `TestWorld.cpp` to keep domain-integration tests separate from basic World unit tests.

## Learnings

- `TestWorldTimeline.cpp` uses a `StubDomain`-style `EnvCaptureDomain` to verify env is resolved *before* domain `step()` is called — no NavalDomain needed for this integration assertion.
- ctest `-R ymir_world_tests` pattern did not match (Catch2 test names are used as CTest test names, not the executable name). Use `ctest --test-dir build --output-on-failure` to run all.

## Files / Surfaces

- `core/libs/world/include/ymir/world/World.h` — added `#include <EnvironmentTimeline.h>`, `timeline_` member, `timeline()` mutable + const getters
- `core/libs/world/src/World.cpp` — added `timeline_.advanceStep(time_, env_)` line 12 of `step()`
- `core/tests/world/TestWorldTimeline.cpp` — new file, 8 test cases
- `core/tests/CMakeLists.txt` — added `world/TestWorldTimeline.cpp` to `ymir_world_tests`

## Errors / Corrections

None.

## Ready for Next Run

Task complete. 255/255 tests pass. Task_04 (WASM loadEnvironment binding) can call `world_->timeline().loadJson(json)` — the getter exists and the `EnvironmentTimeline` header is transitively included via `World.h`.
