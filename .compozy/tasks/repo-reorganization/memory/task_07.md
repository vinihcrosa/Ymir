# Task Memory: task_07.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot
- Move the JSON scenario reader out of `adapters/` into `libs/persistence/`, remove stale `ymir/adapters` includes from C++ headers/sources, and keep temporary pre-task_08 CMake wiring buildable.

## Important Decisions
- Task spec names non-`json/` ScenarioReader paths, but the repository and TechSpec use `adapters/include/ymir/adapters/json/ScenarioReader.h` -> `libs/persistence/include/ymir/persistence/json/ScenarioReader.h`. Follow the TechSpec/repository layout for the actual move.
- `_tasks.md` still marks task_04 and task_06 pending, but repository state already has simulation and world files under `libs/`; proceed from actual tree state and do not alter unrelated task statuses.

## Learnings
- The persistence source layout in this repo is `json/ScenarioReader.{h,cpp}` plus `json/Scenario.h` and `json/BodyDefinition.h`; moving only ScenarioReader would leave stale adapter include prefixes in the DTO headers and fail the task grep.
- No coverage CMake option/target or coverage instrumentation is currently present; `gcov` exists, but the build produced no `.gcno`/`.gcda` files, so the 80% coverage percentage cannot be measured in this run.

## Files / Surfaces
- Expected surfaces: `adapters/CMakeLists.txt`, JSON persistence headers/sources, `tests/adapters/TestScenarioReader.cpp`, and possibly temporary include directories on the old `ymir_adapters` target.
- Touched surfaces: moved `BodyDefinition.h`, `Scenario.h`, `ScenarioReader.h`, and `ScenarioReader.cpp` under `libs/persistence/.../json`; updated `tests/adapters/TestScenarioReader.cpp`; updated `adapters/CMakeLists.txt` temporary source/include wiring.

## Errors / Corrections
- Required verification passed: `cmake --build build`, `ctest --test-dir build`, `ctest --test-dir build -R ScenarioReader --output-on-failure`, and the stale adapter include grep returned no matches.

## Ready for Next Run
- Task 08 should replace the temporary `ymir_adapters` wiring with the real `ymir_persistence` target and can remove old adapter include exposure when legacy targets are retired.
