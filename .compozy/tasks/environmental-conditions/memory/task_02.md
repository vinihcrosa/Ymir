# Task Memory: task_02.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

EnvironmentTimeline C++ class — complete. Header + implementation + 25 tests, all passing.

## Important Decisions

- Helper functions (`normDeg`, `shortestDelta`, `nautFromToMathRad`, `mathVecToNautFrom`, `interpolateUniform`, `composeVectorial`, `parseSpectrum`, `parseUniformSeries`) placed in anonymous namespace in `.cpp` — not exposed in header, keeping public API clean.
- `advanceStep` is `const`; only calls setter on `env` if the corresponding series is non-empty, to avoid spurious `setCurrent(0,0)` when only wind/wave are loaded.
- Wave `seaState` has no public getters in `Environment.h` (Phase 2 note in existing test). Wave tests verify no-crash + unchanged unrelated fields. This is intentional, not a gap.
- `gamma` field in wave keyframes is optional (default 3.3); stored in WaveKeyframe but passed as `(void)gamma` after `setSeaState` since no gamma setter exists yet. Comment in code.
- `loadJson` commits data atomically (all vectors swapped at the end) so a parse error leaves the prior state intact.

## Learnings

- `Environment` assert on `speed >= 0` in setters. `composeVectorial` uses `hypot` which always returns >= 0, so no assert can fire from vectorial composition.
- resultDir guarded with `< 1e-15` epsilon when speed is near zero to avoid NaN from atan2(0,0) propagating to setCurrent/setWind.
- Test suite totals: 247 tests (was 222 after task_01 + 25 json smoke; now 247 after task_02 adds 25 more).

## Files / Surfaces

- CREATED: `core/libs/world/include/ymir/world/EnvironmentTimeline.h`
- CREATED: `core/libs/world/src/EnvironmentTimeline.cpp`
- CREATED: `core/tests/world/TestEnvironmentTimeline.cpp`
- MODIFIED: `core/libs/world/CMakeLists.txt` (added `src/EnvironmentTimeline.cpp`)
- MODIFIED: `core/tests/CMakeLists.txt` (added `world/TestEnvironmentTimeline.cpp` to `ymir_world_tests`)

## Errors / Corrections

None.

## Ready for Next Run

Task 03 (Integrate EnvironmentTimeline into World::step()) can begin. It needs to:
- Add `EnvironmentTimeline timeline_` member + `timeline()` getter to `World.h`
- Call `timeline_.advanceStep(time_, env_)` at the top of `World::step()` before domain iteration
- No new CMake changes needed (EnvironmentTimeline already compiled into ymir_world)
