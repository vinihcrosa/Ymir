---
status: completed
title: Implementar entidade Rudder
type: backend
complexity: medium
dependencies:
  - task_01
---

# Task 3: Implementar entidade Rudder

## Overview

Cria a classe `Rudder` em `libs/vessel/` como entidade de domínio análoga ao `Thruster`
(task_02), mas especializada para lemes. Mantém o ângulo atual e o ângulo demandado,
avança a dinâmica via rate limiter em `update(dt)`, e expõe o estado via `toCommand()`
para `RudderForces`. Esta task é paralela à task_02 e pode ser desenvolvida simultaneamente.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `Rudder` DEVE ser construído a partir de `const RudderConfig&` (referência não-owner)
- `Rudder::ActuatorState` DEVE conter: `currentAngle_rad` e `demandedAngle_rad` (ambos `double`, padrão `0.0`)
- `setDemand(double angle_rad)` DEVE atualizar apenas `demandedAngle_rad`; `currentAngle_rad` NÃO muda até `update(dt)`
- `update(double dt)` DEVE aplicar rate limiter: `Δangle = clamp(demanded - current, -rate*dt, +rate*dt)`, onde rate = `cfg.angleSpeed` (rad/s)
- `update(double dt)` NÃO DEVE deixar `currentAngle_rad` ultrapassar `±cfg.angleMaximum_rad`
- `toCommand()` DEVE retornar `RudderForces::RudderCommand` com `currentAngle_rad`
- `Rudder` DEVE ser não-copiável e não-movível
</requirements>

## Subtasks

- [x] 3.1 Criar `libs/vessel/include/ymir/vessel/Rudder.h` com `ActuatorState`, construtores, e declarações de método
- [x] 3.2 Criar `libs/vessel/src/Rudder.cpp` com implementação de `update()`, `setDemand()`, e `toCommand()`
- [x] 3.3 Adicionar `Rudder.cpp` ao `libs/vessel/CMakeLists.txt`
- [x] 3.4 Criar `libs/vessel/tests/TestRudder.cpp` com os casos de teste especificados

## Implementation Details

Ver TechSpec seção "Interfaces Principais — Rudder" para a assinatura completa.

Rate limiter: `double delta = demandedAngle_rad - currentAngle_rad;`
`currentAngle_rad += std::clamp(delta, -rate*dt, +rate*dt);`
Seguido de clamping no máximo: `currentAngle_rad = std::clamp(currentAngle_rad, -maxAngle, +maxAngle);`

`cfg.angleSpeed` em `RudderConfig` é a taxa máxima em rad/s. Verificar que o campo
existe em `VesselConfig.h` — pode ser `angleSpeed` ou derivado de `angleMaximum / time`.

### Relevant Files

- `libs/vessel/include/ymir/vessel/VesselConfig.h` — `RudderConfig` com `angleMaximum`, `angleSpeed`
- `libs/physics/include/ymir/physics/forces/RudderForces.h` — `RudderCommand` struct (task_01)
- `libs/vessel/CMakeLists.txt` — precisa incluir `Rudder.cpp`

### Dependent Files

- `libs/vessel/include/ymir/vessel/DynamicVessel.h` (task_08) — owns `std::vector<Rudder>`
- `libs/vessel/include/ymir/vessel/controllers/ManeuverController.h` (task_06) — recebe `std::vector<Rudder>&`
- `libs/vessel/include/ymir/vessel/controllers/PrescribedController.h` (task_05) — idem

### Related ADRs

- [ADR-003: Thruster e Rudder como Entidades com Estado de Atuador](adrs/adr-003.md) — Define a separação entre config estática e estado dinâmico para Rudder

## Deliverables

- `libs/vessel/include/ymir/vessel/Rudder.h` (novo)
- `libs/vessel/src/Rudder.cpp` (novo)
- `libs/vessel/tests/TestRudder.cpp` (novo) **(REQUIRED)**
- `libs/vessel/CMakeLists.txt` atualizado para incluir `Rudder.cpp`

## Tests

- Unit tests:
  - [x] Rate limiter positivo: `angleSpeed=0.087 rad/s` (~5°/s), demanda `0.35 rad`, `update(1.0)` → `currentAngle_rad == Approx(0.087).epsilon(1e-9)`
  - [x] Rate limiter negativo: ângulo atual `0.35 rad`, demanda `0.0`, `update(1.0)` → `currentAngle_rad == Approx(0.263).epsilon(1e-9)`
  - [x] Clamping no máximo: demanda `10 rad` (além do máximo), após N updates `currentAngle_rad ≤ cfg.angleMaximum_rad`
  - [x] `setDemand()` não altera `currentAngle_rad` imediatamente
  - [x] `toCommand()` retorna `currentAngle_rad` após `update()`
  - [x] Rate limiter simétrico: demanda positiva e demanda negativa com mesma magnitude produzem variações simétricas
- Integration tests:
  - [x] `Rudder` + `RudderForces`: `update()` + `toCommand()` + `setActuatorState()` + `computeNaval()` produz força de leme coerente com ângulo atual
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Rate limiter validado em direções positiva e negativa
- Clamping de ângulo máximo verificado por teste explícito
- `setDemand` não modifica estado imediato — verificado por teste
