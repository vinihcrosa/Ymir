---
status: completed
title: Criar DynamicVessel aggregate root
type: backend
complexity: high
dependencies:
  - task_02
  - task_03
  - task_04
  - task_05
  - task_06
  - task_07
---

# Task 8: Criar DynamicVessel aggregate root

## Overview

Cria `DynamicVessel`, o aggregate root da embarcação que integra todas as entidades e
controladores desenvolvidos nas tasks anteriores. Possui `Thruster[]`, `Rudder[]`,
`VesselState`, e o controlador ativo via `VesselController` (std::variant). Orquestra
os dois passos de tick (`updateControl` e `updateStates`) e sincroniza o estado dos
atuadores com os modelos de força via `syncToForceModels()`. Converte `libs/vessel/`
de INTERFACE para STATIC e finaliza o CMakeLists.txt completo do módulo.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `VesselController` DEVE ser: `using VesselController = std::variant<ManeuverController, PrescribedController, BerthManeuverSystem>;`
- `DynamicVessel` DEVE ser não-copiável e não-movível (delete explícito de copy e move)
- `updateControl(t, dt, ctx)` DEVE fazer `std::visit` com lambda genérico que chama `ctrl.update(t, dt, ctx, thrusters_, rudders_)`
- `updateStates(dt)` DEVE chamar `thruster.update(dt)` para cada thruster e `rudder.update(dt)` para cada rudder
- `syncToForceModels(ThrustForces*, RudderForces*)` DEVE chamar `tf->setActuatorState(i, thrusters_[i].toCommand())` para cada thruster e `rf->setActuatorState(i, rudders_[i].toCommand())` para cada rudder; DEVE ser no-op se ponteiros são null
- `vesselState()` DEVE retornar referência mutável e const ao `VesselState` membro
- `thruster(idx)` e `rudder(idx)` DEVEM retornar referências const; DEVEM lançar `std::out_of_range` se idx >= count
- `libs/vessel/CMakeLists.txt` DEVE converter de INTERFACE para STATIC, listar todos os `.cpp` das tasks 02-08, e adicionar `tests/` subdirectory via `YMIR_BUILD_TESTS`
- `libs/vessel/tests/CMakeLists.txt` DEVE criar um executável Catch2 com todos os TestXxx.cpp das tasks 02-07 e este task
</requirements>

## Subtasks

- [x] 8.1 Criar `libs/vessel/include/ymir/vessel/DynamicVessel.h` com alias `VesselController`, classe `DynamicVessel`, e todas as declarações de método
- [x] 8.2 Criar `libs/vessel/src/DynamicVessel.cpp` com implementação de `updateControl()`, `updateStates()`, `syncToForceModels()`, e acessores
- [x] 8.3 Atualizar `libs/vessel/CMakeLists.txt` de INTERFACE para STATIC, incluindo todos os `.cpp` e configuração de testes
- [x] 8.4 Criar `libs/vessel/tests/CMakeLists.cpp` como target Catch2 linkando `ymir_vessel` e listando todos os test files das tasks 02-08
- [x] 8.5 Criar `libs/vessel/tests/TestDynamicVessel.cpp` com os casos de teste especificados
- [x] 8.6 Verificar que `compozy tasks validate --name phase-01-vessel` passa com exit 0

## Implementation Details

Ver TechSpec seção "DynamicVessel" para assinatura completa e seção "Estrutura de Arquivos"
para o CMakeLists.txt final.

`DynamicVessel` inclui `ThrustForces` e `RudderForces` em `syncToForceModels()` — isso
cria uma dependência de `libs/vessel/` → `libs/physics/forces/` que já existe (libs/vessel
linka ymir_physics).

O construtor deve inicializar `controller_` com um `PrescribedController` vazio como
default (tabelas vazias → demandas zero). Isso garante que `updateControl()` nunca toca
um variant não-inicializado.

`libs/vessel/tests/CMakeLists.txt` exemplo:
```cmake
add_executable(ymir_vessel_tests
    TestThruster.cpp
    TestRudder.cpp
    TestVesselState.cpp
    TestPrescribedController.cpp
    TestManeuverController.cpp
    TestBerthManeuverSystem.cpp
    TestDynamicVessel.cpp
)
target_link_libraries(ymir_vessel_tests PRIVATE ymir_vessel Catch2::Catch2WithMain)
catch_discover_tests(ymir_vessel_tests)
```

### Relevant Files

- `libs/vessel/include/ymir/vessel/Thruster.h` (task_02)
- `libs/vessel/include/ymir/vessel/Rudder.h` (task_03)
- `libs/vessel/include/ymir/vessel/VesselState.h` (task_04)
- `libs/vessel/include/ymir/vessel/controllers/PrescribedController.h` (task_05)
- `libs/vessel/include/ymir/vessel/controllers/ManeuverController.h` (task_06)
- `libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h` (task_07)
- `libs/physics/include/ymir/physics/forces/ThrustForces.h` (task_01)
- `libs/physics/include/ymir/physics/forces/RudderForces.h` (task_01)
- `libs/vessel/CMakeLists.txt` — a ser convertido

### Dependent Files

- `libs/simulation/include/ymir/simulation/NavalSimulation.h` (task_09) — recebe `DynamicVessel&` em `registerVessel()`

### Related ADRs

- [ADR-001: DynamicVessel como Hub de Integração](adrs/adr-001.md) — DynamicVessel é a decisão central desta task
- [ADR-002: std::variant para Despacho de Controlador](adrs/adr-002.md) — VesselController alias definido aqui
- [ADR-003: Thruster e Rudder como Entidades com Estado de Atuador](adrs/adr-003.md) — syncToForceModels() implementa o padrão de sync

## Deliverables

- `libs/vessel/include/ymir/vessel/DynamicVessel.h` (novo)
- `libs/vessel/src/DynamicVessel.cpp` (novo)
- `libs/vessel/CMakeLists.txt` (atualizado: INTERFACE → STATIC)
- `libs/vessel/tests/CMakeLists.txt` (novo)
- `libs/vessel/tests/TestDynamicVessel.cpp` (novo) **(REQUIRED)**
- Target Catch2 `ymir_vessel_tests` compilável e executável

## Tests

- Unit tests:
  - [x] `DynamicVessel` com `ManeuverController` setado: `updateControl()` chama `setDemand()` no thruster (verificar via getter do estado do thruster pós-update)
  - [x] `DynamicVessel` com `PrescribedController` setado: troca de controlador via `setController()` muda o tipo ativo sem crash
  - [x] `updateStates()` avança `currentRPM` do thruster (verificar que currentRPM mudou após update)
  - [x] `syncToForceModels(tf, rf)` com `tf = nullptr`: sem crash (no-op para null)
  - [x] `thruster(10)` lança `std::out_of_range` quando há apenas 2 thrusters
  - [x] Sequência completa: `updateControl → updateStates → syncToForceModels` com mocks de ThrustForces/RudderForces verifica que comandos corretos chegam nos modelos de força
  - [x] `vesselState().operationalState` pode ser alterado externamente via referência mutável
- Integration tests:
  - [x] DynamicVessel com ManeuverController + Thruster real + Rudder real + ThrustForces real: após 10 ticks, thrust acumulado é não-zero e coerente com demanda
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `libs/vessel/` compila como STATIC sem warnings
- `ymir_vessel_tests` executa com sucesso cobrindo todos os módulos das tasks 02-08
- `compozy tasks validate --name phase-01-vessel` retorna exit 0
