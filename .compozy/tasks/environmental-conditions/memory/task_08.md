# Task Memory: task_08.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Build `EnvironmentConditionPanel.tsx` + tests. Three sections (Current, Wind, Wave). All state via `useEnvironmentStore`. Static/Temporal labels per keyframe count. Tests ≥80% coverage. Task complete.

## Important Decisions

- Wave section uses flat keyframe array (not array-of-series) matching `useEnvironmentStore.waveSeries: WaveConditionDTO[]`. "Add wave keyframe" button maps to `addWaveKeyframe()`. No "add series" concept for wave in Phase 1.
- "Series X — Static/Temporal" label pattern used (Static = 1 kf, Temporal = 2+).
- Gamma input hidden for PIERSON and REGULAR spectrum (only shown for JONSWAP).
- "Static" / "Temporal" also applied to Wave section title (e.g., "Wave — Static").
- All inputs use `type="number"` for direction; native browser rejects non-numeric.
- `disableRemove=false` for wave keyframes (user can remove all wave keyframes; unlike current/wind where last keyframe in a series cannot be removed).

## Learnings

- `pnpm vitest run <file>` runs targeted tests; coverage via `npx vitest run --coverage --coverage.include='path'`.
- `getByText(/current/i)` fails when text appears in multiple elements (span, button, p). Use `getByRole('button', { name: ... })` for add-series buttons instead.
- `apps/web` coverage thresholds apply globally — running against a single new file avoids pre-existing low-coverage failures (VesselPanel, vesselPanelStore, VesselControls).

## Files / Surfaces

- Created: `apps/web/src/features/scenario-creator/components/EnvironmentConditionPanel.tsx`
- Created: `apps/web/src/features/scenario-creator/components/EnvironmentConditionPanel.test.tsx`

## Errors / Corrections

- Initial test "renders all three section labels" used `getByText(/current/i)` — found multiple matches. Fixed to use `getByRole('button', { name: /add current series/i })` pattern.

## Ready for Next Run

Task 08 complete. Task 09 mounts `EnvironmentConditionPanel` in `ScenarioCreatorPage`. Import path: `./components/EnvironmentConditionPanel`. Component export: `EnvironmentConditionPanel` (named export, no default).
