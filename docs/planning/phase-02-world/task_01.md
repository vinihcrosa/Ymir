---
status: completed
title: Environment class + NavalEnvironment deprecated
type: refactor
complexity: low
dependencies: []
---

# Task 1: Environment class + NavalEnvironment deprecated

## Overview

Introduce `Environment` in `libs/world/` as a proper encapsulated class with typed
setters and const getters, replacing the public-field plain struct `NavalEnvironment`.
Make `NavalEnvironment` a `[[deprecated]]` type alias pointing to `Environment` so
existing callers continue to compile without functional change. Update
`libs/world/CMakeLists.txt` to include the new source files.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `Environment` MUST be declared in `libs/world/include/ymir/world/Environment.h` in namespace `ymir`
- `Environment` MUST expose only typed setters (`setWind`, `setCurrent`, `setTide`, `setWaterDepth`, `setSeaState`) and const getters — no public fields
- `setWaterDepth(depth_m)` MUST assert `depth_m > 0` (debug assertion, not exception)
- `setWind` and `setCurrent` MUST assert `speed_ms >= 0`
- Default values MUST match `NavalEnvironment` defaults: `waterDepth = 100.0`, all others zero
- `NavalEnvironment.h` MUST be updated to contain `using NavalEnvironment [[deprecated("Use Environment")]] = Environment;`
- `libs/world/CMakeLists.txt` MUST be updated to compile `Environment.cpp` if implementation is non-trivial
- No existing caller of `NavalEnvironment` (field access or construction) may be broken — type alias ensures this
</requirements>

## Subtasks

- [ ] 1.1 Create `libs/world/include/ymir/world/Environment.h` with class declaration, setters, getters, and private fields
- [ ] 1.2 Create `libs/world/src/Environment.cpp` with setter implementations (validation assertions)
- [ ] 1.3 Update `libs/world/include/ymir/world/NavalEnvironment.h` to deprecated type alias
- [ ] 1.4 Update `libs/world/CMakeLists.txt` to add `Environment.cpp` as a source
- [ ] 1.5 Verify all existing tests that construct or use `NavalEnvironment` compile and pass without modification

## Implementation Details

See TechSpec "Core Interfaces — Environment" section for the exact class shape: private
`SpeedDir` nested struct, six private fields, and the setter/getter signatures.

`setSeaState` stores `Hs`, `Tp`, and `dir` for future use by `NavalDomain::buildContext()`
when sea state propagation to WaveForces is implemented. Fields exist but are not yet
consumed in Phase 2 (NavalDomain uses the wave spectrum configured at construction time).

The existing `NavalEnvironment` struct has fields accessed directly: `env.currentSpeed`,
`env.windSpeed`, etc. The deprecated alias `using NavalEnvironment = Environment` satisfies
the C++ type system but changes field access to getter calls. **This will break existing
callers that access fields directly.** A code-wide find-and-replace of field access patterns
is required:
- `env.currentSpeed` → `env.currentSpeed()`
- `env.windSpeed` → `env.windSpeed()`
- `env.currentDirectionNaut` → `env.currentDirectionNaut()`
- `env.windDirectionNaut` → `env.windDirectionNaut()`
- `env.waterDepth` → `env.waterDepth()`
- `env.tide` → `env.tide()`

### Relevant Files

- `libs/world/include/ymir/world/NavalEnvironment.h` — current struct; to become deprecated alias
- `libs/world/CMakeLists.txt` — must add new source
- `libs/simulation/src/NavalSimulation.cpp` — uses `NavalEnvironment` fields in `buildContext()`
- `libs/simulation/include/ymir/simulation/NavalSimulation.h` — holds `NavalEnvironment env_` member
- `libs/vessel/include/ymir/vessel/NavalContext.h` — has `waterDepth` and `tide` fields populated from NavalEnvironment
- `tests/simulation/TestNavalInfra.cpp` — constructs NavalEnvironment
- `tests/simulation/TestNavalIntegration.cpp` — uses NavalEnvironment fields

### Dependent Files

- `libs/simulation/include/ymir/simulation/NavalDomain.h` (task_05) — will accept `const Environment&`
- `libs/world/include/ymir/world/World.h` (task_07) — will own `Environment` instance

### Related ADRs

- [ADR-005: Environment como Classe com Setters Tipados; NavalEnvironment Deprecated](adrs/adr-005.md) — Decisão de encapsulamento e estratégia de deprecação

## Deliverables

- `libs/world/include/ymir/world/Environment.h` — new class
- `libs/world/src/Environment.cpp` — implementation with validation assertions
- `libs/world/include/ymir/world/NavalEnvironment.h` — updated to deprecated type alias
- `libs/world/CMakeLists.txt` — updated with new source
- All existing tests compile and pass after field-access migration
- Unit tests for Environment class **(REQUIRED)**

## Tests

- Unit tests:
  - [ ] `Environment` default-constructed has `waterDepth() == 100.0`, all others zero
  - [ ] `setWind(5.0, 270.0)` → `windSpeed() == 5.0`, `windDirectionNaut() == 270.0`
  - [ ] `setCurrent(2.0, 90.0)` → `currentSpeed() == 2.0`, `currentDirectionNaut() == 90.0`
  - [ ] `setTide(1.5)` → `tide() == 1.5`
  - [ ] `setWaterDepth(50.0)` → `waterDepth() == 50.0`
  - [ ] `setWaterDepth(0.0)` triggers assertion (death test or compile-time guard)
  - [ ] `setWind(-1.0, 0.0)` triggers assertion
  - [ ] `using NavalEnvironment = Environment`: code using `NavalEnvironment env; env.setWind(...)` compiles and works
- Integration tests:
  - [ ] Full NavalSimulation test suite passes unchanged after field-access migration
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- No public fields in `Environment`
- All direct field accesses on `NavalEnvironment` replaced with getter calls throughout codebase
- `NavalEnvironment` type alias compiles with `[[deprecated]]` without breaking existing code
