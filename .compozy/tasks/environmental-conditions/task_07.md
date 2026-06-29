---
status: completed
title: Worker loadEnvironment message and simulationStore integration
type: frontend
complexity: medium
dependencies:
  - task_04
  - task_06
---

# Task 7: Worker loadEnvironment message and simulationStore integration

## Overview

Connects the frontend `EnvironmentStore` to the WASM simulation by adding the `loadEnvironment` message handler to the Web Worker and updating `simulationStore` to send the environment payload at the correct points in the simulation lifecycle. After this task, configured environmental conditions are applied to every simulation run automatically.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST add a `loadEnvironment` case to the `switch` in `simulation.worker.ts` message handler; call `simulation?.loadEnvironment(json)` on receipt.
- `simulation.worker.ts` change MUST NOT alter any existing `start`, `stop`, `loadScenario`, or `setActuator` behavior.
- `simulationStore.start()` MUST send `loadEnvironment` BEFORE the `start` message, inside the `case 'ready':` handler, after actuator re-sync and before `worker.postMessage({ type: 'start', dt })`.
- `simulationStore.reset()` currently terminates the Worker entirely. The next `start()` call creates a new Worker, which goes through the `ready` flow — no separate reset re-send is needed; the existing `ready` handler covers it.
- MUST import `useEnvironmentStore` in `simulationStore.ts` using `useEnvironmentStore.getState()` (same cross-store pattern used for `useVesselPanelStore`).
- Only send `loadEnvironment` if `useEnvironmentStore.getState().hasConditions()` is `true`; do not send empty payload unnecessarily.
- Message shape MUST match TechSpec "Worker Message Protocol": `{ type: 'loadEnvironment', json: string }`.
</requirements>

## Subtasks

- [x] 7.1 Add `case 'loadEnvironment':` to `simulation.worker.ts` message dispatch calling `simulation?.loadEnvironment(json)`.
- [x] 7.2 Update WASM TypeScript type declaration (if present) to include `loadEnvironment(json: string): void` on `YmirSimulation`.
- [x] 7.3 Import `useEnvironmentStore` in `simulationStore.ts`.
- [x] 7.4 In `simulationStore.start()`, inside `case 'ready':` handler, add the `loadEnvironment` send before the `start` message.
- [x] 7.5 Write tests for the new worker message case and the simulationStore integration.

## Implementation Details

See TechSpec "Worker Message Protocol" section for exact message shape and ordering.

Current `simulationStore.start()` `case 'ready':` handler (simplified):
```typescript
case 'ready': {
    set({ status: 'ready' })
    // loads scenario vessels...
    // re-syncs actuator state...
    get().worker!.postMessage({ type: 'start', dt })  // ← insert loadEnvironment before this
    set({ status: 'running' })
    break
}
```

Insert between the actuator re-sync block and the final `start` message:
```typescript
const envStore = useEnvironmentStore.getState()
if (envStore.hasConditions()) {
    w.postMessage({ type: 'loadEnvironment', json: envStore.toJson() })
}
```

`simulation.worker.ts` current dispatch handles: `start`, `stop`, `loadScenario`, `setActuator`. Add `loadEnvironment` as a new `case` before the default.

### Relevant Files

- `apps/web/src/workers/simulation.worker.ts` — add `loadEnvironment` case
- `apps/web/src/stores/simulationStore.ts` — import store and send message in `ready` handler
- `apps/web/src/stores/environmentStore.ts` — `getState().hasConditions()` and `getState().toJson()` called here (available after Task 6)

### Dependent Files

- `apps/web/src/features/scenario-creator/components/EnvironmentConditionPanel.tsx` — its writes to `EnvironmentStore` are automatically picked up on next simulation start (Task 8)

### Related ADRs

- [ADR-006: Worker loadEnvironment Message Protocol](adrs/adr-006.md) — defines message shape and delivery ordering

## Deliverables

- Updated `apps/web/src/workers/simulation.worker.ts`
- Updated `apps/web/src/stores/simulationStore.ts`
- Tests for new message handler and store integration
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests verifying environment delivery to WASM **(REQUIRED)**

## Tests

- Unit tests:
  - [ ] Worker `loadEnvironment` message with valid JSON: `simulation.loadEnvironment` called with that JSON string
  - [ ] Worker `loadEnvironment` message when `simulation` is null/undefined: no error thrown
  - [ ] `simulationStore.start()` with non-empty `EnvironmentStore`: `loadEnvironment` message sent before `start` message
  - [ ] `simulationStore.start()` with empty `EnvironmentStore` (`hasConditions() === false`): `loadEnvironment` message NOT sent
  - [ ] Existing `start`, `stop`, `loadScenario`, `setActuator` worker message paths unchanged
- Integration tests:
  - [ ] Full simulation start with one current condition configured: vessel drifts laterally in expected direction after several steps
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `simulation.loadEnvironment` called with correct JSON when environment conditions are configured
- No regression in existing worker message handling
- TypeScript compilation exits 0
