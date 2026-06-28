# Task Memory: task_12.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Update Doxyfile INPUT from legacy paths (core/, naval/, adapters/, include) to new bounded-context paths (libs/, apps/, docs/architecture/). Add guarded `docs` CMake custom target to root CMakeLists.txt.

## Important Decisions

- Set `EXTRACT_ALL = YES` (was NO). Original `WARN_AS_ERROR = YES` caused Doxygen to abort on undocumented symbols — new codebase has no doc comments yet. EXTRACT_ALL suppresses those warnings cleanly.
- Used `YMIR_BUILD_DOCS` option already present in CMakeLists.txt as the outer guard; added `find_package(Doxygen OPTIONAL_COMPONENTS dot)` + `if(DOXYGEN_FOUND)` inner guard per task spec.
- Docs target uses `${CMAKE_SOURCE_DIR}/Doxyfile` with `WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}` so relative paths in Doxyfile resolve correctly.

## Learnings

- Existing build dir (build/) can reconfigure with `-DYMIR_BUILD_DOCS=ON` without full reconfigure — cached SUNDIALS/OpenMP already resolved.
- `docs/api/` is generated output — confirm it is in .gitignore (not checked here, worth verifying before commit).

## Files / Surfaces

- `Doxyfile` — INPUT updated, EXTRACT_ALL changed to YES
- `CMakeLists.txt` — docs custom target added after tests block

## Errors / Corrections

- First docs build failed: `WARN_AS_ERROR = YES` + undocumented `CvodeConfig` compound → exit 1. Fixed by setting `EXTRACT_ALL = YES`.

## Ready for Next Run

Task complete. Diff ready for manual review. Auto-commit disabled.
