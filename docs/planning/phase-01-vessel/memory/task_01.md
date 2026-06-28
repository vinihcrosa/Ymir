# Task Memory: task_01.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Refactor ThrustForces, RudderForces para estado de atuador externo via ThrusterCommand/RudderCommand. Renomear commandRPM → initialRPM. Status: COMPLETE.

## Important Decisions

- `ThrusterCommand.currentPitch` inicializado no construtor com `cfg.thrusters[i].pitchRatio` — assim computeNaval usa cmd.currentPitch sem quebrar cálculo de Kt quando setActuatorState não é chamado.
- `ThrusterCommand.currentAzimuth_deg` também inicializado no construtor de `cfg.thrusters[i].azimuth_deg` — comportamento de warm-start correto.
- `RudderForces::RudderConfig` teve `angle_deg` e `rateLimit` removidos (dead fields após refactor). Não havia callers desses campos além do proprio RudderForces.
- Removed `const double dt = state.dt()` de RudderForces::computeNaval (unused), e marcado `state` como `/*state*/` (unused param).
- Não tocou `VesselConfig::ThrusterConfig` — essa struct não tinha `commandRPM`. O `commandRPM` existia apenas em `ThrustForces::ThrusterConfig` (struct aninhada).

## Learnings

- `ThrustForces::ThrusterConfig` é diferente de `VesselConfig::ThrusterConfig` — são structs separadas. O rename foi só no `ThrustForces::ThrusterConfig`.
- Testes antigos usavam `commandRPM = 120.0` com o filtro sendo no-op (currentRPM_ = commandRPM no construtor). A nova API equivale: `initialRPM = 120.0` → `commands_[0].currentRPM = 120.0` no construtor.
- `RudderForces` antiga usava `angle_deg` em graus e convertia internamente. Nova API: `currentAngle_rad` em radianos — testes migrados com `20.0 * M_PI / 180.0`.

## Files / Surfaces

- `libs/physics/include/ymir/physics/forces/ThrustForces.h` — modificado
- `libs/physics/src/forces/ThrustForces.cpp` — modificado
- `libs/physics/include/ymir/physics/forces/RudderForces.h` — modificado
- `libs/physics/src/forces/RudderForces.cpp` — modificado
- `tests/physics/forces/TestThrustForces.cpp` — reescrito
- `tests/physics/forces/TestRudderForces.cpp` — reescrito
- `VesselConfig.h` — não modificado (sem commandRPM nessa struct)

## Errors / Corrections

- Nenhum erro de build ou teste. 79/79 passed após todas as modificações.

## Ready for Next Run

- Task_02 (Thruster entity): usar `ThrustForces::ThrusterCommand` importado de `libs/physics/`. Construtor inicializa `state_.currentRPM = cfg.initialRPM`.
- Task_03 (Rudder entity): usar `RudderForces::RudderCommand`. `currentAngle_rad` em radianos.
- `getThrust(id)` ainda funcional (lê `lastThrust_[id]`, atualizado em computeNaval).
