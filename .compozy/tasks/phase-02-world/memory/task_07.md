# Task Memory: task_07.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

- Implement task_07: add `ymir::World`, nested `WorldSnapshot` types, move `CouplingRegistry` to `libs/world` to avoid a world/simulation CMake cycle, and add required World unit plus NavalDomain coupling integration tests.
- Pre-change signal: `World.h`, `WorldSnapshot.h`, `tests/world/TestWorld.cpp`, and `tests/simulation/TestNavalDomainCoupling.cpp` are absent.
- Completed: `World`, `WorldSnapshot`, registry relocation, World unit tests, and World-driven NavalDomain coupling integration test are implemented and verified.

## Important Decisions

- Follow task_07's explicit resolution to relocate `CouplingRegistry` from `ymir/simulation` to `ymir/world`; this overrides the older TechSpec diagram and keeps the final dependency DAG acyclic.
- `World` is non-copyable and non-movable. Defaulted moves would leave domains with stale pointers to the moved-from `Environment` and `CouplingRegistry`.

## Learnings

- Workspace already contains earlier phase changes as unstaged/untracked files; task_07 edits must avoid reverting or normalizing unrelated generated phase work.
- `World::step()` can verify reset-before-step by seeding a pending producer write before the tick; reset discards it while leaving the last resolved force available to domain force models.

## Files / Surfaces

- Planned surfaces: `libs/world`, `libs/simulation` coupling include paths/CMake, `libs/vessel` private include path for BSM registry implementation, `tests/CMakeLists.txt`, World and simulation tests, task tracking files.
- Touched task_07 surfaces: `libs/world/include/ymir/world/{World.h,WorldSnapshot.h,CouplingRegistry.h}`, `libs/world/src/{World.cpp,CouplingRegistry.cpp}`, `libs/world/CMakeLists.txt`, simulation/vessel registry include paths and CMake, `tests/world/{TestWorld.cpp,TestCouplingRegistry.cpp}`, `tests/simulation/TestNavalDomainCoupling.cpp`, `tests/CMakeLists.txt`, task tracking files.

## Errors / Corrections

- Initial build failed because `std::make_unique<RigidBody6DOF>(..., {}, ...)` could not deduce the empty `Vector6`; fixed by passing an explicit `qdot` vector.
- Self-review deleted `World` move operations to avoid invalidating domain service pointers.

## Ready for Next Run

- Verification evidence for task_07: `cmake --build build -j 8` passed; `./build/tests/ymir_world_tests` passed with 124 assertions in 39 cases; `./build/tests/ymir_simulation_tests "*NavalDomain coupling*"` passed with 777 assertions; `ctest --test-dir build --output-on-failure` passed 203/203; `cmake --build build-coverage --target ymir_coverage -j 8` passed with 200/200 tests and 84.80% line coverage.
