# Task Memory: task_08.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Implemented `DynamicVessel` aggregate root: `DynamicVessel.h` + `DynamicVessel.cpp`. Updated `libs/vessel/CMakeLists.txt` (added `DynamicVessel.cpp` to STATIC sources). Updated `libs/vessel/tests/CMakeLists.txt` (added `TestDynamicVessel.cpp`). Created `TestDynamicVessel.cpp` with 10 unit + 1 integration test.

## Important Decisions

- `DynamicVessel` in namespace `ymir::naval`; `VesselState` is `ymir::vessel::VesselState` → used fully-qualified name throughout.
- Default `controller_` initialized to `PrescribedController(PrescribedController::Config{})` — empty config = zero demands = drift mode.
- `setController()` takes `VesselController` by value, assigns via `operator=` — variant assignment is correct.

## Learnings

- `VesselState` namespace is `ymir::vessel`, not `ymir::naval` — must use qualified name in DynamicVessel since it lives in `ymir::naval`.
- `std::visit` with generic lambda `[&](auto& ctrl) { ctrl.update(...); }` works fine for the variant dispatch.
- Test fixture pattern: store `VesselConfig` as struct member, create `DynamicVessel` via `make()` to ensure config outlives vessel + thrusters/rudders.

## Files / Surfaces

- `libs/vessel/include/ymir/vessel/DynamicVessel.h` (new)
- `libs/vessel/src/DynamicVessel.cpp` (new)
- `libs/vessel/CMakeLists.txt` (added DynamicVessel.cpp line)
- `libs/vessel/tests/CMakeLists.txt` (added TestDynamicVessel.cpp line)
- `libs/vessel/tests/TestDynamicVessel.cpp` (new)

## Errors / Corrections

None — compiled cleanly on first build attempt with existing `build/` directory.

## Ready for Next Run

- task_09: `NavalSimulation` refactor — `registerVessel(bodyId, DynamicVessel& vessel, ThrustForces*, RudderForces*)` uses the same `DynamicVessel` interface implemented here.
- `DynamicVessel` is non-movable; NavalSimulation must hold a pointer/reference, not by value.
