---
status: completed
title: EnvironmentConditionPanel React component
type: frontend
complexity: high
dependencies:
  - task_06
---

# Task 8: EnvironmentConditionPanel React component

## Overview

Builds the instructor-facing UI panel for configuring environmental conditions. The panel groups conditions by type (Current, Wind, Wave), lets instructors add/remove condition series and individual keyframes, and exposes direction/speed/wave-parameter inputs using nautical degrees. The component reads from and writes to `useEnvironmentStore`.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST create `apps/web/src/features/scenario-creator/components/EnvironmentConditionPanel.tsx`.
- Panel MUST have three sections: **Current**, **Wind**, **Wave** — one per condition type (PRD F1).
- Each section MUST allow adding and removing condition series (PRD F1).
- Each series row MUST show: keyframe timestamp (s), direction input (nautical degrees, 0–360), speed input (m/s ≥ 0). Wave rows additionally show: Hs (m), Tp (s), spectrum dropdown, gamma.
- Instructors MUST be able to add/remove individual keyframes within a series (PRD F4).
- Direction inputs MUST display and accept nautical degrees. No raw math convention exposed in the UI.
- Direction input label MUST read "Direction (°)" with helper text "0 = North, clockwise, from where".
- A series with multiple keyframes MUST display them sorted by timestamp and show a note indicating temporal interpolation is active.
- A series with one keyframe MUST be labeled "Static".
- MUST use `useEnvironmentStore` for all state reads and writes; no local component state for condition data.
- MUST NOT implement any compass rose widget in Phase 1 — plain numeric input is sufficient.
- Component MUST be independently renderable (no hidden dependency on simulation state).
</requirements>

## Subtasks

- [x] 8.1 Create `EnvironmentConditionPanel.tsx` with the three-section layout (Current, Wind, Wave).
- [x] 8.2 Implement current series list: add/remove series, add/remove keyframes, direction + speed inputs per keyframe.
- [x] 8.3 Implement wind series list: mirror current series structure.
- [x] 8.4 Implement wave section: single series of keyframes, add Hs/Tp/spectrum/gamma inputs.
- [x] 8.5 Apply "Static" / "Temporal" labeling logic per series keyframe count.
- [x] 8.6 Write component tests covering rendering and user interactions.

## Implementation Details

See PRD section "User Experience — Primary Flow" and "Condition List Layout" for the required UX behavior.

Follow existing component patterns in `apps/web/src/features/scenario-creator/components/`:
- `VesselPanel.tsx` — reference for panel layout with labeled sections and input fields
- `VesselList.tsx` — reference for list + add/remove item pattern

State is managed entirely in `useEnvironmentStore`. Component calls store actions on user input (same pattern as `VesselPanel.tsx` calling `useVesselPanelStore`).

### Relevant Files

- `apps/web/src/stores/environmentStore.ts` — store consumed by this component (available after Task 6)
- `apps/web/src/features/scenario-creator/components/VesselPanel.tsx` — layout and input pattern reference
- `apps/web/src/features/scenario-creator/components/VesselList.tsx` — add/remove list pattern
- `packages/types/src/environment.ts` — `WaveConditionDTO` spectrum union for dropdown options

### Dependent Files

- `apps/web/src/features/scenario-creator/ScenarioCreatorPage.tsx` — mounts this component in Task 9

### Related ADRs

- [ADR-004: Temporal Keyframe Evolution for All Condition Types](adrs/adr-004.md) — drives the "Static" vs "Temporal" labeling logic
- [ADR-002: Homogeneous Condition Collections](adrs/adr-002.md) — each section is a separate list (no mixing current + wind series)

## Deliverables

- `apps/web/src/features/scenario-creator/components/EnvironmentConditionPanel.tsx`
- Component test file `EnvironmentConditionPanel.test.tsx`
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for user interaction flows **(REQUIRED)**

## Tests

- Unit tests:
  - [x] Renders without errors when store has no conditions (empty state)
  - [x] "Add Current" button click calls `addCurrentSeries()` on the store
  - [x] Removing a current series calls `removeCurrentSeries(idx)` with correct index
  - [x] Speed input change calls `setCurrentKeyframe(seriesIdx, kfIdx, { speed: value })`
  - [x] Direction input rejects non-numeric input; accepts 0–360
  - [x] Series with 1 keyframe displays "Static" label
  - [x] Series with 2+ keyframes displays "Temporal" label (or equivalent)
  - [x] "Add keyframe" button appends a keyframe to the series
  - [x] Wave spectrum dropdown shows JONSWAP, PIERSON, REGULAR options
  - [x] Gamma input only visible when spectrum is JONSWAP
- Integration tests:
  - [x] Full flow: add a current series → set direction 90 and speed 1.0 → store `toJson()` reflects the values
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Panel renders all three condition type sections
- All store action bindings verified by tests
- TypeScript compilation exits 0
- Component is importable by `ScenarioCreatorPage` (Task 9)
