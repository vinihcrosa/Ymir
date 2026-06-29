# Task Memory: task_06.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Create `useEnvironmentStore` Zustand store managing environmental conditions (current/wind/wave keyframe series) with `toJson()` serializer and full action set.

## Important Decisions

- Followed `vesselPanelStore.ts` pattern exactly: `create<T>((set, get) => ({...}))`.
- `hasConditions()` uses `get()` (not `set`) since it's a pure read.
- `toJson()` is a pure serializer — sorts each series by `t` ascending before serialization, no side effects.
- Default wave keyframe: `{ t: 0, Hs: 0, Tp: 1, dirNaut: 0, spectrum: 'JONSWAP', gamma: 3.3 }` — `Tp: 1` not 0 (avoids division-by-zero in C++ parser).

## Learnings

- `@sinclair/typebox` was not declared in `apps/web/package.json` — had to add it as devDependency for `Value.Check()` in tests.
- Global coverage thresholds in `apps/web/vitest.config.ts` apply to all src files; run with `--coverage.include='src/stores/environmentStore.ts'` to scope to just the new file.
- Branch coverage: `ki !== kfIdx` else-paths in `removeWindKeyframe` (lines 84, 101) are the 2 remaining uncovered branches (92.59% total — above 80% threshold).

## Files / Surfaces

- Created: `apps/web/src/stores/environmentStore.ts`
- Created: `apps/web/src/stores/environmentStore.test.ts`
- Modified: `apps/web/package.json` (added `@sinclair/typebox ^0.34.0` devDependency)
- Modified: `pnpm-lock.yaml` (lockfile updated)

## Errors / Corrections

- First test run failed: `@sinclair/typebox/value` not resolvable in apps/web context → fixed by adding devDep.
- Branch coverage was 66.66% after initial tests → added 7 additional branch-covering tests (multi-series scenarios, empty-series edge cases, non-target keyframe mutations) → raised to 92.59%.

## Ready for Next Run

Task 07 (simulationStore integration): call `useEnvironmentStore.getState().hasConditions()` and `.toJson()` before sending `start` worker message. Store is ready and exports `useEnvironmentStore` as named export.
