# Task Memory: task_09.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot
- Reorganize existing tests from `tests/core/` and `tests/naval/` into bounded-context test directories and rewrite `tests/CMakeLists.txt` to use `Ymir::*` targets.
- Baseline before edits: test sources still exist under `tests/core/` and `tests/naval/`; `tests/CMakeLists.txt` still references those paths and defines `ymir_naval_tests`.

## Important Decisions
- Place naval force tests under `tests/physics/forces/` per task 9.4, despite the TechSpec table's broader `tests/physics/Test*Forces.cpp` row.
- Keep `tests/physics/TestIntegrator.cpp` in the task-mandated physics location, but build it as `ymir_integrator_tests` linked to `Ymir::Simulation` because the existing file includes and exercises `Simulation`.

## Learnings
- `build` is not coverage-enabled; use the existing `build-coverage` directory for `ymir_coverage`.
- Coverage target emitted LLVM's "functions have mismatched data" warning but completed and reported total line coverage at 81.78%.

## Files / Surfaces
- Expected surfaces: `tests/CMakeLists.txt`, test source paths under `tests/{common,physics,physics/forces,simulation,world,persistence}`.
- Moved the adapter scenario reader test to `tests/persistence/TestScenarioReader.cpp` per the TechSpec test reorganization table.

## Errors / Corrections
- Initial `cmake --build build` failed because `ymir_physics_tests` linked only `Ymir::Physics` while `TestIntegrator.cpp` includes `<ymir/simulation/Simulation.h>`; corrected by splitting `TestIntegrator.cpp` into `ymir_integrator_tests` linked to `Ymir::Simulation`.

## Ready for Next Run
- Task verification completed: `cmake --build build`, `ctest --test-dir build --output-on-failure`, and `cmake --build build-coverage --target ymir_coverage` exited 0.
- `tests/core/` and `tests/naval/` are empty on disk after removing their `.gitkeep` files; task 10 can delete the empty directories if needed.
