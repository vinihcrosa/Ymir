# Bounded Contexts

Each bounded context is a CMake static library target. The linker enforces that no target links to a context it should not depend on (ADR-003). Include prefix, directory name, and CMake target name are all identical by design (ADR-002).

---

## common

**CMake target:** `ymir_common`  
**Include prefix:** `ymir/common/`  
**Directory:** `libs/common/`  
**Dependencies:** none

Shared math utilities, physical constants, and primitive types used across all other contexts. Has no dependency on any other Ymir library.

Responsibilities:
- Vector and matrix math (`LinearAlgebra`, `AngleUtils`, `Interpolation`)
- Physical constants (`PhysicalConstants`)
- Shared type aliases used project-wide (`Types`)

---

## physics

**CMake target:** `ymir_physics`  
**Include prefix:** `ymir/physics/`  
**Directory:** `libs/physics/`  
**Dependencies:** `ymir_common`, `SUNDIALS::cvode`

Rigid-body dynamics: state integration, force accumulation, and all force-module implementations. This is the computational core of the engine.

Responsibilities:
- Rigid-body state representation (`BodyState`, `AbstractBody`, `RigidBody6DOF`)
- Force module interface (`ForceModel`) and all force implementations (inertial, restoring, damping, squat, current, wind, thrust, rudder, tug, wave)
- CVODE integrator wrapper (`CvodeIntegrator`) — adaptive BDF solver for stiff systems
- Naval force model composition (`NavalForceModel`)

Force modules receive `const BodyState&` and return `Forces` — they are pure functions of state with no side effects.

---

## simulation

**CMake target:** `ymir_simulation`  
**Include prefix:** `ymir/simulation/`  
**Directory:** `libs/simulation/`  
**Dependencies:** `ymir_physics`, `ymir_common`

Tick orchestration and the event pub/sub system. Controls simulation lifecycle and ensures every module executes in deterministic order each step.

Responsibilities:
- `Simulation` — start, stop, pause, resume; controls real-time vs. fast-time loop
- Deterministic 12-step tick sequence (see [data-flow.md](data-flow.md))
- `NavalSimulation` — naval-specific orchestration wiring all force modules
- Event system — pub/sub for internal module-to-module and external client communication
- Isolation guarantee — each `Simulation + World` pair is fully independent; no shared mutable state

---

## world

**CMake target:** `ymir_world`  
**Include prefix:** `ymir/world/`  
**Directory:** `libs/world/`  
**Dependencies:** `ymir_physics`, `ymir_common`

State repository for all entities and environment. No physics computed here — only storage and querying.

Responsibilities:
- `World` — single source of truth for entity positions, orientations, and velocities; answers spatial queries
- `Environment` — dynamic ambient conditions: wind speed/direction, current field, tidal level, sea state, visibility
- Wave engine — spectral wave generation (JONSWAP, Pierson-Moskowitz, Regular) with directional spreading (Mitsuyasu cos-2S); per-component force calculation via WAMIT tables
- `Terrain` — bathymetry, coastline, piers, and static obstacles; accepts runtime mutations (dredging, tide changes)

All modules read and write via `World`, never directly between each other.

---

## vessel

**CMake target:** `ymir_vessel`  
**Include prefix:** `ymir/vessel/`  
**Directory:** `libs/vessel/`  
**Dependencies:** `ymir_physics`, `ymir_common`

Vessel configuration and mutable actuator/control state. No physics computed here.

Responsibilities:
- `VesselConfig` — permanent vessel characteristics: dimensions, mass and inertia tensors, added-mass matrix (`invTotalMass` pre-computed in constructor), hydrostatic stiffness matrix, WAMIT RAO tables, propulsor and rudder configurations
- `NavalContext` — mutable actuator state: current/demanded RPM and azimuth angles, rudder angles, tug connections
- Control modes: `drift`, `simulation`, `maneuver` (LOS + PID waypoints), `berthManeuver` (FSM with tugs)
- Derived kinematics computed each tick: SOG, COG, drift angle, speed-to-water

---

## persistence

**CMake target:** `ymir_persistence`  
**Include prefix:** `ymir/persistence/`  
**Directory:** `libs/persistence/`  
**Dependencies:** `ymir_physics`, `ymir_world`, `ymir_vessel`, `nlohmann_json::nlohmann_json`

I/O adapters that translate between external formats and domain objects. No physics or world logic lives here.

Responsibilities:
- JSON scenario reader (`ScenarioReader`) — deserializes scenario files into `BodyDefinition` and `Scenario` structs
- Scenario and entity definitions (`BodyDefinition`, `Scenario`)
- Future: SQLite persistence for simulation history, step records, and event logs
