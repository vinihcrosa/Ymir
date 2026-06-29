# Task Memory: task_07.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Wire `EnvironmentStore` → worker `loadEnvironment` message → WASM. Sends environment JSON before `start` in simulationStore `case 'ready':` handler.

## Important Decisions

- Worker `loadEnvironment` case was already present (added in prior task). Only needed simulationStore integration.
- Subtask 7.2 (WASM type declaration): worker uses `let simulation: any`, mock already has `loadEnvironment`. No separate `.d.ts` needed.
- Used `useEnvironmentStore.getState()` pattern consistent with `useVesselPanelStore.getState()` already in the file.

## Learnings

- Global coverage thresholds in `apps/web/vitest.config.ts` were failing before task 07 due to VesselPanel.tsx (0%), vesselPanelStore.ts (18%), VesselControls.tsx (43%). `pnpm test` exits 1. Task-scoped coverage passes at 92.85%/89.79%/94.54%/92.52%.
- Git stash does NOT stash untracked files — environmentStore.ts and environmentStore.test.ts remained when stashing.
- `beforeEach` must reset both `useSimulationStore` (via `store().reset()`) AND `useEnvironmentStore` (via `useEnvironmentStore.getState().reset()`) to avoid test state bleed.

## Files / Surfaces

- `apps/web/src/stores/simulationStore.ts` — added import + 4 lines in `case 'ready':` before `start` message
- `apps/web/src/stores/simulationStore.test.ts` — added `useEnvironmentStore` import, reset in beforeEach, 5 new tests in `loadEnvironment on start` describe block

## Errors / Corrections

None. Clean implementation on first attempt.

## Ready for Next Run

Task 07 complete. simulationStore now sends `loadEnvironment` before `start` when conditions exist. Task 08 (EnvironmentConditionPanel UI) can proceed.
