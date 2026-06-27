---
status: completed
title: Implementar ManeuverController
type: backend
complexity: medium
dependencies:
  - task_02
  - task_03
---

# Task 6: Implementar ManeuverController

## Overview

Cria `ManeuverController` que navega uma embarcação por uma lista de waypoints usando
LOS (Line-of-Sight) para calcular o bearing alvo e dois PIDs independentes para gerar
demandas de heading (→ leme) e de velocidade (→ RPM dos propulsores). Quando a lista
de waypoints é esgotada, as demandas voltam a zero (drift). Esta task é paralela à
task_05 (PrescribedController).

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `ManeuverController::Waypoint` DEVE conter `x_m`, `y_m`, e `demandedSpeed_mps`
- `ManeuverController::Config` DEVE conter `waypoints`, `captureRadius_m`, e seis ganhos PID (`headingKp/Ki/Kd`, `speedKp/Ki/Kd`)
- `update()` DEVE calcular bearing LOS via `atan2(dy, dx)` em referencial inercial e converter para erro de heading via `wrapToPi(bearing - yaw)`
- `update()` DEVE chamar PID de heading com o erro calculado e aplicar a saída como demanda em todos os lemes
- `update()` DEVE calcular erro de velocidade como `demandedSpeed - SOG` e aplicar saída do PID de velocidade como demanda de RPM em todos os propulsores
- Um waypoint é capturado quando distância até ele < `captureRadius_m`; controller DEVE avançar para o próximo waypoint
- Com lista esgotada, `update()` DEVE setar demanda zero em todos os thrusters e rudders
- `waypointsExhausted()` DEVE retornar `true` após o último waypoint ser capturado
- PID DEVE ser struct interna ao `.cpp` com método `update(error, dt)` e `reset()`; sem anti-windup na Fase 1
- `update()` DEVE ter assinatura compatível com o conceito `VesselController` — ver TechSpec ADR-002
</requirements>

## Subtasks

- [x] 6.1 Criar `libs/vessel/include/ymir/vessel/controllers/ManeuverController.h` com `Waypoint`, `Config`, e declarações de método
- [x] 6.2 Criar `libs/vessel/src/controllers/ManeuverController.cpp` com PID interno, `wrapToPi()`, LOS, e lógica de captura de waypoint
- [x] 6.3 Adicionar `ManeuverController.cpp` ao `libs/vessel/CMakeLists.txt`
- [x] 6.4 Criar `libs/vessel/tests/TestManeuverController.cpp` com os casos de teste especificados

## Implementation Details

Ver TechSpec seção "ManeuverController" para assinatura completa.

`wrapToPi(angle)`: `angle - 2π * round(angle / 2π)` ou equivalente. Necessário para
evitar que o PID compute erro de heading incorreto quando bearing e yaw estão em
quadrantes opostos (e.g., erro real = 5° mas diferença raw = 355°).

PID struct (internal to `.cpp`):
```cpp
struct PID {
    double kp, ki, kd;
    double integral_  = 0.0;
    double prevError_ = 0.0;
    double update(double error, double dt) noexcept;
    void   reset() noexcept;
};
```
`update` retorna `kp*error + ki*integral_ + kd*(error - prevError_)/dt`.

SOG é derivado de `ctx.speedToWater` ou de `ctx.state.qdot()` — verificar qual campo
de NavalContext contém a velocidade sobre o fundo (SOG).

### Relevant Files

- `libs/vessel/include/ymir/vessel/Thruster.h` (task_02) — recebe demanda de RPM
- `libs/vessel/include/ymir/vessel/Rudder.h` (task_03) — recebe demanda de ângulo
- `libs/vessel/include/ymir/vessel/NavalContext.h` — fonte de posição, yaw, velocidade
- `libs/physics/include/ymir/physics/BodyState.h` — `x()`, `y()`, `yaw()`, `u()`, `v()`

### Dependent Files

- `libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h` (task_07) — usa ManeuverController internamente para fase Navigating
- `libs/vessel/include/ymir/vessel/DynamicVessel.h` (task_08) — usa ManeuverController no variant

### Related ADRs

- [ADR-002: std::variant para Despacho de Controlador](adrs/adr-002.md) — ManeuverController satisfaz o conceito implícito

## Deliverables

- `libs/vessel/include/ymir/vessel/controllers/ManeuverController.h` (novo)
- `libs/vessel/src/controllers/ManeuverController.cpp` (novo)
- `libs/vessel/tests/TestManeuverController.cpp` (novo) **(REQUIRED)**
- `libs/vessel/CMakeLists.txt` atualizado

## Tests

- Unit tests:
  - [x] LOS bearing: navio em `(0,0)` heading `0`, waypoint em `(100,0)` → erro de heading = `0` → demanda de leme = `0`
  - [x] LOS bearing: navio em `(0,0)` heading `0`, waypoint em `(0,100)` → bearing = `π/2` → erro = `π/2` → demanda de leme proporcional a `kp * π/2`
  - [x] `wrapToPi`: bearing `355°`, yaw `0°` → erro = `-5°` (não `355°`)
  - [x] Captura de waypoint: navio dentro de `captureRadius_m` de waypoint 0 → `activeWaypointIdx() == 1`
  - [x] Lista esgotada: após capturar todos os waypoints → `waypointsExhausted() == true` e demandas = `0`
  - [x] PID heading: com kp=1, ki=0, kd=0, erro = `0.1 rad` → saída = `0.1 rad` de demanda de leme
  - [x] PID velocidade: com kp=1, ki=0, kd=0, demandedSpeed=3 m/s, SOG=2 m/s → saída = 1.0 (convertida em RPM)
- Integration tests:
  - [x] ManeuverController com `Thruster`/`Rudder` mock: após 100 updates de 0.1s, thrust acumulado move o "navio" simulado em direção ao waypoint
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `wrapToPi` validado para casos de cruzamento de ±π (e.g., bearing 179° vs yaw -179°)
- Captura de waypoint verificada em teste explícito
- Demandas zero após lista esgotada — verificado por teste
