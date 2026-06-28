# Task Memory: task_03.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Implement `Rudder` entity in `libs/vessel/` — analogous to `Thruster` (task_02). Rate limiter via `angleSpeed` (rad/s), clamped to `±angleMaximum`. `toCommand()` → `RudderForces::RudderCommand`.

## Important Decisions

- No surprises — implementation followed TechSpec and Thruster pattern exactly.
- `RudderForces::RudderConfig::thrusterIdx = std::numeric_limits<std::size_t>::max()` used in integration test to disable slipstream lookup (required to avoid null-pointer path in `computeNaval`).
- Integration test sets `ctx.speedToWater[0] = 5.0` — rudder force is zero when surge speed is zero (early-return guard in `computeNaval`).

## Learnings

- `VesselConfig::RudderConfig::angleMaximum` field is plain `angleMaximum` (not `angleMaximum_rad`).
- `VesselConfig::RudderConfig::angleSpeed` is already in rad/s — no conversion needed.
- `RudderForces::RudderConfig` (physics struct) is DIFFERENT from `VesselConfig::RudderConfig` (vessel struct) — same naming pattern as ThrusterConfig.

## Files / Surfaces

- `libs/vessel/include/ymir/vessel/Rudder.h` — new
- `libs/vessel/src/Rudder.cpp` — new
- `libs/vessel/tests/TestRudder.cpp` — new (9 test cases: 8 unit + 1 integration)
- `libs/vessel/CMakeLists.txt` — added `src/Rudder.cpp`
- `libs/vessel/tests/CMakeLists.txt` — added `TestRudder.cpp`

## Errors / Corrections

None.

## Ready for Next Run

Task complete. 95/95 tests passing across full repo. Diff ready for manual commit.
