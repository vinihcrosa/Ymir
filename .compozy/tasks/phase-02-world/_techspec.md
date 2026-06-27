# TechSpec: World + Multi-Domain Naval Architecture — Phase 2

## Executive Summary

Phase 2 introduces four new components — `Environment`, `IDomain`, `World`, and
`NavalDomain` — and one support component (`CouplingRegistry`) while deprecating
`NavalSimulation` and `NavalEnvironment` in place. The primary architectural trade-off
is Jacobi coupling (one-tick force latency, O(1) per tick) over Gauss-Seidel
(zero latency, O(N iterations) per tick). At naval timescales (dt ≤ 1.0 s), Jacobi
latency is physically negligible; the cost savings are concrete. The second trade-off
is `IDomain` abstraction now (one extra virtual dispatch per domain step) versus
deferring it until a second domain type is needed — justified by the roadmap's explicit
design for future non-naval domains.

All new code targets C++17, resides in the `ymir` namespace, maintains zero per-tick
heap allocation, and follows the existing pattern: config structs are value types,
state is pre-allocated in constructors, non-copyable owning types use `= delete`.

---

## System Architecture

### Component Overview

```
World  (libs/world/)
├── GlobalClock            double time_
├── Environment            mutable shared env (wind, current, tide, depth)
├── DomainRegistry         std::vector<unique_ptr<IDomain>>
└── CouplingRegistry       resolves inter-body force links

IDomain  (libs/world/)     abstract interface
└── NavalDomain  (libs/simulation/)
    ├── Simulation          existing body container (RigidBody6DOF map)
    ├── BodyEntry[]         per-body: NavalContext, force models, DynamicVessel
    ├── const Environment*  injected by World::addDomain()
    └── CouplingForces      pending forces from CouplingRegistry

CouplingRegistry  (libs/simulation/)
└── CouplingLink[]         (producerBodyId, consumerBodyId, Forces, ready flag)

CouplingForceModel  (libs/simulation/)
    reads from CouplingRegistry during force accumulation

BerthManeuverSystem  (libs/vessel/)  — modified
    writes tug forces to CouplingRegistry (if set)
```

**Data flow per `World::step(dt)`:**

```
1. time_ += dt
2. for each domain:
     domain.step(dt)
       ├─ for each body (ascending id):
       │    ctx = buildContext(body, env_)
       │    vessel.updateControl(t, dt, ctx)    ← BSM writes to CouplingRegistry
       │    vessel.updateStates(dt)
       │    vessel.syncToForceModels(thrust, rudder)
       │    ΣF = Σ model.computeNaval(state, ctx)  ← CouplingForceModel reads registry
       │    body.step(dt)                         ← CVODE integrates
       └─ (end body loop)
3. coupling_.resolve()   ← flush producer writes into consumer slots
```

**Key invariants:**
- `Environment` is owned by `World`; `NavalDomain` holds a `const Environment*`.
- `CouplingRegistry` is owned by `World`; `NavalDomain` and `BerthManeuverSystem`
  hold non-owning pointers to it.
- `CouplingForceModel` holds a non-owning pointer to `CouplingRegistry` and the
  consumer body id; it reads the resolved force from the previous tick.
- `World::addDomain()` calls `domain.onAddedToWorld(env_)` and injects the registry.

---

## Implementation Design

### Core Interfaces

#### Environment  (`libs/world/include/ymir/world/Environment.h`)

```cpp
namespace ymir {

class Environment {
public:
    void setWind(double speed_ms, double dir_deg_naut);
    void setCurrent(double speed_ms, double dir_deg_naut);
    void setTide(double level_m);
    void setWaterDepth(double depth_m);   // must be > 0
    void setSeaState(double Hs_m, double Tp_s, double dir_deg_naut);

    double windSpeed()           const noexcept { return wind_.speed; }
    double windDirectionNaut()   const noexcept { return wind_.dir; }
    double currentSpeed()        const noexcept { return current_.speed; }
    double currentDirectionNaut()const noexcept { return current_.dir; }
    double waterDepth()          const noexcept { return waterDepth_; }
    double tide()                const noexcept { return tide_; }

private:
    struct SpeedDir { double speed = 0.0; double dir = 0.0; };
    SpeedDir wind_;
    SpeedDir current_;
    double   waterDepth_ = 100.0;
    double   tide_       = 0.0;
};

// Backward compat — deprecated alias removed in Phase 3
using NavalEnvironment [[deprecated("Use Environment")]] = Environment;

} // namespace ymir
```

Setters validate at system boundary: `speed_ms >= 0`, `depth_m > 0`. Assertions (not
exceptions) — callers are internal simulation code, not user input handlers.

---

#### IDomain  (`libs/world/include/ymir/world/IDomain.h`)

```cpp
namespace ymir {

class Environment;

struct BodyPosition {
    int    id;
    double x, y, z;
};

class IDomain {
public:
    virtual ~IDomain() = default;

    virtual void onAddedToWorld(Environment& env,
                                CouplingRegistry& coupling) = 0;
    virtual void step(double dt) = 0;

    virtual std::vector<BodyPosition> allBodyPositions() const = 0;
    virtual BodyState bodyState(int id) const = 0;
    virtual std::string name() const = 0;

    IDomain(const IDomain&) = delete;
    IDomain& operator=(const IDomain&) = delete;
};

} // namespace ymir
```

---

#### CouplingRegistry  (`libs/simulation/include/ymir/simulation/CouplingRegistry.h`)

```cpp
namespace ymir {

class CouplingRegistry {
public:
    // Setup — call before first step
    void addLink(int producerBodyId, int consumerBodyId);

    // Called by BSM/producers during domain.step()
    void writeForce(int producerBodyId, const Forces& f);

    // Called by World::step() after all domain steps
    void resolve();

    // Called by CouplingForceModel during force accumulation
    Forces consumedForce(int consumerBodyId) const;

    // Called by World::step() at the start of each tick
    void reset();

private:
    struct Link {
        int    producerBodyId;
        int    consumerBodyId;
        Forces force  = Forces::zero();
        bool   ready  = false;
    };
    std::vector<Link> links_;  // small N; linear scan is fine
};

} // namespace ymir
```

`consumedForce()` returns `Forces::zero()` if no link exists for `consumerBodyId` or
if the link is not ready (producer did not write this tick).

---

#### NavalDomain  (`libs/simulation/include/ymir/simulation/NavalDomain.h`)

```cpp
namespace ymir {

class NavalDomain final : public IDomain {
public:
    explicit NavalDomain(std::string name = "naval");

    // Body setup — mirrors NavalSimulation API
    void addBody(int id, std::unique_ptr<RigidBody6DOF> body);
    void addNavalForceModel(int bodyId, std::unique_ptr<NavalForceModel> model);
    void registerVessel(int bodyId, DynamicVessel& vessel,
                        ThrustForces* tf = nullptr,
                        RudderForces* rf = nullptr);
    void initialize();

    // IDomain
    void onAddedToWorld(Environment& env, CouplingRegistry& coupling) override;
    void step(double dt) override;
    std::vector<BodyPosition> allBodyPositions() const override;
    BodyState bodyState(int id) const override;
    std::string name() const override;

    // Convenience queries
    double distanceBetween(int idA, int idB) const;
    BodyState state(int id) const;  // alias for bodyState (API parity with NavalSim)
    double time() const;

    void reset();

    NavalDomain(const NavalDomain&) = delete;
    NavalDomain& operator=(const NavalDomain&) = delete;
    NavalDomain(NavalDomain&&) = delete;
    NavalDomain& operator=(NavalDomain&&) = delete;

private:
    struct BodyEntry {
        RigidBody6DOF*                body    = nullptr;
        std::vector<NavalForceModel*> models;
        NavalContext                  ctx;
        Vector6                       q_avg;   // EMA τ=16.5s
        DynamicVessel*                vessel  = nullptr;
        ThrustForces*                 thrust  = nullptr;
        RudderForces*                 rudder  = nullptr;
    };

    NavalContext buildContext(int id, const BodyEntry&, double dt) const;

    std::string                     name_;
    Simulation                      sim_;
    const Environment*              env_      = nullptr;  // injected
    CouplingRegistry*               coupling_ = nullptr;  // injected
    std::map<int, BodyEntry>        entries_;
};

} // namespace ymir
```

`NavalDomain` is structurally identical to `NavalSimulation` with:
1. `NavalEnvironment env_` replaced by `const Environment* env_`
2. `CouplingRegistry* coupling_` added
3. `IDomain` interface implemented
4. Spatial query methods added

The implementation of `step(dt)` is a copy-refactor of `NavalSimulation::step(dt)`;
`buildContext()` reads from `env_` instead of the local struct.

---

#### World  (`libs/world/include/ymir/world/World.h`)

```cpp
namespace ymir {

class World {
public:
    World() = default;

    // Domain management
    void addDomain(std::unique_ptr<IDomain> domain);
    IDomain&       domain(const std::string& name);
    const IDomain& domain(const std::string& name) const;

    // Tick
    void step(double dt);

    // Observation
    double time() const noexcept { return time_; }
    WorldSnapshot snapshot() const;

    // Environment access
    Environment&       environment()       noexcept { return env_; }
    const Environment& environment() const noexcept { return env_; }

    World(const World&) = delete;
    World& operator=(const World&) = delete;

private:
    double                               time_     = 0.0;
    Environment                          env_;
    std::vector<std::unique_ptr<IDomain>>domains_;
    CouplingRegistry                     coupling_;
};

} // namespace ymir
```

`World::step(dt)`:
```cpp
void World::step(double dt) {
    time_ += dt;
    coupling_.reset();
    for (auto& d : domains_)
        d->step(dt);
    coupling_.resolve();
}
```

---

#### CouplingForceModel  (`libs/simulation/include/ymir/simulation/CouplingForceModel.h`)

```cpp
namespace ymir {

class CouplingForceModel final : public NavalForceModel {
public:
    CouplingForceModel(int consumerBodyId, const CouplingRegistry& registry)
        : consumerBodyId_(consumerBodyId), registry_(registry) {}

    std::string name() override { return "CouplingForceModel"; }

protected:
    Forces computeNaval(const BodyState&, const NavalContext&) override {
        return registry_.consumedForce(consumerBodyId_);
    }

private:
    int                    consumerBodyId_;
    const CouplingRegistry& registry_;
};

} // namespace ymir
```

This model is added to any body that receives coupling forces. It reads the force
resolved by `CouplingRegistry::resolve()` from the previous tick (Jacobi).

---

#### BerthManeuverSystem — updated signature (`libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h`)

Add optional registry pointer; existing constructor is unchanged:

```cpp
// New method — call after construction if coupling is active:
void setCouplingRegistry(CouplingRegistry* registry,
                         int shipBodyId,
                         const std::vector<int>& tugBodyIds);
```

In `update()`: if `registry_` is set, write `tugForces_` to
`registry_->writeForce(tugId, tugForce)` for each tug instead of accumulating
in `tugForces_`. If `registry_` is null, behavior is unchanged (backward compat).

`TugParametricForces` is marked `[[deprecated("Use CouplingRegistry + CouplingForceModel")]]`.

---

### Data Models

#### WorldSnapshot

```cpp
struct EnvironmentSnapshot {
    double windSpeed_ms;
    double windDir_deg;
    double currentSpeed_ms;
    double currentDir_deg;
    double waterDepth_m;
    double tide_m;
};

struct BodySnapshot {
    int       id;
    BodyState state;
};

struct DomainSnapshot {
    std::string              name;
    std::vector<BodySnapshot>bodies;
};

struct WorldSnapshot {
    double                    simTime;
    EnvironmentSnapshot       environment;
    std::vector<DomainSnapshot>domains;
};
```

Nested by domain — aligns with Phase 3 server serialization requirements.

---

### Deprecation Contracts

| Symbol | Deprecated In | Removed In | Replacement |
|--------|--------------|-----------|-------------|
| `NavalEnvironment` | Phase 2 | Phase 3 | `Environment` |
| `NavalSimulation` | Phase 2 | Phase 3 | `NavalDomain` + `World` |
| `TugParametricForces` | Phase 2 | Phase 3 | `CouplingRegistry` + `CouplingForceModel` |

Each deprecated symbol uses `[[deprecated("message")]]` on the class/type-alias
declaration. No functional change; all compile without warning if caller does not
use the deprecated path.

---

## Impact Analysis

| Component | Impact Type | Description and Risk | Required Action |
|-----------|-------------|---------------------|-----------------|
| `NavalSimulation` | deprecated | Marked `[[deprecated]]`; no functional change. Risk: existing tests trigger warnings. | Add `-Wno-deprecated-declarations` to test targets or migrate. |
| `NavalEnvironment` | deprecated | Type alias to `Environment`. Risk: code comparing `NavalEnvironment` by type may break. | Find usages; most are field-access only — no change needed. |
| `TugParametricForces` | deprecated | Class still compiles; BSM no longer populates its data source by default when registry is set. Risk: BSM-only tests pass; full scenario tests may diverge. | Keep BSM in non-registry mode in unit tests; add registry mode integration tests. |
| `BerthManeuverSystem` | modified | New `setCouplingRegistry()` method; existing `update()` has conditional branch. Risk: off-by-one if registry is set but not resolved. | Tests: run BSM with and without registry; compare tugForces output. |
| `NavalSimulation::step()` | unchanged | No functional change; internal impl may be refactored to share code with NavalDomain. Risk: code duplication if not refactored. | Optional: extract `buildContext()` to shared util; not required for Phase 2. |
| `libs/simulation/CMakeLists.txt` | modified | Adds `ymir_world` to target_link_libraries. Risk: transitive dep changes for downstream. | Rebuild all; no API change. |
| `libs/world/CMakeLists.txt` | modified | Adds `World.cpp`, `Environment.cpp` sources. Low risk. | Rebuild. |
| Tests: `TestNavalIntegration` | modified | Re-tested against NavalDomain; NavalSimulation tests kept as regression. | Add `NavalDomain` versions of existing scenarios. |

---

## Testing Approach

### Unit Tests

**`tests/simulation/TestCouplingRegistry.cpp`**
- `addLink + writeForce + resolve + consumedForce` round trip
- `consumedForce` returns zero before first resolve
- `consumedForce` returns zero if producer did not write this tick (reset clears ready)
- Multiple links (two tugs → one ship): only correct consumer gets each force
- `reset()` clears ready flags without clearing force values

**`tests/simulation/TestNavalDomain.cpp`**
- `addBody + addNavalForceModel + registerVessel + initialize + step`: no exception
- `state(id)` returns consistent state after step
- `allBodyPositions()` returns correct (x, y, z) from BodyState.q()
- `distanceBetween()`: known positions → known distance (tolerance 1e-10)
- `onAddedToWorld()` sets env pointer: buildContext reads from injected Environment
- `step()` with injected CouplingForceModel: coupling force appears in body DOF response
- Regression: single-vessel NavalDomain matches NavalSimulation output within 1e-8
  per DOF across 1 000 ticks (same vessel config, same initial state, same dt)

**`tests/world/TestEnvironment.cpp`**
- Setters round-trip via getters
- `setWaterDepth(0.0)` triggers assertion
- `setWind(-1.0, 0.0)` triggers assertion

**`tests/world/TestWorld.cpp`**
- `addDomain` → `domain(name)` retrieves correct instance
- `step(dt)` advances `time()` correctly
- `step(dt)` calls domain step (verify via mock or observable side effect)
- `coupling_.resolve()` called after domain steps (CouplingForceModel observable)
- `snapshot()` returns correct simTime and body states

### Integration Tests

**`tests/simulation/TestNavalDomainCoupling.cpp`**
- Scenario: ship (id=0) + 2 tugs (id=1, id=2) in one NavalDomain
- BSM in ship's DynamicVessel, registry set via `setCouplingRegistry()`
- Run 100 ticks: ship sway DOF deviates measurably from no-tug baseline
- Coupling force received by ship matches BSM output from previous tick (Jacobi)

**`tests/simulation/TestNavalDomainRegression.cpp`**
- Same scenario as `TestNavalIntegration` but using `NavalDomain` + `World`
- Assert per-DOF error < 1e-8 vs NavalSimulation for all 1 000 ticks
- Covers: single vessel, ManeuverController, 3 waypoints

**`tests/simulation/TestWorldMultiDomain.cpp`**
- Two NavalDomains registered in one World (valid but unusual)
- `world.step(dt)` does not mix body states across domains
- `world.snapshot()` has two domain entries

### Mocks and Boundaries

- No mocking of `CVODE` or `NavalForceModel` in unit tests — use `ZeroForceModel` (existing).
- CouplingRegistry tested in isolation with `Forces` directly (no body needed).
- NavalDomain tested with minimal `RigidBody6DOF` (1 DOF is fine for structure tests;
  use full 6-DOF for regression test).

---

## Development Sequencing

### Build Order

1. **`Environment`** (`libs/world/`) — no new deps; replaces `NavalEnvironment` fields;
   add `[[deprecated]] using NavalEnvironment = Environment` immediately.

2. **`IDomain` + `BodyPosition`** (`libs/world/`) — depends on step 1 (`Environment`
   appears in `onAddedToWorld` signature). Forward-declare `CouplingRegistry`.

3. **`CouplingRegistry`** (`libs/simulation/`) — depends on `Forces` from physics (already
   available). No dependency on steps 1 or 2.

4. **`CouplingForceModel`** (`libs/simulation/`) — depends on step 3 (`CouplingRegistry`)
   and existing `NavalForceModel`.

5. **`NavalDomain`** (`libs/simulation/`) — depends on steps 1, 2, 3, 4, and all existing
   `libs/simulation/` code (Simulation, NavalForceModel, RigidBody6DOF, DynamicVessel,
   NavalContext). Implementation is a copy-refactor of `NavalSimulation::step()` with
   `env_->*()` calls replacing `env_.*` field access.

6. **`BerthManeuverSystem` update** (`libs/vessel/`) — depends on step 3
   (`CouplingRegistry`). Add `setCouplingRegistry()` + conditional branch in `update()`.
   Deprecate `TugParametricForces`.

7. **`WorldSnapshot`** (`libs/world/`) — depends on `BodyState` (existing) and `Environment`
   (step 1). Pure data types; no logic.

8. **`World`** (`libs/world/`) — depends on steps 1, 2, 7 (IDomain, Environment,
   WorldSnapshot). `World::step()` is 5 lines. `World::snapshot()` iterates domains.

9. **`NavalSimulation` deprecation** — add `[[deprecated]]` attribute to class declaration.
   No functional change.

10. **Unit tests for each step** — written alongside or immediately after each step.
    Step 5 unit test includes regression against `NavalSimulation`.

11. **Integration test: ship + 2 tugs** — depends on steps 3–8 all complete.

12. **CMakeLists.txt updates** — `libs/simulation/CMakeLists.txt`: add `ymir_world` to
    `target_link_libraries`; add `NavalDomain.cpp`, `CouplingRegistry.cpp`.
    `libs/world/CMakeLists.txt`: add `World.cpp`, `Environment.cpp`. Apply before step 8.

### Technical Dependencies

- CVODE (SUNDIALS) — already linked; no new external dependency.
- C++17 — already the project standard; `[[deprecated]]` and `std::variant` already in use.
- No new third-party libraries.

---

## Monitoring and Observability

Phase 2 is a library (no server process), so "monitoring" means:

**Debug assertions in hot path** (compiled out in Release):
- `NavalDomain::step()`: `assert(env_ != nullptr)` — guard against uninitialized domain.
- `CouplingRegistry::writeForce()`: `assert(producerBodyId in links_)` — guard against
  unknown producer.
- `NavalDomain::buildContext()`: `assert(env_->waterDepth() > 0.0)`.

**Determinism check** (test only): `TestNavalDomainRegression` is the continuous
observability signal — it runs in CI on every commit and fails if NavalDomain diverges
from NavalSimulation or if the multi-body scenario produces NaN/inf.

**Coupling visibility**: `CouplingRegistry::consumedForce(id)` is public and testable;
integration tests log the consumed force per tick to validate coupling delivery.

---

## Technical Considerations

### Key Decisions

**Coupling timing**: `CouplingRegistry::reset()` is called at the START of `World::step()`
(before domain steps), not at the end. This means at the beginning of tick `t`, the
registry holds the forces resolved from tick `t-1`. The consumer reads these during tick
`t`'s force accumulation. The producer writes new forces during tick `t`. `resolve()` at
the end of tick `t` pushes those new forces into consumer slots for tick `t+1`. The `reset()`
call clears the `ready` flags so a producer that fails to write in tick `t` does not
silently replay tick `t-1`'s force — `consumedForce()` returns zero for unwritten ports.

**NavalDomain body stepping order**: Ascending ID order (inherited from `Simulation::step()`).
Ship is always id=0; tugs are id=1, id=2, etc. This means: ship steps first (BSM writes
coupling force), tug bodies step next (their own physics evolve). One-directional coupling
only in Phase 2; no reaction force on tugs.

**`NavalDomain` name parameter**: `name()` returns the string passed to the constructor.
`World::domain("name")` uses this for lookup. Default is "naval". Multiple NavalDomains
in one World require distinct names — enforced by `World::addDomain()` assertion.

**`Environment` thread safety**: Not thread-safe. Callers must not call setters while
`World::step()` is running. Single-threaded simulation is the only supported mode.

**EMA (τ=16.5 s) in NavalDomain**: Copied from NavalSimulation. `q_avg` in BodyEntry
is initialized to the body's initial position during `initialize()`. `reset()` reinitializes
from current state.

### Known Risks

| Risk | Mitigation |
|------|-----------|
| `buildContext()` duplication between NavalSimulation and NavalDomain | Accept for Phase 2; both are correct and regression-tested. Extract to shared util in Phase 3 cleanup. |
| BSM coupling branch introduces divergent behavior when registry is set vs not | Unit test: run BSM both ways; assert `tugForces_` output is identical to `consumedForce()` when one-body scenario has no coupling. |
| `const Environment*` null-deref if domain used outside World | `assert(env_ != nullptr)` in `step()` and `buildContext()`. `NavalDomain::initialize()` does not assert — allow use without World for unit testing. |
| Multiple domains in one World sharing CouplingRegistry: body id collisions | CouplingRegistry uses raw body ids; two domains could have bodies with same ids. Mitigated by: Phase 2 has at most one NavalDomain; CouplingRegistry links are explicit (producerBodyId is unique across all links). Long-term: use (domainName, bodyId) composite key — deferred to Phase 3. |

---

## Architecture Decision Records

- [ADR-001: World como Thin Orchestrator com DomainRegistry](adrs/adr-001.md) — World não executa física; orquestra domínios + resolve coupling; NavalDomain é o primeiro domínio e substitui NavalSimulation.
- [ADR-002: Tugs Promovidos de Forças Paramétricas a Corpos Físicos Independentes com CouplingPort](adrs/adr-002.md) — TugParametricForces deprecated; tugs tornam-se corpos NavalDomain trocando força via CouplingRegistry.
- [ADR-003: IDomain Interface em libs/world para Evitar Dependência Circular](adrs/adr-003.md) — IDomain em libs/world; NavalDomain em libs/simulation; deps formam DAG sem ciclo.
- [ADR-004: Acoplamento Jacobi com CouplingRegistry](adrs/adr-004.md) — Jacobi splitting escolhido sobre Gauss-Seidel; latência de um tick aceitável em dt naval ≤ 1.0 s.
- [ADR-005: Environment como Classe com Setters Tipados; NavalEnvironment Deprecated](adrs/adr-005.md) — Environment em libs/world com API encapsulada; NavalEnvironment = type alias deprecated.
