# Ymir

General-purpose physics engine. Designed to simulate the dynamics of physical bodies under arbitrary force systems — extensible to any domain. Coupled with any viewer or simulator via a clean C API.

**Current scope: naval domain.** First implementation covers 6-DOF hydrodynamics for ships and floating platforms (current, wind, waves, thrusters, rudders, tugs). The architecture is domain-agnostic by design.

## What it does (naval module)

- Simulates vessel motion under current, wind, and waves
- Models propulsion (thrusters, rudders), mooring forces, and tug interactions
- Exposes a C API (`DLL`/`.so`/`.dylib`) for integration with Unity, C#, Python, or any host
- Runs in fast-time (pure simulation) or real-time (frame-driven by external host)

## Requirements

| Concern | Decision |
|---------|----------|
| Language | C++17 |
| Mesh library | CGAL |
| ODE solver | SUNDIALS CVODE |
| Build system | CMake ≥ 3.16 |
| Tests | Catch2 |
| Docs | Doxygen |
| Targets | Linux, macOS, Windows — x86_64 & ARM64 |

## Architecture

Ports and Adapters. Physics core is pure library — no I/O, no rendering, no file dependencies. Adapters handle JSON input, CSV output, and the public C API.

```
ymir/
├── core/           # Pure physics — forces, integrator, vessel state
├── adapters/       # JSON I/O, CSV writer, C API (DLL boundary)
├── applications/   # Executables: fasttime, realtime
├── tests/          # Catch2 unit + integration tests
├── docs/           # Architecture docs, design decisions
└── include/        # Public C headers (DLL interface)
```

> Folder structure is provisional — see `docs/` as the project evolves.

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Cross-compilation targets and packaging scripts will be documented once the build system is established.

## Design principles

- **No global state** in the physics core
- **No runtime allocations** inside the integration loop
- **Pure C interface** at the DLL boundary — no STL, no exceptions crossing the ABI
- Every public function documented with Doxygen
- Every module covered by unit tests before integration
