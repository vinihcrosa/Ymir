# Task Memory: task_08.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

- Completed: replaced old aggregate CMake wiring (`core`, `naval`, `adapters`) with six bounded-context targets plus app stubs, keeping tests buildable until task_09 reorganizes test files.

## Important Decisions

- Moved `Types.h` from `libs/physics/include/ymir/physics/Types.h` to `libs/common/include/ymir/common/Types.h` because it only defines shared `Vector6`/`Matrix6x6` aliases and `common` math headers already depend on it. This is required for `ymir_common` to be dependency-free.
- Existing physics force headers still expose vessel config enums/types. To keep the required link graph acyclic for task_08, `ymir_physics` exposes the vessel include directory without adding a target link to `ymir_vessel`.
- Existing simulation and persistence headers include adjacent contexts not listed in the task_08 link graph (`world`/`vessel` for simulation, `simulation` for persistence). CMake includes are kept narrow so existing code compiles while target links stay aligned with the task graph.
- `ymir_vessel` is an `INTERFACE` library because the current vessel context has only headers. This preserves the required alias and dependency graph without adding placeholder C++ solely to produce an archive.
- Added `YMIR_ENABLE_COVERAGE` and `ymir_coverage` target in the test CMake wiring so the task's coverage requirement can be verified without changing default builds.

## Learnings

- `_tasks.md` still marks task_04 and task_06 pending, but the actual files for simulation/world are present under `libs/` and later task memory references them as moved. Treat this as tracking drift for this run rather than missing source inputs.
- `ctest --test-dir build` must run after the build completes; running it in parallel with `cmake --build build` can catch Catch2's temporary `*_NOT_BUILT-*` discovery placeholders.
- Fresh AppleClang/Homebrew coverage configuration needed explicit OpenMP cache hints for SUNDIALS/SuiteSparse discovery, matching the already-populated normal build cache.
- Literal `cmake --build build -- --verbose` fails with this Make generator (`make: unrecognized option --verbose`); equivalent verbose evidence was collected with `cmake --build build --clean-first -- VERBOSE=1`.

## Files / Surfaces

- Root `CMakeLists.txt`
- `libs/{common,physics,simulation,world,vessel,persistence}/CMakeLists.txt`
- `apps/server/CMakeLists.txt`, `apps/fast-time/CMakeLists.txt`
- `tests/CMakeLists.txt`
- `libs/common/include/ymir/common/Types.h` plus include references from the old physics path

## Errors / Corrections

- Fixed `ymir_coverage` custom target argument escaping with `VERBATIM`; without it the shell parsed the coverage ignore regex parentheses.
- Tightened coverage ignore regex to exclude `_deps`, `tests`, and generated build headers; final project `libs/` line coverage was 81.78%.
- Fixed stale `ymir_core` wording in `libs/physics/src/naval_physics.cpp` during self-review.

## Ready for Next Run

- task_09 can link reorganized test executables against `Ymir::*` aliases already available from task_08.
- Final verification after all edits: normal configure/build with tests+apps passed; verbose clean rebuild via `VERBOSE=1` passed; `ctest --test-dir build --output-on-failure` passed 72/72; stale root/old-target grep checks passed; coverage target passed with 81.78% line coverage.
