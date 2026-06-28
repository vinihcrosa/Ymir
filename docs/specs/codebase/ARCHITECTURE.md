# Ymir — Arquitetura Atual

## Modelo de Bounded Contexts

O projeto usa 6 bounded contexts com dependências enforçadas pelo CMake (link time):

```
ymir_common  ←────────────────────────────────────────────────────
    ↑                                                              │
ymir_physics  ←────────────────────────────────                   │
    ↑               ↑                    ↑                        │
ymir_simulation  ymir_world         ymir_vessel                   │
    ↑               ↑                    ↑                        │
ymir_persistence ←─────────────────────────────────────────────────
```

## Contextos e Responsabilidades

| Contexto | Target CMake | Tipo | Responsabilidade |
|----------|-------------|------|-----------------|
| `common` | `ymir_common` | INTERFACE | Math types, Vector6, Matrix6x6, constantes |
| `physics` | `ymir_physics` | STATIC | RigidBody6DOF, 9 módulos de força, integrador ODE |
| `simulation` | `ymir_simulation` | STATIC | Orquestração, NavalDomain, CouplingForceModel |
| `world` | `ymir_world` | STATIC | Wave engine, Environment, CouplingRegistry |
| `vessel` | `ymir_vessel` | STATIC | Thrusters, rudders, controllers, DynamicVessel |
| `persistence` | `ymir_persistence` | STATIC | JSON scenario reader, BodyDefinition |

## Módulos de Força (ymir_physics)

9 módulos independentes, todos implementam `ForceModel`:
- `ThrustForces`, `RudderForces`, `WindForces`, `CurrentForces`
- `WaveForces` (JONSWAP), `DampingForces`, `InertialForces`
- `RestoringForces`, `SquatForces`, `TugForces`

## Integrador ODE

- Implementação atual: **CVODE BDF** (SUNDIALS) — stiff solver
- Interface abstrata: `IIntegrator` em `CvodeIntegrator.h`
- Target pós-migração: **Dormand-Prince RK45** — mesma interface, zero dependências externas

## Regras Arquiteturais (AGENTS.md)

1. **Nenhum contexto viola o grafo de dependência** — enforçado no link time
2. **Physics tem zero conhecimento de I/O** — sem filesystem, sem network
3. **C API boundary é a única superfície pública** — sem STL/exceções cruzando DLL
4. **Zero heap allocation no integration loop** — buffers pré-alocados
5. **Sem raw owning pointers** — smart pointers ou stack allocation

## Apps (stubs)

- `apps/server/` — target CMake INTERFACE, sem implementação (Phase 3 original)
- `apps/fast-time/` — target CMake INTERFACE, sem implementação

## C API Planejada

`include/` está vazio — C API surface prevista mas não implementada. Será o ponto de entrada dos Embind para WASM.

## Fluxo de Dados (runtime)

```
JSON Scenario
    │
    ▼
ScenarioReader (persistence)
    │
    ▼
NavalDomain (simulation)
    │ monta
    ▼
[DynamicVessel, ...] + NavalEnvironment (world)
    │
    ▼
NavalSimulation.step()
    │ chama
    ▼
RigidBody6DOF.integrate() → Integrador ODE
    │ acumula
    ▼
ForceModels[0..n].compute() → Vector6 forces
    │
    ▼
BodyState (posição, velocidade, aceleração)
```
