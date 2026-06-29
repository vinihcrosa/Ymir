# Task Memory: task_09.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Mount `EnvironmentConditionPanel` in the `ScenarioCreatorPage` layout so instructors can access environmental conditions from the scenario editor.

## Important Decisions

- Placed inside `Sidebar.tsx` (not directly in `ScenarioCreatorPage.tsx`) — consistent with existing control grouping (`ScenarioForm`, `VesselList`, `SimulationControls`).
- No props passed to `EnvironmentConditionPanel` — pulls all state from `useEnvironmentStore` internally.

## Learnings

- `ScenarioCreatorPage.tsx` and `Sidebar.tsx` are explicitly excluded from coverage thresholds in `vitest.config.ts` (lines 28–30: "Pure layout composition — no logic to unit test"). Tests written are smoke tests, not coverage-driven.
- WASM-level integration test (add current → start sim → observe drift) is not runnable in jsdom. Covered by `EnvironmentConditionPanel.test.tsx > integration` (toJson round-trip) + existing `simulationStore` tests. Log as E2E/playwright follow-up.
- `ScenarioCreatorPage.test.tsx` mocks `AreaMapView` and `VesselPanel` (heavy deps), `useAreas` and `useVessels` (fetch), and stubs `Worker`/`URL` globals. Renders `Sidebar` fully so `EnvironmentConditionPanel` appears naturally.

## Files / Surfaces

- `apps/web/src/features/scenario-creator/components/Sidebar.tsx` — added import + `<EnvironmentConditionPanel />` below `<SimulationControls />`
- `apps/web/src/features/scenario-creator/ScenarioCreatorPage.test.tsx` — new; 6 smoke tests

## Errors / Corrections

None.

## Ready for Next Run

Task complete. All tasks in the environmental-conditions feature (01–09) are done.
