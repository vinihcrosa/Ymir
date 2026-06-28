# Task Memory: task_06.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Implemented `ManeuverController` — LOS waypoint following with two PIDs (heading + speed).

## Important Decisions

- Used `ymir::math::wrapToPi` from `AngleUtils.h` instead of a local copy — satisfies AGENTS.md "One AngleUtils namespace, used everywhere".
- SOG computed as `sqrt(u^2 + v^2)` from `ctx.state` body-frame velocities (not `speedToWater`, which is relative to current).
- Waypoint capture uses a `while` loop (not recursion) to handle the case where vessel starts inside multiple capture radii.
- PID struct declared in the header private section (as per techspec), implementations in `.cpp`.
- `wrapToPi` test validates directly against `ymir::math::wrapToPi` — not through the controller — because the test value (355°) would require placing a waypoint at a specific angle.

## Learnings

- `headingPid_` and `speedPid_` initialized in member-initializer-list from `cfg_` members — safe because `cfg_` is declared first.
- Integration test uses simplified "RPM = SOG" dynamics for testability; not a claim about real ship physics.

## Files / Surfaces

- `libs/vessel/include/ymir/vessel/controllers/ManeuverController.h` (new)
- `libs/vessel/src/controllers/ManeuverController.cpp` (new)
- `libs/vessel/CMakeLists.txt` (added `ManeuverController.cpp`)
- `libs/vessel/tests/TestManeuverController.cpp` (new)
- `libs/vessel/tests/CMakeLists.txt` (added `TestManeuverController.cpp`)

## Errors / Corrections

None.

## Ready for Next Run

Task complete. 9 new tests (29–37), all pass. 109/109 total suite green. No commit yet (auto-commit disabled).
