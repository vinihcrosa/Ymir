# Ymir Architecture

Ymir is a general-purpose 6-DOF physics engine targeting the naval domain. It runs multiple isolated simulations in parallel and exposes a WebSocket + Protobuf API to external clients.

## Design Principles

- **Domain-agnostic core** — the naval module is one application of the engine, not the engine itself.
- **Bounded contexts enforced at link time** — one CMake static library target per context; linker rejects illegal cross-context dependencies.
- **No I/O in physics** — all external dependencies (JSON, SQLite, networking) live in adapter libraries.
- **Deterministic tick order** — every module executes in a fixed sequence each simulation step.

## Repository Structure

```
ymir/
├── apps/
│   ├── server/        real-time server: WebSocket, Protobuf, API handlers
│   └── fast-time/     batch accelerated: no live clients
├── libs/
│   ├── common/        shared math, types, physical constants
│   ├── physics/       rigid-body dynamics, force modules, CVODE integrator
│   ├── simulation/    tick orchestration, event system
│   ├── world/         wave engine, environment, terrain state
│   ├── vessel/        vessel configuration, control modes, actuator state
│   └── persistence/   JSON scenario reader (SQLite planned)
├── tests/             mirrors libs/ structure
└── docs/
    ├── architecture/  this wiki
    ├── guides/        building.md, contributing.md
    └── adr/           Architecture Decision Records
```

## Architecture Pages

| Page | Contents |
|------|----------|
| [bounded-contexts.md](bounded-contexts.md) | Responsibilities, CMake target, include prefix, and dependencies for each bounded context |
| [data-flow.md](data-flow.md) | Simulation tick sequence (12 steps), force execution order, CVODE integration loop |

## Bounded-Context Map

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

## Two Operation Modes

| App | Mode | Clients | Isolation |
|-----|------|---------|-----------|
| `apps/server` | Real-time 1× | Manager, Viewer (live events) | Thread per simulation |
| `apps/fast-time` | Batch accelerated N× | None live | Process per simulation |

## ADRs

- [ADR-001](../adr/) — Atomic split reorganization
- [ADR-002](../adr/) — Include namespaces match bounded-context directories
- [ADR-003](../adr/) — One CMake static library target per bounded context
