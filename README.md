# Ymir

6-DOF naval physics simulator — rigid-body hydrodynamics for ships and floating platforms under current, wind, waves, thrusters, rudders, and tug interactions.

The engine is domain-agnostic by design. The naval module is the first application.

## Repository layout

```
ymir/
├── apps/
│   ├── server/        real-time server (WebSocket, Protobuf, API handlers)
│   └── fast-time/     batch accelerated simulation (no live clients)
├── libs/
│   ├── common/        math utilities, physical constants, shared types
│   ├── physics/       bodies, force modules, CVODE integrator
│   ├── simulation/    tick orchestration, event system
│   ├── world/         wave engine, environment, terrain state
│   ├── vessel/        vessel config, control modes, actuator state
│   └── persistence/   JSON scenario reader (SQLite planned)
├── tests/             mirrors libs/ structure
└── docs/
    ├── architecture/  architecture wiki and ADR index
    ├── guides/        building.md, contributing.md
    └── adr/           Architecture Decision Records
```

## Bounded contexts

Each context is a CMake static library. The linker enforces dependency rules — no target may link to a context it does not depend on.

| Context | CMake target | Include prefix | Directory |
|---------|-------------|----------------|-----------|
| Common | `ymir_common` | `ymir/common/` | `libs/common/` |
| Physics | `ymir_physics` | `ymir/physics/` | `libs/physics/` |
| Simulation | `ymir_simulation` | `ymir/simulation/` | `libs/simulation/` |
| World | `ymir_world` | `ymir/world/` | `libs/world/` |
| Vessel | `ymir_vessel` | `ymir/vessel/` | `libs/vessel/` |
| Persistence | `ymir_persistence` | `ymir/persistence/` | `libs/persistence/` |

**Common** — shared math, physical constants, and primitive types; no dependencies on other Ymir libraries.  
**Physics** — rigid-body dynamics: state integration, force accumulation, and all force-module implementations.  
**Simulation** — tick orchestration, real-time vs. fast-time loop, deterministic step sequence, pub/sub event system.  
**World** — state repository for all entities and environment; wave engine, terrain, ambient conditions.  
**Vessel** — vessel configuration, mutable actuator/control state, control modes (drift, LOS, berth maneuver).  
**Persistence** — I/O adapters translating between external formats (JSON) and domain objects.

See [docs/architecture/bounded-contexts.md](docs/architecture/bounded-contexts.md) for the full dependency table and responsibility breakdown.

## Building

### Prerequisites

| Requirement | Version |
|-------------|---------|
| C++ compiler | GCC 9+ or Clang 10+ (C++17) |
| CMake | 3.20+ |
| SUNDIALS | 6.x (`SUNDIALS::cvode`) |
| Catch2 | 3.x (tests) |
| nlohmann/json | any recent release (persistence) |

### Commands

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

## Further reading

- [docs/architecture/README.md](docs/architecture/README.md) — architecture wiki: bounded-context map, data-flow diagrams, ADR index
- [AGENTS.md](AGENTS.md) — contributor rules, coding standards, include conventions, commit discipline
