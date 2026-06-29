---
status: completed
title: TypeScript EnvironmentProfileDTO types
type: chore
complexity: low
dependencies: []
---

# Task 5: TypeScript EnvironmentProfileDTO types

## Overview

Creates the TypeBox-validated TypeScript types that define the shared data contract between the frontend `EnvironmentStore` and the C++ JSON parser in `EnvironmentTimeline`. Also extends `ScenarioDTO` with an optional `environment` field for scenario persistence (F5). This task has no C++ dependency and can run in parallel with Tasks 2–4.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST create `packages/types/src/environment.ts` with: `EnvironmentConditionDTO`, `WaveConditionDTO`, `EnvironmentProfileDTO`, and their `Static<>` TypeScript types.
- `EnvironmentConditionDTO` MUST have: `t: number`, `speed: number (minimum: 0)`, `dirNaut: number (minimum: 0, maximum: 360)`.
- `WaveConditionDTO` MUST have: `t: number`, `Hs: number (minimum: 0)`, `Tp: number (minimum: 0)`, `dirNaut: number`, `spectrum: 'JONSWAP' | 'PIERSON' | 'REGULAR'`, `gamma: number (minimum: 0, default: 3.3)`.
- `EnvironmentProfileDTO` MUST have: `currentSeries: EnvironmentConditionDTO[][]`, `windSeries: EnvironmentConditionDTO[][]`, `waveSeries: WaveConditionDTO[]` (flat array — Phase 1 single wave series).
- `ScenarioDTO` in `packages/types/src/scenario.ts` MUST gain `environment: Type.Optional(EnvironmentProfileDTO)`.
- All new types MUST be exported from `packages/types/src/index.ts`.
- `CreateScenarioDTO` (derived from `ScenarioDTO` via `Type.Omit`) MUST automatically include `environment` without additional changes.
- Field descriptions MUST be consistent with TechSpec "Data Models" naming (nautical degrees, m/s, meters, seconds).
</requirements>

## Subtasks

- [x] 5.1 Create `packages/types/src/environment.ts` with `EnvironmentConditionDTO`, `WaveConditionDTO`, `EnvironmentProfileDTO`.
- [x] 5.2 Add `environment: Type.Optional(EnvironmentProfileDTO)` to `ScenarioDTO` in `packages/types/src/scenario.ts`.
- [x] 5.3 Export all new types from `packages/types/src/index.ts`.
- [x] 5.4 Verify `CreateScenarioDTO` still derives correctly via `Type.Omit(ScenarioDTO, ['id', 'createdAt'])`.
- [x] 5.5 Add type tests (compile-time and runtime schema validation) for the new types.

## Implementation Details

See TechSpec "Core Interfaces" (`EnvironmentProfileDTO`) and "Data Models" sections for exact field definitions.

Follow the TypeBox pattern already used in `packages/types/src/scenario.ts`:
- Import `Type` and `Static` from `@sinclair/typebox`
- Declare schema as `const XxxDTO = Type.Object({...})`
- Export TypeScript type as `export type XxxDTO = Static<typeof XxxDTO>`

`packages/types/src/scenario.ts` current `ScenarioDTO` structure (28 lines) — add one optional field, do not touch other fields.

### Relevant Files

- `packages/types/src/scenario.ts` — add `environment` field to `ScenarioDTO`
- `packages/types/src/index.ts` — add exports for new file
- `packages/types/src/vessel.ts` — reference for TypeBox patterns with nested objects
- `packages/types/src/simulation.ts` — reference for `VesselStateDTO` pattern

### Dependent Files

- `apps/web/src/stores/environmentStore.ts` — imports `EnvironmentProfileDTO`, `EnvironmentConditionDTO`, `WaveConditionDTO` (Task 6)
- `apps/web/src/features/scenario-creator/store.ts` — `toCreateScenarioDTO()` will include `environment` field

### Related ADRs

- [ADR-004: Temporal Keyframe Evolution for All Condition Types](adrs/adr-004.md) — defines what fields each condition type must carry

## Deliverables

- `packages/types/src/environment.ts` — new file with three exported schemas
- Updated `packages/types/src/scenario.ts` — `ScenarioDTO` extended with `environment`
- Updated `packages/types/src/index.ts` — exports
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests confirming schema validation **(REQUIRED)**

## Tests

- Unit tests:
  - [x] Valid `EnvironmentConditionDTO` (`{t:0, speed:1.0, dirNaut:90}`) passes TypeBox `Value.Check()` validation
  - [x] `EnvironmentConditionDTO` with `speed:-1` fails validation
  - [x] Valid `WaveConditionDTO` with `spectrum:'JONSWAP'` passes validation
  - [x] `WaveConditionDTO` with `spectrum:'INVALID'` fails validation
  - [x] `EnvironmentProfileDTO` with empty arrays for all three series passes validation
  - [x] `ScenarioDTO` without `environment` field passes validation (field is optional)
  - [x] `ScenarioDTO` with a valid `environment` object passes validation
  - [x] `CreateScenarioDTO` correctly omits `id` and `createdAt` and retains `environment`
- Integration tests:
  - [x] `packages/types` TypeScript compilation exits 0 with no type errors after changes
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- TypeScript compilation of `packages/types` exits 0
- All three new DTO types importable from `@ymir/types`
- Existing `ScenarioDTO` consumers (scenario-creator store, API routes if any) compile without changes
