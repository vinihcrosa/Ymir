---
status: completed
title: Refactor ThrustForces, RudderForces e VesselConfig para estado de atuador externo
type: refactor
complexity: medium
dependencies: []
---

# Task 1: Refactor ThrustForces, RudderForces e VesselConfig para estado de atuador externo

## Overview

Remove o gerenciamento interno de estado de atuador de `ThrustForces` e `RudderForces`,
substituindo-o por structs de comando externos (`ThrusterCommand`, `RudderCommand`) que
serão alimentados pelas entidades `Thruster` e `Rudder` (tasks 02–03). Renomeia
`commandRPM` para `initialRPM` em `ThrusterConfig` para deixar claro que config é
imutável em runtime. Esta task cria a interface de ponte que permite libs/vessel/ e
libs/physics/ coexistirem sem dependência circular.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- ThrustForces DEVE expor `struct ThrusterCommand { double currentRPM; double currentAzimuth_deg; double currentPitch; }` e método `void setActuatorState(std::size_t id, const ThrusterCommand& cmd) noexcept`
- ThrustForces DEVE remover o membro `currentRPM_` e toda lógica de filtro de 1ª ordem de `computeNaval()`
- ThrustForces `computeNaval()` DEVE ler `commands_[i].currentRPM` para calcular thrust (não mais computar o filtro internamente)
- RudderForces DEVE expor `struct RudderCommand { double currentAngle_rad; }` e método `void setActuatorState(std::size_t id, const RudderCommand& cmd) noexcept`
- RudderForces DEVE remover o membro `currentAngle_` e toda lógica de rate limiter de `computeNaval()`
- `ThrusterConfig.commandRPM` DEVE ser renomeado para `initialRPM` com valor padrão `0.0`
- Todos os testes existentes que passam `commandRPM` em config DEVEM ser migrados para `initialRPM`
- `computeNaval()` em ambas as classes DEVE ser função pura dado o estado externo (sem side effects de estado)
</requirements>

## Subtasks

- [x] 1.1 Adicionar `ThrusterCommand` struct e `setActuatorState()` em `ThrustForces.h`; adicionar `commands_` como membro privado; remover `currentRPM_`
- [x] 1.2 Refatorar `ThrustForces::computeNaval()` para ler de `commands_[i]` em vez de computar filtro; remover código de atualização de RPM
- [x] 1.3 Adicionar `RudderCommand` struct e `setActuatorState()` em `RudderForces.h`; adicionar `commands_` como membro privado; remover `currentAngle_`
- [x] 1.4 Refatorar `RudderForces::computeNaval()` para ler de `commands_[i]` em vez de aplicar rate limiter; remover código de atualização de ângulo
- [x] 1.5 Renomear `ThrusterConfig.commandRPM` → `initialRPM` em `ThrustForces.h`; atualizar todos os callers (testes) para usar o novo nome
- [x] 1.6 Verificar que testes existentes de ThrustForces e RudderForces passam com a nova API (ajustar setup de testes para chamar `setActuatorState()` antes de `computeNaval()`)

## Implementation Details

Ver TechSpec seção "Modificações em ThrustForces / RudderForces" para as assinaturas
exatas de `ThrusterCommand`, `RudderCommand`, e `setActuatorState()`.

O construtor de `ThrustForces` deve inicializar `commands_` com o RPM inicial de cada
thruster lendo `cfg.thrusters[i].initialRPM` (antigo `commandRPM`). Isso mantém o
comportamento de warm-start.

Atenção: `ThrustForces::getThrust(id)` (usado por RudderForces para slipstream) deve
continuar funcionando — lê de `lastThrust_` que é atualizado em `computeNaval()`.

### Relevant Files

- `libs/physics/include/ymir/physics/forces/ThrustForces.h` — classe principal a ser modificada
- `libs/physics/src/forces/ThrustForces.cpp` — implementação a ser refatorada
- `libs/physics/include/ymir/physics/forces/RudderForces.h` — classe principal a ser modificada
- `libs/physics/src/forces/RudderForces.cpp` — implementação a ser refatorada
- `libs/vessel/include/ymir/vessel/VesselConfig.h` — renomear `commandRPM` → `initialRPM`

### Dependent Files

- `libs/vessel/include/ymir/vessel/Thruster.h` (task_02) — usará `ThrustForces::ThrusterCommand`
- `libs/vessel/include/ymir/vessel/Rudder.h` (task_03) — usará `RudderForces::RudderCommand`
- Todos os arquivos de cenário e teste que referenciam `commandRPM` ou criam `ThrustForces`/`RudderForces` com estado interno

### Related ADRs

- [ADR-003: Thruster e Rudder como Entidades com Estado de Atuador](adrs/adr-003.md) — Decisão que motiva esta refatoração; define ThrusterCommand/RudderCommand como bridge sem dep circular

## Deliverables

- `ThrustForces.h/.cpp` refatorados com `ThrusterCommand` e `setActuatorState()`
- `RudderForces.h/.cpp` refatorados com `RudderCommand` e `setActuatorState()`
- `VesselConfig.h` com `ThrusterConfig.initialRPM` (antigo `commandRPM`)
- Todos os cenários e testes existentes migrados para `initialRPM`
- Testes unitários atualizados validando comportamento do `computeNaval()` com estado externo **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `ThrustForces::computeNaval()` com `setActuatorState(0, {rpm=100, az=0, pitch=0})` produz força de empuxo idêntica ao comportamento anterior (regressão de força)
  - [x] `ThrustForces::computeNaval()` com `setActuatorState(0, {rpm=0, ...})` produz força zero (rpm zero → sem empuxo)
  - [x] `ThrustForces::computeNaval()` com azimuth 90° produz força lateral (não frontal)
  - [x] `RudderForces::computeNaval()` com `setActuatorState(0, {angle=0.3})` produz força de leme esperada para esse ângulo
  - [x] `RudderForces::computeNaval()` com `setActuatorState(0, {angle=0.0})` produz força zero
  - [x] Construtor de `ThrustForces` inicializa `commands_[i].currentRPM` com `cfg.thrusters[i].initialRPM`
- Integration tests:
  - [x] Cenário existente de NavalSimulation completa step sem crash após migração (79/79 passando)
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `computeNaval()` em ThrustForces e RudderForces não modifica mais estado interno (verificável por `const` analysis ou revisão de código)
- Nenhum uso de `commandRPM` permanece no codebase
- Nenhuma regressão de força nos cenários existentes
