# Task Memory: task_01.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Verify and link nlohmann/json to `ymir_world` CMake target. Task complete.

## Important Decisions

- nlohmann/json v3.11.3 was already declared in `core/cmake/Dependencies.cmake` — no new library added.
- Added as PUBLIC link (not PRIVATE) because Task 2 will expose the header in `EnvironmentTimeline.h`, making downstream targets need transitive access.
- WASM CMakeLists (`CMakeLists.wasm.txt`) does NOT include `Dependencies.cmake`, so a separate FetchContent block was added there with the same version pin.

## Learnings

- The WASM build uses `core/CMakeLists.wasm.txt` (not a subdirectory CMakeLists), staged via `core/build-wasm.sh`. It only includes `CompilerWarnings.cmake` and `Emscripten.cmake` — any dependency shared with the native build must be explicitly repeated there.
- `ymir_world_tests` executable links `Ymir::World`, so nlohmann/json headers become available to world tests transitively through the PUBLIC link.

## Files / Surfaces

- `core/libs/world/CMakeLists.txt` — added `nlohmann_json::nlohmann_json` to PUBLIC link
- `core/CMakeLists.wasm.txt` — added FetchContent block for nlohmann/json v3.11.3
- `core/tests/world/TestJsonSmoke.cpp` — new; 5 smoke tests
- `core/tests/CMakeLists.txt` — added `TestJsonSmoke.cpp` to `ymir_world_tests`

## Errors / Corrections

None.

## Ready for Next Run

Task complete. 222/222 tests pass. Task 2 (EnvironmentTimeline) may proceed — `#include <nlohmann/json.hpp>` is available in both native and WASM builds via `ymir_world`.
