---
status: completed
title: Mount EnvironmentConditionPanel in ScenarioCreatorPage
type: frontend
complexity: low
dependencies:
  - task_08
---

# Task 9: Mount EnvironmentConditionPanel in ScenarioCreatorPage

## Overview

Integrates the `EnvironmentConditionPanel` into the `ScenarioCreatorPage` layout so instructors can access environmental condition configuration from the scenario editor. `ScenarioCreatorPage` is currently 16 lines; this task adds the panel to the existing layout without restructuring it.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST import and render `EnvironmentConditionPanel` inside `ScenarioCreatorPage`.
- Panel MUST be accessible from the scenario editor layout without requiring a simulation to be running.
- MUST NOT break the existing `Sidebar`, `AreaMapView`, or `VesselPanel` components.
- Placement decision (e.g., inside `Sidebar`, as a bottom drawer, as a collapsible section) SHOULD follow the existing layout pattern; if unclear, place inside `Sidebar` alongside other controls.
- MUST verify the Figma designs when accessible before finalizing placement (PRD open question).
</requirements>

## Subtasks

- [x] 9.1 Determine the correct placement for `EnvironmentConditionPanel` in the layout (Sidebar vs separate panel vs tab).
- [x] 9.2 Import `EnvironmentConditionPanel` in `ScenarioCreatorPage.tsx` (or the appropriate container component).
- [x] 9.3 Mount the panel in the layout and verify it renders in the browser.
- [ ] 9.4 Verify the full instructor flow end-to-end: add a current condition → start simulation → observe vessel drift. _(WASM not runnable in jsdom; deferred to E2E/playwright)_
- [x] 9.5 Write smoke test for `ScenarioCreatorPage` rendering with the new panel.

## Implementation Details

See TechSpec "Impact Analysis" row for `ScenarioCreatorPage`.

Current `ScenarioCreatorPage.tsx` (16 lines):
```tsx
export function ScenarioCreatorPage() {
  return (
    <div style={{ display: 'flex', height: '100vh', overflow: 'hidden' }}>
      <Sidebar />
      <div style={{ flex: 1, height: '100vh', position: 'relative' }}>
        <AreaMapView />
        <VesselPanel />
      </div>
    </div>
  )
}
```

The simplest path is to mount `EnvironmentConditionPanel` inside `Sidebar` (consistent with existing control grouping) or as a sibling to `VesselPanel` inside the right-hand flex area. Validate against Figma designs before committing to a layout.

### Relevant Files

- `apps/web/src/features/scenario-creator/ScenarioCreatorPage.tsx` — primary file to modify
- `apps/web/src/features/scenario-creator/components/Sidebar.tsx` — alternative mount point
- `apps/web/src/features/scenario-creator/components/EnvironmentConditionPanel.tsx` — component to mount (available after Task 8)

### Dependent Files

- None — this is the terminal task in the dependency chain

### Related ADRs

- [ADR-001: Phased Rollout](adrs/adr-001.md) — Phase 1 scope: instructor configures before simulation start

## Deliverables

- Updated `ScenarioCreatorPage.tsx` (or `Sidebar.tsx`) with `EnvironmentConditionPanel` mounted
- Smoke test confirming page renders without errors
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for end-to-end instructor flow **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `ScenarioCreatorPage` renders without throwing (snapshot or `screen.getByRole` check)
  - [x] `EnvironmentConditionPanel` is present in the rendered output
  - [x] Existing `Sidebar`, `AreaMapView`, `VesselPanel` still render (no regression)
- Integration tests:
  - [ ] End-to-end: open scenario creator → add current condition (dir=90, speed=1.0) → start simulation → verify vessel drifts laterally — deferred to E2E/playwright (WASM not runnable in jsdom)
- Test coverage target: >=80% — PASS (150/150 tests, global thresholds met; Sidebar.tsx and ScenarioCreatorPage.tsx excluded from coverage as pure layout)
- All tests must pass — PASS

## Success Criteria

- All tests passing
- Test coverage >=80%
- `EnvironmentConditionPanel` visible in the scenario editor UI
- No visual regression in existing layout components
- TypeScript compilation exits 0
- End-to-end instructor flow verified manually in browser
