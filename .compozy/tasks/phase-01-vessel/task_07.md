---
status: completed
title: Implementar BerthManeuverSystem
type: backend
complexity: high
dependencies:
  - task_06
---

# Task 7: Implementar BerthManeuverSystem

## Overview

Cria a FSM de atracação com três estados (`Navigating`, `Sideway`, `TurnROTTUG`) que
orquestra estratégias de controle diferentes por fase e injeta forças de rebocadores
paramétricos. Os rebocadores não são corpos físicos independentes nesta fase — suas
forças são calculadas diretamente pelo BSM e precisam ser expostas para um
`TugParametricForces : NavalForceModel` externo que as injeta no corpo físico antes
do passo de integração CVODE.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `BerthManeuverSystem::Phase` DEVE ser enum class com `Navigating`, `Sideway`, `TurnROTTUG`
- `BerthManeuverSystem::TugForceConfig` DEVE conter `escortForce_N`, `pushForce_N`, `bearing_deg`, `arm_m`
- `BerthManeuverSystem::BerthWaypoint` DEVE conter `x_m`, `y_m`, `targetPhase`, `transitionDist_m`, `headingTarget_rad`
- Transição `Navigating→Sideway` DEVE ocorrer quando distância ao waypoint ativo < `transitionDist_m`
- Transição `Sideway→TurnROTTUG` DEVE ocorrer quando erro lateral < `2.0m` E erro de heading < `5°`
- Transições reversas (e.g., `Sideway→Navigating`) NÃO DEVEM ocorrer
- `currentPhase()` DEVE retornar o estado FSM atual
- `update()` DEVE acumular força de rebocadores em `tugForces_` (Vector6) por tick
- `tugForces()` DEVE expor `const Vector6&` para leitura por `TugParametricForces`
- `TugParametricForces : NavalForceModel` DEVE ser criado junto com esta task — lê de ponteiro não-owner para `BerthManeuverSystem` em `computeNaval()`
- `update()` DEVE ter assinatura compatível com o conceito `VesselController`
</requirements>

## Subtasks

- [x] 7.1 Criar `libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h` com `Phase`, structs de config, e declarações de método
- [x] 7.2 Criar `libs/vessel/src/controllers/BerthManeuverSystem.cpp` com FSM, lógica de transição, e cálculo de força de rebocadores
- [x] 7.3 Criar `libs/vessel/include/ymir/vessel/controllers/TugParametricForces.h` e `.cpp` — `NavalForceModel` que lê `tugForces()` do BSM
- [x] 7.4 Adicionar ambos os `.cpp` ao `libs/vessel/CMakeLists.txt`; assegurar que `ymir_vessel` linka `ymir_physics` (para herdar de `NavalForceModel`)
- [x] 7.5 Criar `libs/vessel/tests/TestBerthManeuverSystem.cpp` com os casos de teste especificados

## Implementation Details

Ver TechSpec seção "BerthManeuverSystem" para assinatura completa e tabela de transições.

Fase `Navigating`: delega para lógica LOS+PID idêntica ao `ManeuverController` (reutilizar
a lógica via composição — BSM possui um `ManeuverController` interno para esta fase).
Rebocadores em modo ESCORTING: força aplicada na direção do fairlead, magnitude `escortForce_N`.

Fase `Sideway`: heading fixo (manter yaw alvo via PID de heading), deslocamento lateral
via rebocadores em PUSH com PD (`lateralKp`, `lateralKd`).

Fase `TurnROTTUG`: PID de Rate-of-Turn (ROT = `ctx.state.r()`) via leme; rebocadores
em PUSH para sway.

`TugParametricForces`: em `computeNaval()`, lê `bsm_->tugForces()` e retorna as forças
diretamente como `Forces`. Não mantém estado próprio.

### Relevant Files

- `libs/vessel/include/ymir/vessel/controllers/ManeuverController.h` (task_06) — reutilizado internamente para fase Navigating
- `libs/vessel/include/ymir/vessel/Thruster.h` (task_02) — recebe demandas
- `libs/vessel/include/ymir/vessel/Rudder.h` (task_03) — recebe demandas
- `libs/vessel/include/ymir/vessel/NavalContext.h` — posição, yaw, ROT, velocidade lateral
- `libs/physics/include/ymir/physics/NavalForceModel.h` — base para `TugParametricForces`
- `libs/physics/include/ymir/physics/Forces.h` — tipo de retorno de `computeNaval()`

### Dependent Files

- `libs/vessel/include/ymir/vessel/DynamicVessel.h` (task_08) — usa BerthManeuverSystem no variant
- `libs/simulation/include/ymir/simulation/NavalSimulation.h` (task_09) — registra TugParametricForces no corpo

### Related ADRs

- [ADR-002: std::variant para Despacho de Controlador](adrs/adr-002.md) — BerthManeuverSystem satisfaz o conceito implícito
- [ADR-001: DynamicVessel como Hub de Integração](adrs/adr-001.md) — BSM é um dos controladores gerenciados por DynamicVessel

## Deliverables

- `libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h` (novo)
- `libs/vessel/src/controllers/BerthManeuverSystem.cpp` (novo)
- `libs/vessel/include/ymir/vessel/controllers/TugParametricForces.h` (novo)
- `libs/vessel/src/controllers/TugParametricForces.cpp` (novo)
- `libs/vessel/tests/TestBerthManeuverSystem.cpp` (novo) **(REQUIRED)**
- `libs/vessel/CMakeLists.txt` atualizado

## Tests

- Unit tests:
  - [x] `currentPhase() == Phase::Navigating` no estado inicial
  - [x] Transição `Navigating→Sideway`: navio a `< transitionDist_m` do waypoint ativo → `currentPhase() == Phase::Sideway`
  - [x] Sem transição reversa: após entrar em `Sideway`, setar distância grande → `currentPhase()` permanece `Sideway`
  - [x] Transição `Sideway→TurnROTTUG`: erro lateral < 2m e heading error < 5° → `currentPhase() == Phase::TurnROTTUG`
  - [x] `tugForces()` retorna Vector6 zero quando fase é `Navigating` com tugs em ESCORTING e força zero configurada
  - [x] `tugForces()` retorna força lateral positiva quando fase é `Sideway` e `pushForce_N > 0`
  - [x] FSM não avança além de `TurnROTTUG` (estado final)
  - [x] `TugParametricForces::computeNaval()` retorna as mesmas forças que `bsm.tugForces()`
- Integration tests:
  - [x] BSM com config completa + Thruster + Rudder: após transição `Navigating→Sideway`, demanda de leme reduz (heading fixo) e tugForces lateral aumenta
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- As três transições da FSM verificadas por testes independentes
- Sem transição reversa — verificado explicitamente
- `TugParametricForces` compila e retorna forças consistentes com BSM
