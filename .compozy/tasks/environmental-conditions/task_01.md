---
status: completed
title: Verify and add JSON parsing library to C++ core
type: infra
complexity: low
dependencies: []
---

# Task 1: Verify and add JSON parsing library to C++ core

## Overview

`EnvironmentTimeline::loadJson()` (Task 2) requires a JSON parsing library in the C++ `world` library. This task confirms whether a suitable library (nlohmann/json or equivalent) is already available and, if not, adds it as a dependency. All subsequent C++ tasks depend on this being resolved.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST confirm the JSON parsing library is available to `core/libs/world` targets before Task 2 begins.
- MUST use a library already present in the project if one exists (avoid adding duplicate dependencies).
- MUST NOT introduce a library that is incompatible with the Emscripten/WASM build target.
- SHOULD prefer nlohmann/json (header-only, MIT) if no library is already present.
- Chosen library MUST support parsing a `std::string` into a structured object without external I/O.
</requirements>

## Subtasks

- [x] 1.1 Search CMakeLists.txt, vcpkg.json, and conanfile.txt (if any) in `core/` for existing JSON library references.
- [x] 1.2 If nlohmann/json or another suitable library is already declared, confirm it is linked to `world` target and stop.
- [x] 1.3 If absent, add nlohmann/json as a dependency in the build system and link it to the `world` library target.
- [x] 1.4 Verify the WASM build pipeline (Docker/Emscripten CMake) can find and compile with the chosen library.
- [x] 1.5 Write a minimal smoke test (in CMake CTest or existing test runner) that parses `{"ok":true}` and asserts the value.

## Implementation Details

See TechSpec "Technical Dependencies" section for the rationale and candidate library.

The `world` library CMakeLists.txt must link the JSON library to the `ymir_world` (or equivalent) target. No changes to `Environment.h`, `World.h`, or any existing source file should be needed at this stage.

### Relevant Files

- `core/libs/world/CMakeLists.txt` — target definition for the `world` library; add JSON dependency here
- `core/CMakeLists.txt` — root CMake; may already declare JSON via FetchContent or find_package
- `vcpkg.json` or `conanfile.txt` (project root or `core/`) — package manifest if present
- `core/src/wasm/CMakeLists.txt` — WASM build target; must also see the library

### Dependent Files

- `core/libs/world/include/ymir/world/EnvironmentTimeline.h` — will `#include` the JSON library header (Task 2)
- `core/libs/world/src/EnvironmentTimeline.cpp` — will call the JSON parsing API (Task 2)

### Related ADRs

- [ADR-005: C++ EnvironmentTimeline as Keyframe Resolution Layer](adrs/adr-005.md) — identifies JSON parsing as a required dependency

## Deliverables

- Build system updated to provide JSON library to `world` and WASM targets
- Smoke test confirming JSON library is accessible and parses basic input
- Unit tests with 80%+ coverage **(REQUIRED)**
- Documentation comment in CMakeLists.txt identifying the chosen library and version

## Tests

- Unit tests:
  - [x] Smoke: `nlohmann::json::parse("{\"ok\":true}").at("ok").get<bool>()` returns `true` (or equivalent for chosen library)
  - [x] Smoke: parsing malformed JSON `"{broken"` throws the library's parse exception
- Integration tests:
  - [ ] CMake build of `world` library succeeds with JSON header included in a new `.cpp` file
  - [ ] WASM Docker build succeeds after adding the library
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `core/libs/world` CMake target compiles with `#include <nlohmann/json.hpp>` (or equivalent) without error
- WASM Docker build is green after the change
- Zero changes to existing `Environment.h`, `World.h`, or test files
