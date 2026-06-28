# TechSpec: Ymir Repository Reorganization

## Executive Summary

The repository is reorganized in two sequential commits. The first moves all source files from `core/`, `naval/`, and `adapters/` into six bounded-context directories under `libs/`, updates every `#include` path and every `CMakeLists.txt`, and verifies the build passes. The second creates the documentation layer (`docs/architecture/`, updated `AGENTS.md`, updated `README.md`, Doxygen wired into CMake) and deletes `.docs/`.

Primary trade-off: new include namespaces (`ymir/physics/`, `ymir/world/`, etc.) touch every test file and many source files — approximately 50 include directives — but make the namespace self-documenting and consistent with the directory name and CMake target name.

---

## System Architecture

### Component Overview

After reorganization, the repository has the following structure:

```
ymir/
├── apps/
│   ├── server/            CMakeLists.txt + .gitkeep (placeholder)
│   └── fast-time/         CMakeLists.txt + .gitkeep (placeholder)
├── libs/
│   ├── common/            ymir_common — math utilities, physical constants, shared types
│   ├── physics/           ymir_physics — bodies, force model, integrator, all force implementations
│   ├── simulation/        ymir_simulation — Simulation, NavalSimulation
│   ├── world/             ymir_world — wave engine, environment
│   ├── vessel/            ymir_vessel — VesselConfig, NavalContext
│   └── persistence/       ymir_persistence — JSON scenario reader
├── services/              .gitkeep (reserved, empty)
├── tests/
│   ├── common/            tests for ymir_common
│   ├── physics/           tests for ymir_physics
│   ├── simulation/        tests for ymir_simulation
│   ├── world/             tests for ymir_world
│   └── persistence/       tests for ymir_persistence
├── docs/
│   ├── README.md
│   ├── architecture/      module wiki (Markdown)
│   ├── guides/            building.md, contributing.md
│   └── adr/               existing ADRs
├── cmake/
├── CMakeLists.txt
├── AGENTS.md
└── README.md
```

### CMake Dependency Graph

```
ymir_common
    └── (no internal deps)

ymir_physics
    ├── ymir_common
    └── SUNDIALS::cvode

ymir_simulation
    ├── ymir_physics
    └── ymir_common

ymir_world
    ├── ymir_physics
    └── ymir_common

ymir_vessel
    ├── ymir_physics
    └── ymir_common

ymir_persistence
    ├── ymir_physics
    ├── ymir_world
    ├── ymir_vessel
    └── nlohmann_json::nlohmann_json
```

---

## Implementation Design

### Core Interfaces

New CMake target pattern (same for all six libs):

```cmake
# libs/physics/CMakeLists.txt
add_library(ymir_physics STATIC
    src/RigidBody6DOF.cpp
    src/integrator/CvodeIntegrator.cpp
    src/forces/InertialForces.cpp
    # ... all force .cpp files
)
add_library(Ymir::Physics ALIAS ymir_physics)

target_include_directories(ymir_physics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ymir_physics
    PUBLIC ymir_common SUNDIALS::cvode
)
```

### File Mapping — `core/` → `libs/`

| Source (current) | Destination | New `#include` |
|------------------|-------------|----------------|
| `core/include/ymir/core/AbstractBody.h` | `libs/physics/include/ymir/physics/AbstractBody.h` | `<ymir/physics/AbstractBody.h>` |
| `core/include/ymir/core/BodyState.h` | `libs/physics/include/ymir/physics/BodyState.h` | `<ymir/physics/BodyState.h>` |
| `core/include/ymir/core/ForceModel.h` | `libs/physics/include/ymir/physics/ForceModel.h` | `<ymir/physics/ForceModel.h>` |
| `core/include/ymir/core/Forces.h` | `libs/physics/include/ymir/physics/Forces.h` | `<ymir/physics/Forces.h>` |
| `core/include/ymir/core/RigidBody6DOF.h` | `libs/physics/include/ymir/physics/RigidBody6DOF.h` | `<ymir/physics/RigidBody6DOF.h>` |
| `core/include/ymir/core/Types.h` | `libs/physics/include/ymir/physics/Types.h` | `<ymir/physics/Types.h>` |
| `core/include/ymir/core/integrator/CvodeConfig.h` | `libs/physics/include/ymir/physics/integrator/CvodeConfig.h` | `<ymir/physics/integrator/CvodeConfig.h>` |
| `core/include/ymir/core/integrator/CvodeIntegrator.h` | `libs/physics/include/ymir/physics/integrator/CvodeIntegrator.h` | `<ymir/physics/integrator/CvodeIntegrator.h>` |
| `core/include/ymir/core/math/AngleUtils.h` | `libs/common/include/ymir/common/math/AngleUtils.h` | `<ymir/common/math/AngleUtils.h>` |
| `core/include/ymir/core/math/Interpolation.h` | `libs/common/include/ymir/common/math/Interpolation.h` | `<ymir/common/math/Interpolation.h>` |
| `core/include/ymir/core/math/LinearAlgebra.h` | `libs/common/include/ymir/common/math/LinearAlgebra.h` | `<ymir/common/math/LinearAlgebra.h>` |
| `core/include/ymir/core/Simulation.h` | `libs/simulation/include/ymir/simulation/Simulation.h` | `<ymir/simulation/Simulation.h>` |
| `core/src/core.cpp` | `libs/physics/src/physics.cpp` | — |
| `core/src/RigidBody6DOF.cpp` | `libs/physics/src/RigidBody6DOF.cpp` | — |
| `core/src/Simulation.cpp` | `libs/simulation/src/Simulation.cpp` | — |
| `core/src/integrator/CvodeIntegrator.cpp` | `libs/physics/src/integrator/CvodeIntegrator.cpp` | — |

### File Mapping — `naval/` → `libs/`

| Source (current) | Destination | New `#include` |
|------------------|-------------|----------------|
| `naval/include/ymir/naval/NavalContext.h` | `libs/vessel/include/ymir/vessel/NavalContext.h` | `<ymir/vessel/NavalContext.h>` |
| `naval/include/ymir/naval/NavalEnvironment.h` | `libs/world/include/ymir/world/NavalEnvironment.h` | `<ymir/world/NavalEnvironment.h>` |
| `naval/include/ymir/naval/NavalForceModel.h` | `libs/physics/include/ymir/physics/NavalForceModel.h` | `<ymir/physics/NavalForceModel.h>` |
| `naval/include/ymir/naval/NavalSimulation.h` | `libs/simulation/include/ymir/simulation/NavalSimulation.h` | `<ymir/simulation/NavalSimulation.h>` |
| `naval/include/ymir/naval/PhysicalConstants.h` | `libs/common/include/ymir/common/PhysicalConstants.h` | `<ymir/common/PhysicalConstants.h>` |
| `naval/include/ymir/naval/VesselConfig.h` | `libs/vessel/include/ymir/vessel/VesselConfig.h` | `<ymir/vessel/VesselConfig.h>` |
| `naval/include/ymir/naval/forces/CurrentForces.h` | `libs/physics/include/ymir/physics/forces/CurrentForces.h` | `<ymir/physics/forces/CurrentForces.h>` |
| `naval/include/ymir/naval/forces/DampingForces.h` | `libs/physics/include/ymir/physics/forces/DampingForces.h` | `<ymir/physics/forces/DampingForces.h>` |
| `naval/include/ymir/naval/forces/InertialForces.h` | `libs/physics/include/ymir/physics/forces/InertialForces.h` | `<ymir/physics/forces/InertialForces.h>` |
| `naval/include/ymir/naval/forces/RestoringForces.h` | `libs/physics/include/ymir/physics/forces/RestoringForces.h` | `<ymir/physics/forces/RestoringForces.h>` |
| `naval/include/ymir/naval/forces/RudderForces.h` | `libs/physics/include/ymir/physics/forces/RudderForces.h` | `<ymir/physics/forces/RudderForces.h>` |
| `naval/include/ymir/naval/forces/SquatForces.h` | `libs/physics/include/ymir/physics/forces/SquatForces.h` | `<ymir/physics/forces/SquatForces.h>` |
| `naval/include/ymir/naval/forces/ThrustForces.h` | `libs/physics/include/ymir/physics/forces/ThrustForces.h` | `<ymir/physics/forces/ThrustForces.h>` |
| `naval/include/ymir/naval/forces/TugForces.h` | `libs/physics/include/ymir/physics/forces/TugForces.h` | `<ymir/physics/forces/TugForces.h>` |
| `naval/include/ymir/naval/forces/WindForces.h` | `libs/physics/include/ymir/physics/forces/WindForces.h` | `<ymir/physics/forces/WindForces.h>` |
| `naval/include/ymir/naval/wave/WaveComponent.h` | `libs/world/include/ymir/world/wave/WaveComponent.h` | `<ymir/world/wave/WaveComponent.h>` |
| `naval/include/ymir/naval/wave/WaveForces.h` | `libs/world/include/ymir/world/wave/WaveForces.h` | `<ymir/world/wave/WaveForces.h>` |
| `naval/include/ymir/naval/wave/WaveSpectrum.h` | `libs/world/include/ymir/world/wave/WaveSpectrum.h` | `<ymir/world/wave/WaveSpectrum.h>` |
| `naval/src/naval.cpp` | `libs/physics/src/naval_physics.cpp` | — |
| `naval/src/NavalForceModel.cpp` | `libs/physics/src/NavalForceModel.cpp` | — |
| `naval/src/NavalSimulation.cpp` | `libs/simulation/src/NavalSimulation.cpp` | — |
| `naval/src/forces/CurrentForces.cpp` | `libs/physics/src/forces/CurrentForces.cpp` | — |
| `naval/src/forces/DampingForces.cpp` | `libs/physics/src/forces/DampingForces.cpp` | — |
| `naval/src/forces/InertialForces.cpp` | `libs/physics/src/forces/InertialForces.cpp` | — |
| `naval/src/forces/RestoringForces.cpp` | `libs/physics/src/forces/RestoringForces.cpp` | — |
| `naval/src/forces/RudderForces.cpp` | `libs/physics/src/forces/RudderForces.cpp` | — |
| `naval/src/forces/SquatForces.cpp` | `libs/physics/src/forces/SquatForces.cpp` | — |
| `naval/src/forces/ThrustForces.cpp` | `libs/physics/src/forces/ThrustForces.cpp` | — |
| `naval/src/forces/TugForces.cpp` | `libs/physics/src/forces/TugForces.cpp` | — |
| `naval/src/forces/WindForces.cpp` | `libs/physics/src/forces/WindForces.cpp` | — |
| `naval/src/wave/WaveComponent.cpp` | `libs/world/src/wave/WaveComponent.cpp` | — |
| `naval/src/wave/WaveForces.cpp` | `libs/world/src/wave/WaveForces.cpp` | — |
| `naval/src/wave/WaveSpectrum.cpp` | `libs/world/src/wave/WaveSpectrum.cpp` | — |

### File Mapping — `adapters/` and `applications/`

| Source (current) | Destination | New `#include` |
|------------------|-------------|----------------|
| `adapters/include/ymir/adapters/json/BodyDefinition.h` | `libs/persistence/include/ymir/persistence/json/BodyDefinition.h` | `<ymir/persistence/json/BodyDefinition.h>` |
| `adapters/include/ymir/adapters/json/Scenario.h` | `libs/persistence/include/ymir/persistence/json/Scenario.h` | `<ymir/persistence/json/Scenario.h>` |
| `adapters/include/ymir/adapters/json/ScenarioReader.h` | `libs/persistence/include/ymir/persistence/json/ScenarioReader.h` | `<ymir/persistence/json/ScenarioReader.h>` |
| `adapters/src/adapters.cpp` | `libs/persistence/src/persistence.cpp` | — |
| `adapters/src/json/ScenarioReader.cpp` | `libs/persistence/src/json/ScenarioReader.cpp` | — |
| `applications/CMakeLists.txt` | `apps/server/CMakeLists.txt` | — |

### Test Reorganization

Tests move to mirror the new lib structure. Test executables are split per bounded context.

| Current test file | New location | Links to |
|------------------|--------------|----------|
| `tests/core/TestMath.cpp` | `tests/common/TestMath.cpp` | `ymir_common` |
| `tests/core/TestForces.cpp` | `tests/physics/TestForces.cpp` | `ymir_physics` |
| `tests/core/TestBody.cpp` | `tests/physics/TestBody.cpp` | `ymir_physics` |
| `tests/core/TestForceModel.cpp` | `tests/physics/TestForceModel.cpp` | `ymir_physics` |
| `tests/core/TestIntegrator.cpp` | `tests/physics/TestIntegrator.cpp` | `ymir_physics` |
| `tests/core/TestSimulationIntegration.cpp` | `tests/simulation/TestSimulationIntegration.cpp` | `ymir_simulation` |
| `tests/naval/TestNavalInfra.cpp` | `tests/simulation/TestNavalInfra.cpp` | `ymir_simulation` |
| `tests/naval/Test*Forces.cpp` (9 files) | `tests/physics/Test*Forces.cpp` | `ymir_physics` |
| `tests/naval/TestWaveSpectrum.cpp` | `tests/world/TestWaveSpectrum.cpp` | `ymir_world` |
| `tests/naval/TestWaveForces.cpp` | `tests/world/TestWaveForces.cpp` | `ymir_world` |
| `tests/naval/TestNavalIntegration.cpp` | `tests/simulation/TestNavalIntegration.cpp` | `ymir_simulation` |
| `tests/adapters/TestScenarioReader.cpp` | `tests/persistence/TestScenarioReader.cpp` | `ymir_persistence` |
| `tests/SmokeTest.cpp` | `tests/SmokeTest.cpp` (stays at root of tests/) | `ymir_physics` |

New test executables in `tests/CMakeLists.txt`:
- `ymir_common_tests` — links `ymir_common`
- `ymir_physics_tests` — links `ymir_physics`
- `ymir_simulation_tests` — links `ymir_simulation`
- `ymir_world_tests` — links `ymir_world`
- `ymir_persistence_tests` — links `ymir_persistence`

### Root `CMakeLists.txt` Changes

```cmake
# Remove:
add_subdirectory(core)
add_subdirectory(naval)
add_subdirectory(adapters)

# Add:
add_subdirectory(libs/common)
add_subdirectory(libs/physics)
add_subdirectory(libs/simulation)
add_subdirectory(libs/world)
add_subdirectory(libs/vessel)
add_subdirectory(libs/persistence)

if(YMIR_BUILD_APPS)
    add_subdirectory(apps/server)
endif()

# Add docs target:
if(YMIR_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    add_subdirectory(docs)
endif()
```

### Doxygen Configuration

Update `Doxyfile` to scan new paths:

```
INPUT = libs/common/include \
        libs/physics/include \
        libs/simulation/include \
        libs/world/include \
        libs/vessel/include \
        libs/persistence/include

OUTPUT_DIRECTORY = docs/generated
```

Add `docs/CMakeLists.txt`:
```cmake
doxygen_add_docs(docs
    ${CMAKE_SOURCE_DIR}/libs
    COMMENT "Generating API documentation"
)
```

Add `docs/generated/` to `.gitignore`.

### Documentation Files to Create

`docs/README.md` — index linking to all architecture pages and guides.

`docs/architecture/overview.md` — bounded context map, data flow per tick, module responsibilities summary.

`docs/architecture/physics.md` — content from `.prds/modules/physics.md` (already written).

`docs/architecture/world.md` — content from `.prds/modules/world.md`.

`docs/architecture/vessel.md` — content from `.prds/modules/vessel.md`.

`docs/architecture/simulation.md` — content from `.prds/modules/simulation.md`.

`docs/architecture/infrastructure.md` — content from `.prds/modules/infrastructure.md`.

`docs/guides/building.md` — prerequisites (CMake 3.16+, SUNDIALS, nlohmann_json, Catch2), configure and build commands, run tests command, generate docs command.

`docs/guides/contributing.md` — coding standards (from AGENTS.md), commit conventions, where to add new files per bounded context.

---

## Impact Analysis

| Component | Impact Type | Description and Risk | Required Action |
|-----------|-------------|---------------------|-----------------|
| `core/` directory | Deprecated | All files move to `libs/physics/`, `libs/simulation/`, `libs/common/` | Delete after move |
| `naval/` directory | Deprecated | All files split across `libs/physics/`, `libs/world/`, `libs/vessel/`, `libs/simulation/` | Delete after move |
| `adapters/` directory | Deprecated | All files move to `libs/persistence/` | Delete after move |
| `applications/` directory | Deprecated | CMakeLists moves to `apps/server/` | Delete after move |
| `include/` directory | Deprecated | Empty — delete | Delete |
| `.docs/` directory | Deprecated | Legacy docs — delete | Delete |
| All test files (15 files) | Modified | Include paths updated; files move to new subdirectories | Update every `#include` |
| All source `.cpp` files (19 files) | Modified | Include paths updated to new namespaces | Update every `#include` |
| All header `.h` files (29 files) | Modified | Internal includes updated; files renamed in place | Update cross-header includes |
| `CMakeLists.txt` (root) | Modified | Replace 3 `add_subdirectory` calls with 6 | Rewrite relevant section |
| `Doxyfile` | Modified | Update `INPUT` paths to point to `libs/` | Update `INPUT` and `OUTPUT_DIRECTORY` |
| `AGENTS.md` | Modified | Update all path references | Full rewrite |
| `README.md` | Modified | Update structure overview and build instructions | Full rewrite |
| `.github/workflows/ci.yml` | Modified (low risk) | Verify no hardcoded paths to `core/` or `naval/` | Inspect and update if needed |

---

## Testing Approach

### Verification After Commit 1 (File Move)

```bash
# 1. Verify no stale includes remain
grep -r "ymir/core" libs/ tests/ apps/ --include="*.h" --include="*.cpp"
grep -r "ymir/naval" libs/ tests/ apps/ --include="*.h" --include="*.cpp"
grep -r "ymir/adapters" libs/ tests/ apps/ --include="*.h" --include="*.cpp"
# All three commands must return zero results.

# 2. Build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DYMIR_BUILD_TESTS=ON
cmake --build build

# 3. Run all tests
ctest --test-dir build --output-on-failure
```

Success criterion: all commands exit zero.

### Verification After Commit 2 (Documentation)

```bash
# 1. Generate Doxygen
cmake -B build -DYMIR_BUILD_DOCS=ON
cmake --build build --target docs
# Doxygen must exit without errors.

# 2. Confirm no legacy folders remain
ls core/ naval/ adapters/ applications/ include/ .docs/ 2>&1
# All six must report "No such file or directory"
```

---

## Development Sequencing

### Build Order

1. **Create `libs/` directory skeleton** — create all six `libs/<context>/` directories with `include/ymir/<context>/` and `src/` subdirectories. No dependencies.

2. **Move `core/math/` → `libs/common/`** — no internal cross-includes; math utilities have no dependency on other Ymir headers. Depends on step 1.

3. **Move `core/` body/force/integrator files → `libs/physics/`** — headers in this group include `ymir/core/math/` which is now `ymir/common/math/`. Update those includes. Depends on step 2.

4. **Move `core/Simulation.h/.cpp` → `libs/simulation/`** — Simulation includes `RigidBody6DOF.h` now at `ymir/physics/`. Update. Depends on step 3.

5. **Move `naval/forces/` → `libs/physics/`** — force headers include `ymir/core/BodyState.h` (now `ymir/physics/BodyState.h`) and `ymir/naval/NavalContext.h` (now `ymir/vessel/NavalContext.h`). Update. Depends on steps 2–3.

6. **Move `naval/VesselConfig.h`, `NavalContext.h` → `libs/vessel/`** — headers include `ymir/core/Types.h` (now `ymir/physics/Types.h`). Update. Depends on step 3.

7. **Move `naval/wave/` + `NavalEnvironment.h` → `libs/world/`** — wave headers include `ymir/naval/NavalContext.h` (now `ymir/vessel/NavalContext.h`) and `ymir/core/BodyState.h`. Update. Depends on steps 3 and 6.

8. **Move `naval/NavalForceModel.h/.cpp`, `NavalSimulation.h/.cpp` → `libs/physics/` and `libs/simulation/`** — these include force headers and core headers now in new locations. Update. Depends on steps 3–7.

9. **Move `adapters/` → `libs/persistence/`** — includes `ymir/adapters/json/...` → `ymir/persistence/json/...`. Update. Depends on steps 3–8.

10. **Move `applications/CMakeLists.txt` → `apps/server/CMakeLists.txt`**. Create `apps/fast-time/CMakeLists.txt` placeholder. Depends on nothing.

11. **Create `services/` placeholder** — add `.gitkeep`. No dependencies.

12. **Write six `libs/<context>/CMakeLists.txt` files** — one per target with correct source lists and link dependencies per ADR-003. Depends on steps 1–9.

13. **Rewrite root `CMakeLists.txt`** — replace old `add_subdirectory` calls with new ones. Depends on step 12.

14. **Rewrite `tests/CMakeLists.txt`** — five new test executables; move test files to mirrored subdirectories; update all `#include` paths in test files. Depends on steps 1–12.

15. **Verify build and tests** — run grep checks, build, ctest. Fix any missed includes. Depends on all previous steps.

16. **Create `docs/architecture/`** — copy and adapt content from `.prds/modules/*.md` files. No code dependencies; can run in parallel with steps 1–15 if desired.

17. **Update `Doxyfile`** — point `INPUT` to `libs/`. Depends on step 1.

18. **Rewrite `AGENTS.md`** — update all path references; link to `docs/architecture/`. Depends on step 16.

19. **Rewrite `README.md`** — update structure diagram, build instructions, links. Depends on step 16.

20. **Delete legacy directories** — `core/`, `naval/`, `adapters/`, `applications/`, `include/`, `.docs/`. Depends on step 15.

21. **Update `.gitignore`** — ensure `build/` and `docs/generated/` are excluded. No dependencies.

22. **Final verification** — confirm no legacy folders remain; run full build and ctest one final time. Depends on all steps.

### Technical Dependencies

- SUNDIALS, nlohmann_json, and Catch2 must remain available — these are external dependencies unchanged by this reorganization
- CI must pass after the reorganization commit before merging

---

## Technical Considerations

### Key Decisions

**New include namespaces (ADR-002)**
- Decision: include paths change to `ymir/physics/`, `ymir/world/`, etc.
- Rationale: folder name, CMake target name, and include prefix are all identical — one mental model
- Trade-off: ~50 include directives must be updated atomically

**One target per bounded context (ADR-003)**
- Decision: six CMake STATIC library targets instead of two
- Rationale: linker enforces module boundaries; test executables link only what they test
- Trade-off: more `CMakeLists.txt` files to maintain; dependency graph must be kept acyclic

### Known Risks

| Risk | Likelihood | Mitigation |
|------|-----------|-----------|
| Stale `#include` missed during move | Medium | `grep -r "ymir/core\|ymir/naval\|ymir/adapters"` after move; zero hits required |
| Cross-header includes not updated | Medium | Headers that include other Ymir headers must be inspected manually; build error reveals any miss |
| Circular CMake dependency | Low | Dependency graph is acyclic by design; CMake errors on cycles |
| CI path references to old folders | Low | Inspect `.github/workflows/ci.yml` before committing |

---

## Architecture Decision Records

- [ADR-001: Atomic Split Reorganization](adrs/adr-001.md) — Single commit moves all files to final bounded context locations
- [ADR-002: New Include Namespaces Match Bounded Context Directories](adrs/adr-002.md) — Include prefixes change to `ymir/physics/`, `ymir/world/`, etc. to match directory and target names
- [ADR-003: One CMake Static Library Target Per Bounded Context](adrs/adr-003.md) — Six targets (`ymir_common`, `ymir_physics`, `ymir_simulation`, `ymir_world`, `ymir_vessel`, `ymir_persistence`) enforce module boundaries at link time
