# Workflow Memory

Keep only durable, cross-task context here. Do not duplicate facts that are obvious from the repository, PRD documents, or git history.

## Current State

Tasks 01–09 ALL COMPLETE. Phase 1 done. ThrustForces/RudderForces refactored (task_01). Thruster entity (task_02). Rudder entity (task_03). `libs/vessel` is STATIC. VesselState header-only (task_04). PrescribedController (task_05). ManeuverController (task_06). BerthManeuverSystem + TugParametricForces (task_07). DynamicVessel aggregate root (task_08). NavalSimulation refactored to N-body (task_09).

## Shared Decisions

- `ThrustForces::ThrusterCommand` e `RudderForces::RudderCommand` definidos DENTRO das próprias classes em `libs/physics/` — evita dep circular (libs/vessel/ pode importar esses tipos de libs/physics/).
- `ThrustForces::ThrusterCommand` tem 3 campos: `currentRPM`, `currentAzimuth_deg`, `currentPitch` (todos com default 0.0).
- `RudderForces::RudderCommand` tem 1 campo: `currentAngle_rad` (default 0.0, em RADIANOS).
- `ThrustForces::ThrusterConfig` (struct aninhada) é diferente de `VesselConfig::ThrusterConfig` — structs separadas com finalidades diferentes.
- `Thruster::ActuatorState` armazena azimuth em GRAUS (`_deg`). `azimuthSpeed` e `pitchRate` em VesselConfig são em **deg/s** (não rad/s). Default `azimuthSpeed = 5.0` deg/s.
- `enable_testing()` deve estar ANTES dos `add_subdirectory(libs/...)` no root CMakeLists.txt para que `catch_discover_tests` funcione em libs in-tree (e.g., `libs/vessel/tests/`).
- `VesselConfig::ThrusterConfig` ganhou campos `initialRPM = 0.0` e `pitchRate = 5.0`.

## Shared Learnings

- `VesselConfig::ThrusterConfig` NÃO tinha `commandRPM` — o rename foi só em `ThrustForces::ThrusterConfig`.
- `RudderForces::RudderConfig` não tem mais `angle_deg` nem `rateLimit` (removidos como dead code).
- `getThrust(id)` em ThrustForces ainda funciona: lê `lastThrust_[id]`, populado em `computeNaval()`.
- Testes de `libs/vessel/` ficam em `libs/vessel/tests/` com próprio `CMakeLists.txt` (não em `tests/vessel/`).

## Open Risks

- Nenhum.

## Handoffs

- `Thruster` e `Rudder` são não-copiáveis mas MOVE-CONSTRUCTIBLE. Usar `reserve(n)` + `emplace_back(cfg)` para popular vectors.
- `ManeuverController` pronto. SOG = `sqrt(u^2+v^2)` de body-frame — não usa `speedToWater`. `wrapToPi` via `ymir::math::AngleUtils`. PID sem anti-windup (Phase 1 — manter ki=0 em cenários de teste para evitar windup).
- `PrescribedController`, `ManeuverController`, e `BerthManeuverSystem` satisfazem o conceito implícito `update(t, dt, ctx, thrusters, rudders)` — prontos para `DynamicVessel::std::variant`.
- `VesselConfig::RudderConfig::angleMaximum` é plain `angleMaximum` (sem sufixo `_rad`). `angleSpeed` em rad/s.
- `RudderForces::RudderConfig` (physics) é DIFERENTE de `VesselConfig::RudderConfig` (vessel).
- Integration test com `RudderForces`: setar `thrusterIdx = std::numeric_limits<std::size_t>::max()` para desabilitar slipstream; setar `ctx.speedToWater[0] != 0` para evitar early-return em `computeNaval`.
- `BerthManeuverSystem::tugForces()` retorna `const Vector6&` acumulado no último `update()`. `TugParametricForces` armazena ponteiro não-owner para BSM — BSM deve outlive TugParametricForces. `NavalForceModel` é non-copyable e non-movable — sempre via `std::make_unique`.
- `DynamicVessel` é NON-COPYABLE e NON-MOVABLE — `NavalSimulation` deve armazenar `DynamicVessel*` (raw non-owner pointer), nunca por valor. `VesselState` está em `ymir::vessel` (não `ymir::naval`) — usar qualified name nas headers que ficam em `ymir::naval`.
- `DynamicVessel` construtor: `VesselConfig&` + `vector<Thruster>` + `vector<Rudder>` por valor (move-into). Configs (`ThrusterConfig`, `RudderConfig`) em `VesselConfig` devem outlive os thrusters/rudders referenciados.
- `NavalSimulation` agora N-body: `addBody(id, body)` → `addNavalForceModel(int bodyId, ...)` → `registerVessel(int bodyId, DynamicVessel&, ThrustForces*, RudderForces*)` → `initialize()`. `state(int bodyId)` throws `std::out_of_range`. `libs/simulation` depende de `ymir_vessel` (link explícito em CMakeLists).
- BSM dentro de `std::variant` não expõe pointer externo — para usar `TugParametricForces` em testes, manter BSM standalone fora do vessel e passá-lo tanto para `setController(copy)` quanto para `TugParametricForces(ptr)`.
