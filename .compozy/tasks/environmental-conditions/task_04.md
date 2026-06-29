---
status: completed
title: WASM loadEnvironment binding
type: backend
complexity: medium
dependencies:
  - task_03
---

# Task 4: WASM loadEnvironment binding

## Overview

Exposes `loadEnvironment(json)` from C++ to JavaScript by adding the method to `YmirSimulation` in `YmirBindings.cpp` and registering it with Embind. After this task, the Web Worker can call `simulation.loadEnvironment(json)` to load the full environment timeline before the simulation starts.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST add `void loadEnvironment(const std::string& json)` method to `YmirSimulation` that calls `world_->timeline().loadJson(json)`.
- MUST register the new method in the `EMSCRIPTEN_BINDINGS` block: `.function("loadEnvironment", &YmirSimulation::loadEnvironment)`.
- `loadEnvironment()` called with an empty string or `{}` MUST produce a no-op (empty timeline = default zero environment).
- `loadEnvironment()` called with valid JSON MUST overwrite any previously loaded timeline (idempotent re-load).
- MUST NOT break any existing Embind-registered methods or their behavior.
- The WASM binary MUST rebuild successfully after this change (Docker/Emscripten pipeline).
</requirements>

## Subtasks

- [x] 4.1 Add `void loadEnvironment(const std::string& json)` method to `YmirSimulation` class in `YmirBindings.cpp`.
- [x] 4.2 Implement the method body: call `world_->timeline().loadJson(json)` (with guard for empty string).
- [x] 4.3 Add `.function("loadEnvironment", &YmirSimulation::loadEnvironment)` to the `EMSCRIPTEN_BINDINGS` block.
- [x] 4.4 Rebuild the WASM binary and verify `simulation.loadEnvironment` is callable from a JS smoke test.
- [x] 4.5 Add TypeScript type for the new method to the WASM module type declaration (if one exists in the project).

## Implementation Details

See TechSpec "Worker Message Protocol" section and "WASM binding" build order step.

`YmirBindings.cpp` current `EMSCRIPTEN_BINDINGS` block (lines 356–368):
```cpp
EMSCRIPTEN_BINDINGS(ymir) {
    emscripten::class_<YmirSimulation>("YmirSimulation")
        .constructor<>()
        .function("addVessel",          &YmirSimulation::addVessel)
        // ... existing 7 methods
        .function("getTime",            &YmirSimulation::getTime);
}
```

Add `loadEnvironment` after `getTime`.

If the project has a TypeScript declaration file for the WASM module (likely at `apps/web/src/workers/ymir.d.ts` or similar), add `loadEnvironment(json: string): void` to the `YmirSimulation` interface.

### Relevant Files

- `core/src/wasm/YmirBindings.cpp` — add method and Embind registration
- `core/libs/world/include/ymir/world/World.h` — `timeline()` getter used by new method (available after Task 3)
- `core/src/wasm/CMakeLists.txt` — WASM build target; may need to verify `world` library is linked
- `apps/web/src/workers/` — look for `.d.ts` type declaration for WASM module; update if found

### Dependent Files

- `apps/web/src/workers/simulation.worker.ts` — calls `simulation.loadEnvironment(json)` in Task 7
- TypeScript WASM type declarations — must reflect new method signature

### Related ADRs

- [ADR-005: C++ EnvironmentTimeline as Keyframe Resolution Layer](adrs/adr-005.md) — WASM is the delivery mechanism
- [ADR-006: Worker loadEnvironment Message Protocol](adrs/adr-006.md) — defines the JS-side caller contract

## Deliverables

- Updated `YmirBindings.cpp` with `loadEnvironment` method and Embind registration
- WASM binary rebuilt and verified
- TypeScript WASM type declaration updated (if applicable)
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests verifying `loadEnvironment` callable from JS **(REQUIRED)**

## Tests

- Unit tests (C++ — if a native test harness exercises `YmirSimulation` directly):
  - [x] `YmirSimulation::loadEnvironment("{}")` does not throw and timeline remains empty
  - [x] `YmirSimulation::loadEnvironment(validCurrentJson)` followed by `step()` results in nonzero `env.currentSpeed()`
- Integration tests (JS/WASM):
  - [x] `typeof simulation.loadEnvironment === 'function'` after WASM module load
  - [x] `simulation.loadEnvironment('{"currentSeries":[[{"t":0,"speed":1.0,"dirNaut":90}]],"windSeries":[],"waveSeries":[]}')` followed by `simulation.step(0.1)` does not throw
  - [x] Calling `loadEnvironment` twice with different JSON replaces the timeline (idempotency)
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `simulation.loadEnvironment` callable from JavaScript with no exceptions
- WASM Docker build exits 0
- All previously registered Embind methods still work
