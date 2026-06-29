# TechSpec: Environmental Conditions

**Feature:** Environmental Conditions for Simulation Scenarios  
**Status:** Draft  
**Date:** 2026-06-29  
**PRD:** [_prd.md](_prd.md)

---

## Executive Summary

Environmental conditions (current, wind, wave) are introduced as a C++-owned timeline that resolves to `Environment` setters at each simulation step. The frontend serializes a typed `EnvironmentProfileDTO` — containing keyframe series for each condition type — to a JSON string once before simulation starts, and delivers it via a new `loadEnvironment` worker message. C++ deserializes the payload into `EnvironmentTimeline`, which calls `World::step()` to resolve interpolated and vectorially composed values into the existing `Environment` singleton before any domain steps.

The primary trade-off is **two new C++ files and a JSON parsing dependency** in exchange for simulation-time-authoritative resolution, no per-step JS/WASM communication, and a clean extension point for Phase 3 (real-time instructor overrides).

Phase 1 delivers: uniform current, wind, and wave conditions; multiple current/wind series composing vectorially; temporal keyframe interpolation for all types; single wave series (multi-wave composition deferred to Phase 2 with physics engine changes); scenario persistence.

---

## System Architecture

### Component Overview

```
┌────────────────────────────────────────────────────────────────┐
│  Scenario Creator (Frontend)                                   │
│                                                                │
│  EnvironmentStore ──toJson()──┐                                │
│  ScenarioStore                │                                │
│  simulationStore ─────────────┼──loadEnvironment msg──►Worker  │
└───────────────────────────────┼────────────────────────────────┘
                                │ json string (once on start/reset)
                                ▼
┌────────────────────────────────────────────────────────────────┐
│  simulation.worker.ts (Web Worker)                             │
│                                                                │
│  YmirSimulation.loadEnvironment(json)                          │
└────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌────────────────────────────────────────────────────────────────┐
│  C++ WASM (YmirBindings.cpp + World)                           │
│                                                                │
│  YmirSimulation::loadEnvironment(json)                         │
│    └──► World::timeline().loadJson(json)                       │
│                                                                │
│  World::step(dt):                                              │
│    1. timeline_.advanceStep(time_, env_)   ← NEW               │
│    2. domain->step(dt)   (unchanged)                           │
│       └─► NavalDomain::buildContext() reads env_  (unchanged)  │
└────────────────────────────────────────────────────────────────┘
```

**Components:**

| Component | Role |
|-----------|------|
| `EnvironmentTimeline` (C++) | Stores keyframe series, resolves to `Environment` setters each step |
| `World` (C++) | Owns `EnvironmentTimeline`; calls `advanceStep()` before domain steps |
| `YmirSimulation` (WASM facade) | Exposes `loadEnvironment(json)` to JavaScript |
| `simulation.worker.ts` | Routes `loadEnvironment` message to WASM |
| `simulationStore.ts` | Sends `loadEnvironment` + `start` in correct order |
| `EnvironmentStore` (Zustand) | Owns UI state; serializes to JSON for WASM |
| `EnvironmentConditionPanel` (React) | Instructor UI for adding/editing conditions |
| `EnvironmentProfileDTO` (TypeBox) | Shared contract between frontend and C++ JSON parser |

---

## Implementation Design

### Core Interfaces

**`EnvironmentTimeline` — C++ header (`core/libs/world/include/ymir/world/EnvironmentTimeline.h`):**

```cpp
namespace ymir {

class EnvironmentTimeline {
public:
    struct UniformKeyframe { double t; double speed; double dirNaut; };
    struct WaveKeyframe {
        double t, Hs, Tp, dirNaut, gamma;
        enum class Spectrum { JONSWAP, PIERSON, REGULAR } spectrum;
    };

    void loadJson(const std::string& json);   // throws std::runtime_error on bad input
    void advanceStep(double t, Environment& env) const;
    bool empty() const noexcept;
    void reset() noexcept;

private:
    // Inner vector = one condition series; outer = multiple series composing vectorially
    std::vector<std::vector<UniformKeyframe>> currentSeries_;
    std::vector<std::vector<UniformKeyframe>> windSeries_;
    std::vector<WaveKeyframe>                 waveSeries_;   // Phase 1: single series
};

} // namespace ymir
```

**`EnvironmentProfileDTO` — TypeScript (`packages/types/src/environment.ts`):**

```typescript
export const EnvironmentConditionDTO = Type.Object({
    t:       Type.Number({ description: 'keyframe time [s], >= 0' }),
    speed:   Type.Number({ minimum: 0 }),
    dirNaut: Type.Number({ minimum: 0, maximum: 360 }),
})

export const WaveConditionDTO = Type.Object({
    t:        Type.Number(),
    Hs:       Type.Number({ minimum: 0 }),
    Tp:       Type.Number({ minimum: 0 }),
    dirNaut:  Type.Number({ minimum: 0, maximum: 360 }),
    spectrum: Type.Union([Type.Literal('JONSWAP'), Type.Literal('PIERSON'), Type.Literal('REGULAR')]),
    gamma:    Type.Number({ minimum: 0, default: 3.3 }),
})

export const EnvironmentProfileDTO = Type.Object({
    currentSeries: Type.Array(Type.Array(EnvironmentConditionDTO)),
    windSeries:    Type.Array(Type.Array(EnvironmentConditionDTO)),
    waveSeries:    Type.Array(WaveConditionDTO),  // Phase 1: single series (flat array)
})

export type EnvironmentProfile = Static<typeof EnvironmentProfileDTO>
```

**`EnvironmentStore` — Zustand (`apps/web/src/stores/environmentStore.ts`):**

```typescript
interface EnvironmentStore {
    currentSeries: EnvironmentConditionDTO[][];
    windSeries:    EnvironmentConditionDTO[][];
    waveSeries:    WaveConditionDTO[];

    addCurrentSeries:    () => void;
    removeCurrentSeries: (idx: number) => void;
    setCurrentKeyframe:  (seriesIdx: number, kfIdx: number, patch: Partial<EnvironmentConditionDTO>) => void;
    addWindSeries:       () => void;
    removeWindSeries:    (idx: number) => void;
    setWindKeyframe:     (seriesIdx: number, kfIdx: number, patch: Partial<EnvironmentConditionDTO>) => void;
    setWaveKeyframe:     (kfIdx: number, patch: Partial<WaveConditionDTO>) => void;
    addWaveKeyframe:     () => void;
    removeWaveKeyframe:  (idx: number) => void;

    hasConditions: () => boolean;
    toJson:        () => string;  // serialized EnvironmentProfileDTO → JSON
    reset:         () => void;
}
```

### Data Models

**JSON payload shape** (contract between `EnvironmentStore.toJson()` and `EnvironmentTimeline::loadJson()`):

```json
{
  "currentSeries": [
    [
      { "t": 0,   "speed": 0.5, "dirNaut": 90 },
      { "t": 900, "speed": 1.0, "dirNaut": 90 }
    ]
  ],
  "windSeries": [
    [{ "t": 0, "speed": 10.0, "dirNaut": 0 }]
  ],
  "waveSeries": [
    { "t": 0, "Hs": 2.0, "Tp": 10.0, "dirNaut": 270, "spectrum": "JONSWAP", "gamma": 3.3 }
  ]
}
```

Rules enforced in C++ parser:
- Arrays sorted by `t` (ascending); parser re-sorts on load.
- `speed >= 0`, `dirNaut` in [0, 360).
- `Hs >= 0`, `Tp > 0`, `gamma >= 0` (0 → auto JONSWAP formula).
- Empty arrays are valid (no-op for that type).

**`ScenarioDTO` extension** (`packages/types/src/scenario.ts`):

```typescript
export const ScenarioDTO = Type.Object({
  // ... existing fields unchanged ...
  environment: Type.Optional(EnvironmentProfileDTO),
})
```

`environment` is optional to preserve backward compatibility with existing scenarios that have no environmental conditions.

### Resolution Algorithm (C++)

**Keyframe interpolation** (implemented in `EnvironmentTimeline.cpp`):

```
interpolate(series, t):
  if series.empty()      → return { speed: 0, dirNaut: 0 }
  if t ≤ series.front().t → clamp to first keyframe
  if t ≥ series.back().t  → clamp to last keyframe
  find k1, k2 bracketing t
  alpha = (t - k1.t) / (k2.t - k1.t)
  speed   = k1.speed + alpha * (k2.speed - k1.speed)
  delta   = k2.dirNaut - k1.dirNaut
  if delta > 180:  delta -= 360
  if delta < -180: delta += 360
  dirNaut = k1.dirNaut + alpha * delta    // shortest-path angular interp
  return { speed, dirNaut }
```

**Vectorial composition** (N uniform series → single resultant):

```
composeCurrent(currentSeries_, t):
  vx = 0, vy = 0
  for each series:
    { speed, dirNaut } = interpolate(series, t)
    // "from" convention: add 180° to get "to" direction for vector
    toDir_math = (270 - dirNaut) * π/180   // nautical "from" → math "to"
    vx += speed * cos(toDir_math)
    vy += speed * sin(toDir_math)
  resultSpeed = hypot(vx, vy)
  resultDirNaut = atan2(-vy, -vx) * 180/π converted back to nautical "from"
  env.setCurrent(resultSpeed, resultDirNaut)
```

Wind follows identical logic. Wave uses single series; `interpolate(waveSeries_, t)` resolves Hs, Tp, dirNaut; `env.setSeaState(Hs, Tp, dirNaut)` called directly.

### Worker Message Protocol

New message type added to `simulation.worker.ts`:

```typescript
case 'loadEnvironment': {
    const json = (e.data as { type: string; json: string }).json
    simulation?.loadEnvironment(json)
    break
}
```

`simulationStore.start()` ordering (after `ready` received, before `start` sent):

```typescript
if (environmentStore.hasConditions()) {
    worker.postMessage({ type: 'loadEnvironment', json: environmentStore.toJson() })
}
worker.postMessage({ type: 'start', dt })
```

Same re-send after `reset()` and after `loadScenario` (scenario reconstruction creates new `YmirSimulation`, wiping WASM state).

---

## Impact Analysis

| Component | Impact | Description | Required Action |
|-----------|--------|-------------|-----------------|
| `core/libs/world/include/ymir/world/EnvironmentTimeline.h` | New | Timeline class header | Create |
| `core/libs/world/src/EnvironmentTimeline.cpp` | New | Resolution + interpolation logic | Create |
| `core/libs/world/include/ymir/world/World.h` | Modified | Add `EnvironmentTimeline timeline_` field + getter; declare updated `step()` | Modify |
| `core/libs/world/src/World.cpp` | Modified | Call `timeline_.advanceStep(time_, env_)` before domain loop | Modify |
| `core/src/wasm/YmirBindings.cpp` | Modified | Add `loadEnvironment(json)` to `YmirSimulation`; register in Embind | Modify |
| `packages/types/src/environment.ts` | New | `EnvironmentProfileDTO`, `EnvironmentConditionDTO`, `WaveConditionDTO` | Create |
| `packages/types/src/scenario.ts` | Modified | Add `environment?: EnvironmentProfileDTO` to `ScenarioDTO` | Modify |
| `apps/web/src/stores/environmentStore.ts` | New | Zustand store for UI condition state + JSON serialization | Create |
| `apps/web/src/workers/simulation.worker.ts` | Modified | Add `loadEnvironment` case | Modify |
| `apps/web/src/stores/simulationStore.ts` | Modified | Send `loadEnvironment` on start and reset | Modify |
| `apps/web/src/features/scenario-creator/components/EnvironmentConditionPanel.tsx` | New | Instructor UI for conditions | Create |
| `apps/web/src/features/scenario-creator/ScenarioCreatorPage.tsx` | Modified | Mount `EnvironmentConditionPanel` in layout | Modify |
| `core/tests/world/TestEnvironmentTimeline.cpp` | New | Unit tests for interpolation, composition, edge cases | Create |
| JSON parsing library in C++ core | Dependency | Verify nlohmann/json availability; add if missing | Verify |

---

## Testing Approach

### Unit Tests (C++)

File: `core/tests/world/TestEnvironmentTimeline.cpp`

Critical scenarios:
- **Empty timeline**: `advanceStep()` is a no-op; `Environment` remains at defaults.
- **Single keyframe**: any `t` returns that keyframe's values (no interpolation).
- **Two keyframes, t between them**: verify linear speed interpolation and shortest-path angular interpolation.
- **Angular wrap-around**: series [350°, t=0] → [10°, t=100]; at t=50 → 0°, not 180°.
- **Clamp before first keyframe**: t < 0 returns first keyframe.
- **Clamp after last keyframe**: t > max returns last keyframe.
- **Vectorial composition, two series**: current A (1.0 m/s from North) + current B (0.5 m/s from South) → resultant 0.5 m/s from North.
- **Vectorial composition, perpendicular**: two equal-speed perpendicular currents → 45° resultant.
- **Malformed JSON**: `loadJson()` throws `std::runtime_error` for missing required fields, negative speed, invalid spectrum string.
- **reset()**: `empty()` returns `true` after reset; `advanceStep()` is no-op.
- **Wave keyframe interpolation**: Hs and Tp interpolate linearly; direction shortest-path.

### Unit Tests (TypeScript)

File: `apps/web/src/stores/environmentStore.test.ts`

- `toJson()` produces valid JSON parseable by `JSON.parse`.
- `toJson()` output schema validates against `EnvironmentProfileDTO`.
- Adding a keyframe sorts series by `t`.
- `hasConditions()` returns false for empty store, true after adding any series.
- `reset()` clears all series.

### Integration Tests

- Start simulation with a 2-keyframe current timeline; observe `getState()` vessel drift direction changes after first keyframe elapses.
- Scenario with `environment` field survives save/load cycle without data loss (round-trip via `ScenarioDTO`).
- Empty `environment` field in `ScenarioDTO` produces no-op at WASM level.

---

## Development Sequencing

### Build Order

1. **Verify JSON parsing library** — confirm nlohmann/json (or equivalent) is already a dependency in `core/libs/world`; add if absent. No other dependencies.

2. **`EnvironmentTimeline` C++ class** — create header + implementation with `loadJson()` and `advanceStep()`. Depends on step 1 (JSON library) and existing `Environment` header.

3. **`World` integration** — add `EnvironmentTimeline timeline_` field to `World.h`; call `timeline_.advanceStep(time_, env_)` in `World::step()`. Depends on step 2.

4. **WASM binding** — add `loadEnvironment(json)` to `YmirSimulation` in `YmirBindings.cpp`; register in Embind. Depends on step 3.

5. **C++ tests** — `TestEnvironmentTimeline.cpp`. Depends on step 2 (can be written before World integration).

6. **`packages/types` — `environment.ts`** — create `EnvironmentProfileDTO` and related types. No C++ dependency; can run parallel to steps 2–5.

7. **`ScenarioDTO` extension** — add `environment?: EnvironmentProfileDTO` to `ScenarioDTO`. Depends on step 6.

8. **`EnvironmentStore` (Zustand)** — create store with `toJson()` serializer. Depends on step 6.

9. **Worker message handler** — add `loadEnvironment` case to `simulation.worker.ts`. Depends on step 4 (WASM exposes the method).

10. **`simulationStore` integration** — send `loadEnvironment` on start/reset. Depends on steps 8 and 9.

11. **`EnvironmentConditionPanel` component** — instructor UI. Depends on step 8.

12. **`ScenarioCreatorPage` integration** — mount panel in layout. Depends on step 11.

### Technical Dependencies

- **JSON parsing library in C++ core**: must be resolved before step 2. Candidate: [nlohmann/json](https://github.com/nlohmann/json) (header-only, MIT). Verify presence in `core/CMakeLists.txt` or `vcpkg.json`.
- **WASM rebuild**: steps 4 and 9 require a fresh WASM build before the worker can call `loadEnvironment`. Verify Docker-based Emscripten build pipeline works end-to-end before starting frontend integration.

---

## Technical Considerations

### Key Decisions

**C++ owns resolution (ADR-005)**
- Chosen: `EnvironmentTimeline` in C++, full timeline passed once.
- Rationale: simulation time is authoritative in C++; no clock drift between JS and WASM.
- Trade-off: JSON parsing dependency in C++ core.

**`loadEnvironment` as a separate worker message (ADR-006)**
- Chosen: distinct `loadEnvironment` message, sent after `ready` and before `start`.
- Rationale: decouples `EnvironmentStore` from `ScenarioStore`; avoids ordering hazards with `loadScenario`'s WASM reconstruction.
- Trade-off: two-message startup sequence requires enforced ordering in `simulationStore`.

**Phase 1: single wave series**
- `Environment::setSeaState()` accepts a single `(Hs, Tp, dir)` tuple. Multiple simultaneous wave states require physics engine changes (multiple `SeaState` slots and corresponding force model changes). Multi-wave composition is deferred to Phase 2.
- Phase 1: `waveSeries` in the JSON is a flat array of keyframes (not an array of series). Single wave condition interpolated over time.

**Anti-Corruption Layer for nautical degrees**
- `EnvironmentTimeline::advanceStep()` is the single conversion point from nautical "from" convention (stored in JSON and in `Environment`) to the math convention needed for vector arithmetic.
- `Environment` setters accept nautical degrees; `NavalDomain::buildContext()` calls `nautToBodyFrame()` for its own conversion. No conversion duplication.

### Known Risks

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| JSON schema drift between TypeScript serializer and C++ parser | Medium | `EnvironmentProfileDTO` is the canonical schema; C++ parser rejects unknown fields and missing required keys |
| Angular interpolation wraps incorrectly through 180° | Medium | Dedicated unit test for 350°→10° transition (see Testing section) |
| `loadEnvironment` sent after `start` (race) | Low | `simulationStore` sends messages synchronously: `loadEnvironment` first, then `start` — same pattern as existing actuator re-sync |
| `World::step()` time ordering: `advanceStep()` called before or after `time_ += dt` | Low | Call `advanceStep(time_, env_)` at the top of `step()` before domain iteration; `time_` increments after all domain steps |
| WASM rebuild not triggered when only C++ `world` lib changes | Low | Verify Docker build script includes `world` lib as a dependency of the WASM target |

---

## Architecture Decision Records

ADRs from PRD phase:
- [ADR-001: Phased Rollout — Uniform Before Grid Maps](adrs/adr-001.md) — Deliver uniform + keyframes in Phase 1; grid maps in Phase 2; real-time in Phase 3.
- [ADR-002: Homogeneous Condition Collections](adrs/adr-002.md) — No mixing uniform and grid within one collection.
- [ADR-003: Independent Evaluation + Vector Sum for Grid Maps](adrs/adr-003.md) — Per-grid evaluation at vessel position, summed; outside grid = zero.
- [ADR-004: Temporal Keyframe Evolution for All Condition Types](adrs/adr-004.md) — Unified keyframe model for current, wind, and wave.

ADRs from technical design:
- [ADR-005: C++ EnvironmentTimeline as Keyframe Resolution Layer](adrs/adr-005.md) — C++ owns interpolation and composition; frontend passes JSON once.
- [ADR-006: Worker `loadEnvironment` Message Protocol](adrs/adr-006.md) — Separate message type; sent after `ready`, before `start`.
