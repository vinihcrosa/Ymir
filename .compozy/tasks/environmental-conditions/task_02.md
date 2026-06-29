---
status: completed
title: EnvironmentTimeline C++ class
type: backend
complexity: high
dependencies:
  - task_01
---

# Task 2: EnvironmentTimeline C++ class

## Overview

Introduces `EnvironmentTimeline`, the C++ class that owns the full environmental condition timeline, deserializes it from JSON, and resolves keyframe-interpolated and vectorially composed values into `Environment` setters at each simulation step. This is the core physics-side implementation for the entire environmental conditions feature.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST implement `loadJson(const std::string&)` that parses the `EnvironmentProfileDTO` JSON schema (see TechSpec "Data Models" section).
- MUST implement `advanceStep(double t, Environment& env)` that resolves current, wind, and wave conditions at time `t` and calls the appropriate `Environment` setters.
- Keyframe interpolation MUST use shortest-path angular interpolation for `dirNaut` (handles 350°→10° wrap-around without going through 180°).
- Speed and scalar wave parameters (Hs, Tp, gamma) MUST interpolate linearly between keyframes.
- Vectorial composition MUST sum resolved velocity vectors across all current series and all wind series independently; see TechSpec "Resolution Algorithm" section.
- Before the first keyframe and after the last keyframe, MUST clamp to the boundary keyframe value (no extrapolation).
- MUST sort keyframes by `t` on load (do not assume caller provides sorted input).
- `loadJson()` MUST throw `std::runtime_error` for: missing required fields, `speed < 0`, `Hs < 0`, `Tp <= 0`, invalid `spectrum` string.
- `empty()` MUST return `true` when no conditions are loaded; `advanceStep()` on an empty timeline MUST be a no-op.
- `reset()` MUST clear all loaded data and return `empty()` to `true`.
- Phase 1: `waveSeries` in JSON is a flat array of keyframes (single wave series); multi-wave composition is out of scope.
- MUST NOT modify `Environment.h` or any existing class.
- All direction arithmetic MUST stay within `EnvironmentTimeline.cpp`; no direction conversion logic leaks into callers.
</requirements>

## Subtasks

- [ ] 2.1 Create `core/libs/world/include/ymir/world/EnvironmentTimeline.h` with the class declaration per TechSpec "Core Interfaces".
- [ ] 2.2 Create `core/libs/world/src/EnvironmentTimeline.cpp` with `loadJson()`, keyframe sorting, and JSON field validation.
- [ ] 2.3 Implement the interpolation helper (shortest-path angular + linear scalar) used by all three condition types.
- [ ] 2.4 Implement vectorial composition for current and wind (N series → resultant speed + dirNaut).
- [ ] 2.5 Implement `advanceStep()` calling `env.setCurrent()`, `env.setWind()`, `env.setSeaState()` with resolved values.
- [ ] 2.6 Add `EnvironmentTimeline.cpp` to `core/libs/world/CMakeLists.txt`.
- [ ] 2.7 Create `core/tests/world/TestEnvironmentTimeline.cpp` with the full test suite from TechSpec "Testing Approach".

## Implementation Details

See TechSpec "Core Interfaces" for the class declaration and "Resolution Algorithm" for the interpolation and composition pseudocode. See TechSpec "Data Models" for the exact JSON schema that `loadJson()` must accept.

The nautical direction convention used throughout: 0 = North, clockwise, "from where". Vector arithmetic requires converting "from" to "to" before computing components; see TechSpec "Resolution Algorithm" for the formula. After composition, convert back to nautical "from" before calling `env.setCurrent()`.

### Relevant Files

- `core/libs/world/include/ymir/world/Environment.h` — setters called by `advanceStep()`; check exact signatures before implementing
- `core/libs/world/src/World.cpp` — will call `advanceStep()` in Task 3; do not modify here
- `core/tests/world/TestEnvironment.cpp` — reference for test framework and assertion patterns
- `core/libs/world/CMakeLists.txt` — add new `.cpp` source file here

### Dependent Files

- `core/libs/world/include/ymir/world/World.h` — will hold `EnvironmentTimeline` member (Task 3)
- `core/src/wasm/YmirBindings.cpp` — will call `loadJson()` indirectly via World (Task 4)

### Related ADRs

- [ADR-005: C++ EnvironmentTimeline as Keyframe Resolution Layer](adrs/adr-005.md) — defines ownership model and resolution contract
- [ADR-004: Temporal Keyframe Evolution for All Condition Types](adrs/adr-004.md) — specifies interpolation model
- [ADR-003: Independent Evaluation + Vector Sum for Grid Maps](adrs/adr-003.md) — defines composition model (uniform analog: same principle)

## Deliverables

- `core/libs/world/include/ymir/world/EnvironmentTimeline.h` — new class header
- `core/libs/world/src/EnvironmentTimeline.cpp` — full implementation
- `core/tests/world/TestEnvironmentTimeline.cpp` — complete test suite
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for `advanceStep()` calling correct `Environment` setters **(REQUIRED)**

## Tests

- Unit tests:
  - [x] Empty timeline: `advanceStep(0.0, env)` leaves `env.currentSpeed() == 0` and `env.windSpeed() == 0`
  - [x] Single current keyframe at t=0, speed=1.0: `advanceStep(500, env)` → `env.currentSpeed() == 1.0` (clamp after last)
  - [x] Two current keyframes [t=0, spd=0], [t=100, spd=2.0]: `advanceStep(50, env)` → `env.currentSpeed() == 1.0`
  - [x] Angular wrap-around: series [t=0, dir=350°], [t=100, dir=10°]: `advanceStep(50, env)` → resultant direction ≈ 0° (not 180°)
  - [x] Before first keyframe: t=-10 clamps to first keyframe values
  - [x] After last keyframe: t=999 when last is t=100 clamps to last keyframe values
  - [x] Two current series, opposite directions, equal speeds: resultant speed == 0
  - [x] Two current series, same direction: resultant speed == sum of both speeds
  - [x] Perpendicular currents each 1.0 m/s: resultant speed ≈ √2
  - [x] Wave single keyframe: `advanceStep()` calls `env.setSeaState(Hs, Tp, dirNaut)` with correct values
  - [x] Wave two keyframes: Hs interpolates linearly at midpoint
  - [x] `loadJson()` throws on `speed: -1`
  - [x] `loadJson()` throws on missing `dirNaut` field
  - [x] `loadJson()` throws on `spectrum: "UNKNOWN"`
  - [x] `loadJson()` throws on malformed JSON string
  - [x] `reset()`: `empty()` returns `true`; subsequent `advanceStep()` is a no-op
- Integration tests:
  - [x] Full round-trip: load JSON with current + wind + wave, call `advanceStep(0, env)`, verify all three `env` getters return expected values
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- All 14 unit test scenarios above pass
- `Environment` setters called with correct nautical-degree direction values (verified by test mocking or direct getter checks)
- Zero changes to `Environment.h`, `World.h`, or any existing source file
