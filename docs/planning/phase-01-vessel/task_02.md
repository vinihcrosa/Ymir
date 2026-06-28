---
status: completed
title: Implementar entidade Thruster
type: backend
complexity: medium
dependencies:
  - task_01
---

# Task 2: Implementar entidade Thruster

## Overview

Cria a classe `Thruster` em `libs/vessel/` como entidade de domínio que possui estado
de atuador (RPM atual, pitch atual, azimuth atual) e aplica a dinâmica de primeira ordem
do propulsor a cada tick via `update(dt)`. A separação entre `ThrusterConfig` (imutável)
e `Thruster::ActuatorState` (mutável) é o resultado direto do ADR-003 e é o pré-requisito
para todos os controladores e para `DynamicVessel`.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `Thruster` DEVE ser construído a partir de `const ThrusterConfig&` (referência não-owner)
- `Thruster::ActuatorState` DEVE conter: `currentRPM`, `currentPitch`, `currentAzimuth_deg`, `demandedRPM`, `demandedPitch`, `demandedAzimuth_deg` (todos `double`, padrão `0.0`)
- `setDemand(rpm, pitch, azimuth_deg)` DEVE atualizar apenas os campos `demanded*`; o estado `current*` NÃO muda até `update(dt)` ser chamado
- `update(double dt)` DEVE aplicar filtro de 1ª ordem no RPM: `currentRPM += (1 - exp(-dt/τ)) * (demandedRPM - currentRPM)`, onde τ = `cfg.rotationTime`
- `update(double dt)` DEVE aplicar rate limiter no azimuth: `Δaz = clamp(demanded - current, -rate*dt, +rate*dt)`, onde rate = `cfg.azimuthSpeed`
- `update(double dt)` DEVE aplicar rate limiter no pitch analogamente
- `toCommand()` DEVE retornar `ThrustForces::ThrusterCommand` com os valores `current*` do estado
- `Thruster` DEVE ser não-copiável e não-movível — ver TechSpec
- O construtor DEVE inicializar `state_.currentRPM` com `cfg.initialRPM`
</requirements>

## Subtasks

- [x] 2.1 Criar `libs/vessel/include/ymir/vessel/Thruster.h` com `ActuatorState`, construtores, e declarações de método
- [x] 2.2 Criar `libs/vessel/src/Thruster.cpp` com implementação de `update()`, `setDemand()`, e `toCommand()`
- [x] 2.3 Adicionar `Thruster.cpp` ao `libs/vessel/CMakeLists.txt` (converter de INTERFACE para STATIC se necessário — ver task_08 para a mudança completa de CMakeLists)
- [x] 2.4 Criar `libs/vessel/tests/TestThruster.cpp` com os casos de teste especificados

## Implementation Details

Ver TechSpec seção "Interfaces Principais — Thruster" para a assinatura completa da
classe. A fórmula do filtro de 1ª ordem é `currentRPM += alpha * (demanded - current)`
onde `alpha = 1.0 - std::exp(-dt / cfg.rotationTime)`.

`toCommand()` converte `ActuatorState` para `ThrustForces::ThrusterCommand` — tipo
definido em `libs/physics/` que `libs/vessel/` pode incluir sem dep circular.

Para o pitch, se `ThrusterConfig` não tiver `pitchRate` ainda, usar `azimuthSpeed`
como aproximação conservadora ou adicionar o campo.

### Relevant Files

- `libs/vessel/include/ymir/vessel/VesselConfig.h` — `ThrusterConfig` com `rotationTime`, `azimuthSpeed`, `initialRPM`
- `libs/physics/include/ymir/physics/forces/ThrustForces.h` — `ThrusterCommand` struct (task_01)
- `libs/vessel/CMakeLists.txt` — precisa incluir `Thruster.cpp`

### Dependent Files

- `libs/vessel/include/ymir/vessel/DynamicVessel.h` (task_08) — owns `std::vector<Thruster>`
- `libs/vessel/include/ymir/vessel/controllers/ManeuverController.h` (task_06) — recebe `std::vector<Thruster>&`
- `libs/vessel/include/ymir/vessel/controllers/PrescribedController.h` (task_05) — idem

### Related ADRs

- [ADR-003: Thruster e Rudder como Entidades com Estado de Atuador](adrs/adr-003.md) — Define a separação entre config estática e estado dinâmico

## Deliverables

- `libs/vessel/include/ymir/vessel/Thruster.h` (novo)
- `libs/vessel/src/Thruster.cpp` (novo)
- `libs/vessel/tests/TestThruster.cpp` (novo) **(REQUIRED)**
- `libs/vessel/CMakeLists.txt` atualizado para incluir `Thruster.cpp`

## Tests

- Unit tests:
  - [x] Filtro RPM: `Thruster t(cfg_tau50)` com `demandedRPM=100`, `update(1.0)` → `currentRPM == Approx(100 * (1 - exp(-1.0/50))).epsilon(1e-6)`
  - [x] Filtro RPM: após 5 tau = 250s de updates consecutivos de 1s, `currentRPM ≈ 99.3%` do demandado
  - [x] Rate limiter azimuth: `azimuthSpeed=5°/s`, demanda `90°`, `update(1.0)` → `currentAzimuth_deg == 5.0`
  - [x] Rate limiter azimuth: demanda negativa → azimuth decrementa em `rate*dt`
  - [x] `setDemand()` não altera `current*` antes de `update()`: `setDemand(100, 0, 0)`, checar `state().currentRPM == 0`
  - [x] Construtor inicializa `currentRPM` com `cfg.initialRPM` (não zero quando initialRPM ≠ 0)
  - [x] `toCommand()` retorna `currentRPM` e `currentAzimuth_deg` corretos após `update()`
- Integration tests:
  - [x] `Thruster` + `ThrustForces`: `update()` seguido de `toCommand()` + `setActuatorState()` + `computeNaval()` produz força não-zero coerente com RPM atual
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Filtro de 1ª ordem validado analiticamente (não apenas smoke test)
- Rate limiter validado em ambas as direções (positiva e negativa)
- `setDemand` não modifica estado imediato — verificado por teste explícito
