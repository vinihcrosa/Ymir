# Task Memory: task_02.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Implement `Thruster` entity in `libs/vessel/` — COMPLETE.

## Important Decisions

- `azimuthSpeed` interpreted as **deg/s** (not rad/s as documented in VesselConfig comment). State is stored in degrees (`_deg` suffix), so rate must also be in deg/s for a unit-consistent formula. Default changed from `0.087` to `5.0` in VesselConfig.
- Added `pitchRate` field (deg/s) to `VesselConfig::ThrusterConfig` — was missing, added with default `5.0`.
- Added `initialRPM` field to `VesselConfig::ThrusterConfig` — was missing, added with default `0.0`.
- Tests live in `libs/vessel/tests/TestThruster.cpp` with own `CMakeLists.txt`.
- `enable_testing()` moved before `add_subdirectory(libs/...)` in root CMakeLists.txt so `catch_discover_tests` registers correctly for in-tree lib tests.

## Learnings

- `catch_discover_tests` requires `enable_testing()` to have been called before the `add_subdirectory` that adds the test executable. Moved `enable_testing()` to before the libs block.
- Integration test uses `NavalContext` from `libs/vessel/` and `BodyState`/`ThrustForces` from `libs/physics/` — all fine since `ymir_vessel` links `ymir_physics`.

## Files / Surfaces

- `libs/vessel/include/ymir/vessel/Thruster.h` — new
- `libs/vessel/src/Thruster.cpp` — new
- `libs/vessel/tests/TestThruster.cpp` — new
- `libs/vessel/tests/CMakeLists.txt` — new
- `libs/vessel/CMakeLists.txt` — INTERFACE → STATIC, adds Thruster.cpp, adds tests subdir
- `libs/vessel/include/ymir/vessel/VesselConfig.h` — added `initialRPM`, `pitchRate`; changed `azimuthSpeed` default/comment
- `CMakeLists.txt` (root) — moved `enable_testing()` before lib subdirectories

## Errors / Corrections

- None.

## Ready for Next Run

Task complete. Diff ready for manual review (auto-commit disabled).
