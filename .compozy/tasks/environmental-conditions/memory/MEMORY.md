# Workflow Memory

Keep only durable, cross-task context here. Do not duplicate facts that are obvious from the repository, PRD documents, or git history.

## Current State

All tasks (01–09) complete. `EnvironmentTimeline` in C++, `YmirSimulation::loadEnvironment(json)` in WASM, worker `loadEnvironment` case, `useEnvironmentStore` Zustand store, `simulationStore` integration, `EnvironmentConditionPanel` React component, and panel mounted in `ScenarioCreatorPage` via `Sidebar` — all done. Feature is fully integrated; diff is staged for manual review and commit.

## Shared Decisions

- **JSON library**: nlohmann/json v3.11.3 (already in `Dependencies.cmake`). Linked PUBLIC to `ymir_world`. Same version pinned in `CMakeLists.wasm.txt`.
- **WASM dependency pattern**: The WASM build (`CMakeLists.wasm.txt`) does NOT include `core/cmake/Dependencies.cmake`. Any dependency used in `libs/` that is declared in Dependencies.cmake must be re-declared with an identical FetchContent block in `CMakeLists.wasm.txt`. This applies to all future tasks adding dependencies.
- **World getter pattern**: New getters on `World` are defined inline in `World.h` (no separate `.cpp` implementation). Follow this pattern for any future getters.

## Shared Learnings

- `core/build-wasm.sh` stages `CMakeLists.wasm.txt` (renamed to `CMakeLists.txt`) alongside `libs/`, `cmake/`, and `src/` — it's a separate build tree from the native build.
- Test runner for native build: `ctest --test-dir /Users/viniciusrosa/Documents/GitHub/Ymir/core/build`. Full suite: 255 tests as of task_03 completion.
- ctest `-R <executable-name>` does not match — Catch2 registers individual test names as CTest tests. Use `--output-on-failure` with no `-R` filter to run all, or `-R "<test case name pattern>"` for targeted runs.
- `packages/types` had no test runner before task_05. vitest `^4.1.9` + `@vitest/coverage-v8` added. Config at `packages/types/vitest.config.ts`. Coverage scoped to only the files under test (exclude pre-existing area/simulation/vessel).
- TypeBox `Value.Check()` fails on `format: 'date-time'` fields unless `FormatRegistry.Set('date-time', ...)` is called in test setup — required for any test that validates `ScenarioDTO`.
- `@sinclair/typebox` is NOT declared in `apps/web/package.json` — any test in `apps/web` that calls `Value.Check()` must add it as devDependency first.
- `apps/web` coverage thresholds are global (all src files). `pnpm test` exits 1 due to VesselPanel.tsx (0%), vesselPanelStore.ts (18%), VesselControls.tsx (43%) — pre-existing, not caused by env-conditions work. Use `--coverage.include='src/stores/...'` to verify task-scoped coverage.

## Open Risks

- WASM Docker build not verified locally (requires Emscripten toolchain). Mitigated: nlohmann/json is header-only and Emscripten-compatible; same pattern used by other header-only deps. Must verify in CI before merging task_01 diff.

## Handoffs

- Task 09 (ScenarioCreatorPage mount): Import `EnvironmentConditionPanel` from `'./components/EnvironmentConditionPanel'` (named export). No props required — component pulls state entirely from `useEnvironmentStore`.
- `loadEnvironment` guard: empty string or `"{}"` = no-op (early return before `loadJson`). Valid JSON with proper schema is forwarded to `World::timeline().loadJson(json)`.
