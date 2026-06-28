# Task Memory: task_06.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot
- Move wave engine headers/sources and `NavalEnvironment` from legacy `naval/` paths into `libs/world/`, rewriting includes to `ymir/world/...` while preserving temporary pre-task_08 CMake wiring.

## Important Decisions
- No new `ymir_world` target will be introduced in this task; existing `ymir_naval` remains the build carrier until task_08.
- Because `naval/src/NavalEnvironment.cpp` does not exist in the repository, only the header was moved; no placeholder implementation file was created.

## Learnings
- Baseline before edits: wave files still exist under `naval/include/ymir/naval/wave` and `naval/src/wave`; stale includes remain in wave sources/headers, wave tests, `TestNavalInfra`, and `libs/simulation/include/ymir/simulation/NavalSimulation.h`.
- Validation after edits: required stale include greps return no matches, `cmake --build build` succeeds, `ctest --test-dir build` passes 72/72, and `ctest --test-dir build -R Wave` passes 6/6.
- Coverage validation is still blocked by missing instrumentation: `ctest --test-dir build -T Coverage` reports "Cannot find any coverage files. Ignoring Coverage request."

## Files / Surfaces
- Planned surfaces: `naval/CMakeLists.txt`, moved wave files, moved `NavalEnvironment` files, simulation/persistence/tests include consumers as discovered by grep.
- Touched surfaces: `libs/world/include/ymir/world/{NavalEnvironment.h,wave/*.h}`, `libs/world/src/wave/*.cpp`, `naval/CMakeLists.txt`, wave/environment include consumers in tests and `libs/simulation/include/ymir/simulation/NavalSimulation.h`.

## Errors / Corrections
- Task spec expected `naval/src/NavalEnvironment.cpp`, but `find . -path '*NavalEnvironment*' -type f` found only the header before moves.

## Ready for Next Run
- Available-file implementation subtasks are done; 6.4 remains unchecked because `NavalEnvironment.cpp` is absent. Task status was left pending because the required 80% coverage criterion could not be evidenced with the current build setup.
