# Task Memory: task_04.md

## Objective Snapshot

Add `loadEnvironment(json)` to `YmirSimulation` in `YmirBindings.cpp` and register it in the Embind block. Add mock stub + worker case on the JS side. Add integration tests.

## Important Decisions

- **Empty/`{}` guard**: `loadEnvironment` returns early if `json.empty() || json == "{}"`. Rationale: `loadJson("{}")` would throw (missing required top-level keys); the task requires these inputs to be a no-op.
- **No `.d.ts` file**: No separate WASM type declaration file exists in the project. `MockYmirSimulation` in `mock-wasm.ts` is the TypeScript type surface for the worker. Added `loadEnvironment(_json: string)` no-op there.
- **C++ tests**: `YmirSimulation` uses Emscripten headers — cannot be linked in native test suite. Behavior is covered by `TestEnvironmentTimeline.cpp` + `TestWorldTimeline.cpp` (which test `EnvironmentTimeline::loadJson` and `World::timeline()` directly). JS integration tests cover the binding.
- **Pre-existing coverage failure**: `pnpm test` fails coverage thresholds (57% overall) due to `VesselPanel.tsx` and `SidePanelStore.ts` — both pre-existing, out of task_04 scope.

## Files / Surfaces

- `core/src/wasm/YmirBindings.cpp` — added `loadEnvironment` method + Embind `.function()` registration
- `apps/web/src/workers/mock-wasm.ts` — added no-op `loadEnvironment(_json: string)` to `MockYmirSimulation`
- `apps/web/src/workers/simulation.worker.ts` — added `loadEnvironment` case to message handler
- `apps/web/src/workers/mock-wasm.test.ts` — added 5 integration tests for `loadEnvironment`

## Verification Evidence

- `tsc --noEmit`: 0 errors
- `ctest` full suite: 255/255 passed
- `vitest run mock-wasm.test.ts`: 9/9 passed (4 pre-existing + 5 new)

## Status

Complete. Diff ready for manual review (auto-commit disabled).
