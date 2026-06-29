---
status: completed
title: EnvironmentStore Zustand store
type: frontend
complexity: medium
dependencies:
    - task_05
---

# Task 6: EnvironmentStore Zustand store

## Overview

Creates the `useEnvironmentStore` Zustand store that manages the instructor's environmental condition configuration in the frontend. The store holds current, wind, and wave keyframe series; provides add/remove/edit actions; and exposes a `toJson()` method that serializes the state to the JSON contract accepted by `EnvironmentTimeline::loadJson()` in C++.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST create `apps/web/src/stores/environmentStore.ts` following the `create<StoreType>((set) => ({...}))` pattern used in `vesselPanelStore.ts`.
- Store state MUST match `EnvironmentProfileDTO`: `currentSeries: EnvironmentConditionDTO[][]`, `windSeries: EnvironmentConditionDTO[][]`, `waveSeries: WaveConditionDTO[]`.
- MUST expose actions: `addCurrentSeries`, `removeCurrentSeries(idx)`, `setCurrentKeyframe(seriesIdx, kfIdx, patch)`, `addCurrentKeyframe(seriesIdx)`, `removeCurrentKeyframe(seriesIdx, kfIdx)`, and equivalent actions for wind and wave.
- `addCurrentSeries()` MUST initialize a new series with one default keyframe (`{t:0, speed:0, dirNaut:0}`).
- `addCurrentKeyframe(seriesIdx)` MUST append a keyframe at `t = lastKeyframe.t + 300` (5 min interval default).
- `toJson()` MUST return a JSON string matching the `EnvironmentProfileDTO` schema; keyframes within each series MUST be sorted by `t` ascending before serialization.
- `hasConditions()` MUST return `false` when all three series arrays are empty, `true` otherwise.
- `reset()` MUST clear all series to empty arrays.
- MUST NOT store derived/computed state; `toJson()` is a pure serializer called on demand.
- Direction values stored in the store MUST be nautical degrees (0–360); no conversion in the store layer.
</requirements>

## Subtasks

- [x] 6.1 Create `apps/web/src/stores/environmentStore.ts` with the `EnvironmentStore` interface and initial state.
- [x] 6.2 Implement all current series actions (add, remove series; add, remove, set keyframe).
- [x] 6.3 Implement all wind series actions (mirroring current series actions).
- [x] 6.4 Implement wave keyframe actions (wave is a flat array, not array-of-arrays).
- [x] 6.5 Implement `toJson()` that serializes state sorted by `t` and validates against `EnvironmentProfileDTO`.
- [x] 6.6 Implement `hasConditions()` and `reset()`.
- [x] 6.7 Write `apps/web/src/stores/environmentStore.test.ts` covering all actions and serialization.

## Implementation Details

See TechSpec "Core Interfaces" (`EnvironmentStore`) and "Data Models" (JSON payload shape) sections.

Follow the Zustand pattern in `apps/web/src/stores/vesselPanelStore.ts`:
- Use `set((s) => ({...s, field: newValue}))` for immutable updates
- Export as `export const useEnvironmentStore = create<EnvironmentStore>((set) => ({...}))`
- Import types from `@ymir/types`

`toJson()` must produce output matching the exact JSON schema shown in TechSpec "Data Models". The C++ parser expects keys `currentSeries`, `windSeries`, `waveSeries` at the top level.

### Relevant Files

- `apps/web/src/stores/vesselPanelStore.ts` — Zustand store pattern reference
- `apps/web/src/stores/simulationStore.ts` — cross-store access pattern (`useVesselPanelStore.getState()`)
- `packages/types/src/environment.ts` — imported DTOs (available after Task 5)

### Dependent Files

- `apps/web/src/stores/simulationStore.ts` — calls `useEnvironmentStore.getState().toJson()` in Task 7
- `apps/web/src/features/scenario-creator/components/EnvironmentConditionPanel.tsx` — reads/writes store in Task 8
- `apps/web/src/features/scenario-creator/store.ts` — may include `environment` when building `CreateScenarioDTO`

### Related ADRs

- [ADR-006: Worker loadEnvironment Message Protocol](adrs/adr-006.md) — `toJson()` output feeds the `loadEnvironment` message payload
- [ADR-002: Homogeneous Condition Collections](adrs/adr-002.md) — store must enforce separate series arrays for each type (no mixing)

## Deliverables

- `apps/web/src/stores/environmentStore.ts` — complete Zustand store
- `apps/web/src/stores/environmentStore.test.ts` — full test suite
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for `toJson()` round-trip **(REQUIRED)**

## Tests

- Unit tests:
  - [ ] Initial state: `hasConditions()` returns `false`; `currentSeries`, `windSeries`, `waveSeries` are empty arrays
  - [ ] `addCurrentSeries()`: `currentSeries.length` increases by 1; new series contains one default keyframe
  - [ ] `removeCurrentSeries(0)`: series removed from state
  - [ ] `setCurrentKeyframe(0, 0, { speed: 2.5 })`: updates `currentSeries[0][0].speed` to `2.5`
  - [ ] `addCurrentKeyframe(0)`: new keyframe appended with `t = lastKeyframe.t + 300`
  - [ ] `removeCurrentKeyframe(0, 1)`: removes second keyframe from first series
  - [ ] `reset()`: `hasConditions()` returns `false` after reset
  - [ ] `toJson()`: output parses as valid JSON
  - [ ] `toJson()`: JSON string validates against `EnvironmentProfileDTO` TypeBox schema
  - [ ] `toJson()`: keyframes within a series are sorted by `t` ascending regardless of insertion order
  - [ ] `hasConditions()` returns `true` after `addWindSeries()`
  - [ ] Wind and wave actions mirror current behavior (spot-check one action each)
- Integration tests:
  - [ ] `JSON.parse(toJson())` round-trips: parsed object matches original store state
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `toJson()` output accepted by `JSON.parse()` and passes `EnvironmentProfileDTO` validation
- TypeScript compilation exits 0
- Store exports `useEnvironmentStore` as a named export importable by other modules
