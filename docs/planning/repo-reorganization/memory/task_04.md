# Task Memory: task_04.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot
- Move `Simulation` and `NavalSimulation` headers/sources into `libs/simulation/`, replace stale/bare includes with `ymir/simulation/...`, and keep legacy CMake targets compiling until task_08 creates real bounded-context targets.

## Important Decisions
- Preserve the temporary legacy-target build pattern from earlier tasks: update `ymir_core`/`ymir_naval` source paths and include directories, but do not add a `ymir_simulation` target in this task.

## Learnings
- Pre-change stale include signal: `rg "ymir/core/Simulation|ymir/naval/NavalSimulation|<Simulation\\.h>|<NavalSimulation\\.h>"` found bare `Simulation.h` includes plus old `ymir/naval/NavalSimulation.h` includes in source/tests/adapters.
- `cmake --build build` and `ctest --test-dir build` pass after the move; the exact stale include greps return no matches.
- Coverage remains unverifiable in this repo state: no coverage flags/data files exist, no coverage config is present in CMake files, and `cmake --build build --target ExperimentalCoverage` fails looking for `build/tests/CMakeFiles/CTestScript.cmake`.

## Files / Surfaces
- Touched surfaces: `core/CMakeLists.txt`, `naval/CMakeLists.txt`, `libs/simulation/include/ymir/simulation/*`, `libs/simulation/src/*`, simulation/naval tests, and `adapters/include/ymir/adapters/json/ScenarioReader.h`.

## Errors / Corrections
- Attempted parallel `git mv` operations caused transient `.git/index.lock` contention; two moves succeeded and the remaining two were completed after confirming the lock was gone.
- Coverage target failure is not fixed here because wiring coverage infrastructure is outside task_04's file-move scope.

## Ready for Next Run
- Implementation and build/test verification are done, but task tracking should not be marked fully completed unless the workflow accepts the existing coverage-infrastructure gap or a later task wires coverage.
