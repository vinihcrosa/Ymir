# Workflow Memory

Keep only durable, cross-task context here. Do not duplicate facts that are obvious from the repository, PRD documents, or git history.

## Current State

- task_01 done: `libs/{common,physics,simulation,world,vessel,persistence}/{include/ymir/<ctx>,src}` skeleton exists. NOTE: nested subdirs (e.g. `common/math`) NOT pre-created — `mkdir -p` before `git mv`.
- task_02 done: math headers + PhysicalConstants moved to `libs/common/`; old `core/`+`naval/` CMake targets still active.
- task_08 done: root build now uses `libs/{common,physics,simulation,world,vessel,persistence}` and app stubs under `apps/`; old root `add_subdirectory(core|naval|adapters)` wiring is removed.
- task_08 moved shared `Types.h` to `libs/common/include/ymir/common/Types.h` so common math can remain dependency-free.
- task_09 done: tests now live under bounded-context directories and `tests/CMakeLists.txt` defines context-oriented test executables linked through `Ymir::*` targets.

## Shared Decisions

- Until task_08, moved header-only files were made resolvable by adding `$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/<ctx>/include>` to still-active old targets. Those old targets are no longer in the root build after task_08.
- task_03 temporarily exposed `core/include/ymir/core` through `ymir_core` and changed remaining `ymir/core/Simulation.h` includes to bare `<Simulation.h>` so the stale-core grep can be clean while Simulation remains unmoved. task_04 should replace these with `<ymir/simulation/Simulation.h>` when Simulation moves.
- task_04 added `libs/simulation/src/Simulation.cpp` to `ymir_core`, `libs/simulation/src/NavalSimulation.cpp` to `ymir_naval`, and exposed `libs/simulation/include` through `ymir_core` only as temporary pre-task_08 build wiring.
- task_05 added `libs/vessel/include` to the still-active `ymir_naval` target as temporary pre-task_08 build wiring for moved vessel headers.
- task_07 added `libs/persistence/include` and `libs/persistence/src/json/ScenarioReader.cpp` to the still-active `ymir_adapters` target as temporary pre-task_08 build wiring for moved persistence files.

## Shared Learnings

- Bulk include rewrite: `grep -rl <pat> ... | while IFS= read -r f; do sed -i '' ...; done`. A plain `for f in $(...)` mangles multiline lists into one sed arg.
- Moved headers keep internal includes pointing at not-yet-moved headers (e.g. common/math still includes `ymir/core/Types.h`). Each task only fixes paths in its own grep scope; cross-header fixes happen when the included file moves.
- Coverage is now available through `-DYMIR_ENABLE_COVERAGE=ON` and `cmake --build <coverage-build> --target ymir_coverage`; task_08 measured project `libs/` line coverage at 81.78%.
- On AppleClang/Homebrew SUNDIALS, fresh build directories may need explicit `OpenMP_ROOT=/opt/homebrew/opt/libomp` plus OpenMP flag/library cache hints for SUNDIALS/SuiteSparse discovery.

## Open Risks

- Task tracking drift remains: `_tasks.md` still marks task_04 and task_06 pending even though task_08 built against simulation/world files already present under `libs/`.
- `ymir_physics` currently exposes `libs/vessel/include` as an include-only bridge because force headers use vessel config enums/types while ADR-003 forbids a physics -> vessel target link. Future boundary cleanup should remove this bridge.

## Handoffs
