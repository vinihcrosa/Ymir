# Task Memory: task_06.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot
- Add the missing BerthManeuverSystem registry-mode implementation and tests for task 6 while preserving the existing parametric tug path when no registry is set.

## Important Decisions
- Keep the existing public API already present in `BerthManeuverSystem.h`; implementation will add a small private helper to route per-tug forces either to `CouplingRegistry::writeForce()` or the legacy `tugForces_` accumulator.
- Avoid changing the simulation/vessel target dependency graph for production libraries; add only the simulation include path required for the `.cpp` include and link the vessel test target to `Ymir::Simulation` for registry-mode unit tests.
- Registry-mode tests use distinct observer consumer ids per tug to prove BSM writes each producer's individual force; this matches the current `CouplingRegistry` API where `consumedForce()` is keyed by consumer id and can otherwise aggregate multiple producers.

## Learnings
- Pre-change state is partial: `setCouplingRegistry()` and registry members already exist in the header, but the method is not implemented, `update()` has no registry write path, and `TugParametricForces` is not deprecated.
- Fresh configure in `build-task06` failed before project generation because Homebrew SUNDIALS pulled SuiteSparse/OpenMP and CMake could not find `OpenMP_C_FLAGS`; verification used the existing configured `build` and `build-coverage` directories.
- Coverage target reports total line coverage 83.29% and all coverage-run tests pass; some existing per-file/module line coverage remains below 80%.

## Files / Surfaces
- `libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h`
- `libs/vessel/src/controllers/BerthManeuverSystem.cpp`
- `libs/vessel/include/ymir/vessel/controllers/TugParametricForces.h`
- `libs/vessel/tests/TestBerthManeuverSystem.cpp`
- `libs/vessel/CMakeLists.txt`
- `libs/vessel/tests/CMakeLists.txt`

## Errors / Corrections
- Initial assertion death test inherited Catch2's signal handler in the child process and printed a failing child report despite parent exit 0; fixed by restoring `SIGABRT` default and redirecting child stdout/stderr to `/dev/null`.

## Ready for Next Run
- Task 6 implementation and verification are complete; no automatic commit was made.
