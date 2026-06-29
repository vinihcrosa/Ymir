---
status: completed
title: Integrate EnvironmentTimeline into World::step()
type: backend
complexity: medium
dependencies:
  - task_02
---

# Task 3: Integrate EnvironmentTimeline into World::step()

## Overview

Wires `EnvironmentTimeline` into `World` so that the resolved environment values are applied to `Environment` before any domain steps each tick. After this task, `NavalDomain::buildContext()` — which reads `env_` unchanged — automatically receives temporally resolved environmental forces.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST add `EnvironmentTimeline timeline_` as a private member of `World`.
- MUST expose a public `EnvironmentTimeline& timeline() noexcept` getter.
- MUST call `timeline_.advanceStep(time_, env_)` in `World::step()` AFTER `time_ += dt` and BEFORE the domain loop — this ordering ensures domains receive the environment resolved for the current tick's time.
- `advanceStep()` on an empty timeline MUST be a no-op (no change to `env_`); existing tests must still pass.
- MUST NOT change the interface or behavior of `Environment`, `NavalDomain`, or any force model.
- `WorldSnapshot` does NOT need to include timeline state.
- All existing `ymir_world_tests` MUST continue to pass after this change.
</requirements>

## Subtasks

- [x] 3.1 Add `#include <ymir/world/EnvironmentTimeline.h>` and `EnvironmentTimeline timeline_` member to `World.h`.
- [x] 3.2 Add `EnvironmentTimeline& timeline() noexcept` (and const variant) getter declaration to `World.h`.
- [x] 3.3 Implement the getter in `World.cpp`.
- [x] 3.4 Insert `timeline_.advanceStep(time_, env_)` call into `World::step()` at the correct position (after `time_ += dt`, before the domain loop).
- [x] 3.5 Add `world` tests verifying: empty timeline = no-op in step; loaded timeline updates env before domain step.

## Implementation Details

See TechSpec "System Architecture" component overview and "Known Risks" (time ordering) section.

Current `World::step()` in `core/libs/world/src/World.cpp`:
```cpp
void World::step(double dt)
{
    time_ += dt;
    coupling_.reset();
    for (auto& d : domains_) d->step(dt);
    coupling_.resolve();
}
```

After this task, it should call `timeline_.advanceStep(time_, env_)` between the `time_ += dt` line and `coupling_.reset()`.

### Relevant Files

- `core/libs/world/include/ymir/world/World.h` — add member and getter declaration
- `core/libs/world/src/World.cpp` — modify `step()` and add getter implementation
- `core/libs/world/CMakeLists.txt` — confirm `EnvironmentTimeline.cpp` is already listed (Task 2); no changes needed here
- `core/tests/world/TestEnvironment.cpp` — reference for test style
- `core/tests/CMakeLists.txt` — add new test file to `ymir_world_tests` executable

### Dependent Files

- `core/src/wasm/YmirBindings.cpp` — calls `world_->timeline().loadJson()` in Task 4
- All existing world tests — must continue to pass

### Related ADRs

- [ADR-005: C++ EnvironmentTimeline as Keyframe Resolution Layer](adrs/adr-005.md) — defines where `advanceStep()` is called and timing requirements

## Deliverables

- Updated `World.h` with `timeline_` member and getter
- Updated `World.cpp` with `advanceStep()` call in `step()`
- New tests in `core/tests/world/` verifying timeline integration
- Unit tests with 80%+ coverage **(REQUIRED)**
- Integration tests for `advanceStep()` + domain step ordering **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `World::step()` with empty timeline: `env.currentSpeed()` remains 0 after step (no regression)
  - [x] `World::step()` with single-keyframe current timeline (speed=1.5, dirNaut=90): after one step, `world.environment().currentSpeed() == 1.5`
  - [x] `World::timeline()` getter returns non-const reference usable for `loadJson()` call
  - [x] All existing `TestEnvironment.cpp` scenarios still pass unchanged
- Integration tests:
  - [x] Full `World` + StubDomain step: configure current via timeline, run one step, verify domain receives nonzero current speed (EnvCaptureDomain captures env_.currentSpeed() during step)
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- All existing `ymir_world_tests` pass without modification
- `World::step()` calls `timeline_.advanceStep()` before any domain's `step()` is called
- `YmirBindings.cpp` can call `world_->timeline().loadJson(json)` (compilation verified in Task 4)
