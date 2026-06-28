---
status: completed
title: Refatorar NavalSimulation para N corpos
type: refactor
complexity: high
dependencies:
    - task_08
---

# Task 9: Refatorar NavalSimulation para N corpos

## Overview

Refatora `NavalSimulation` de single-body para N corpos, mudando o construtor de
`NavalSimulation(unique_ptr<RigidBody6DOF>)` para `addBody(id, unique_ptr<RigidBody6DOF>)`,
agrupando estado por corpo em `BodyEntry`, e adicionando `registerVessel()` para integrar
`DynamicVessel` no tick. Esta é a maior breaking change da Fase 1: todos os testes
existentes de NavalSimulation precisam ser migrados. A task também adiciona testes de
integração end-to-end que validam os critérios de aceitação da Fase 1.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- Construtor padrão `NavalSimulation()` DEVE ser o único construtor público; o construtor `NavalSimulation(unique_ptr<RigidBody6DOF>)` DEVE ser removido
- `addBody(int id, unique_ptr<RigidBody6DOF> body)` DEVE registrar o corpo em `sim_` e criar um `BodyEntry` em `entries_[id]`
- `addNavalForceModel(int bodyId, unique_ptr<NavalForceModel> model)` DEVE associar o modelo ao `BodyEntry` correto e chamar `model->bindContext(&entry.ctx)`
- `registerVessel(int bodyId, DynamicVessel& vessel, ThrustForces* tf, RudderForces* rf)` DEVE armazenar ponteiros não-owners em `entries_[bodyId]`
- `state(int bodyId)` DEVE substituir `state()` sem argumento; DEVE lançar `std::out_of_range` para bodyId desconhecido
- `step(dt)` DEVE iterar sobre `entries_` em ordem de ID crescente, executar `buildContext`, chamar `vessel->updateControl/updateStates/syncToForceModels` se vessel não-null, e então `sim_.step(dt)`
- `initialize()` DEVE inicializar EMA (`q_avg`) de cada entry com a posição inicial do corpo correspondente
- `reset()` DEVE resetar EMA e chamar `resetState()` em todos os modelos de todos os entries
- Testes existentes `TestNavalInfra.cpp`, `TestNavalIntegration.cpp`, `TestSimulationIntegration.cpp` DEVEM ser migrados para a nova API sem regredir em cobertura
- Dois corpos com IDs distintos DEVEM ter steps independentes (estado de um não afeta o outro)
</requirements>

## Subtasks

- [ ] 9.1 Reescrever `libs/simulation/include/ymir/simulation/NavalSimulation.h` com `BodyEntry` struct e nova API pública
- [ ] 9.2 Reescrever `libs/simulation/src/NavalSimulation.cpp` com nova implementação de `step()`, `initialize()`, `reset()`, `buildContext()` por entry
- [ ] 9.3 Migrar `tests/simulation/TestNavalInfra.cpp` para nova API: substituir `NavalSimulation sim(makeTestBody())` por `NavalSimulation sim; sim.addBody(0, makeTestBody()); ` e `sim.state()` por `sim.state(0)`
- [ ] 9.4 Migrar `tests/simulation/TestNavalIntegration.cpp` analogamente
- [ ] 9.5 Verificar `tests/simulation/TestSimulationIntegration.cpp` e migrar se necessário
- [ ] 9.6 Criar testes de integração end-to-end em `tests/simulation/TestVesselIntegration.cpp`

## Implementation Details

Ver TechSpec seção "NavalSimulation Refatorado (N corpos)" para a definição completa
de `BodyEntry` e as assinaturas de todos os métodos.

`buildContext(BodyEntry& entry, double dt)`: análogo ao atual `buildContext(dt)`, mas
opera sobre `entry.body` e `entry.q_avg` em vez dos membros escalares. A lógica de EMA
permanece: `alpha = 1 - exp(-dt/16.5)`, aplicada por componente de `q_avg`.

`step(dt)` expandido:
1. Para cada entry: `entry.ctx = buildContext(entry, dt)` → atualiza contexto
2. Para cada entry com vessel: `vessel->updateControl(...)`, `vessel->updateStates(...)`, `vessel->syncToForceModels(...)`
3. `sim_.step(dt)` — integra todos os corpos
4. EMA update para cada entry

`libs/simulation/CMakeLists.txt` deve incluir `ymir_vessel` em `target_link_libraries`
(para que `DynamicVessel.h` seja encontrado). Verificar se isso cria dep circular —
a dep já existe (vessel depende de physics, simulation depende de physics; simulation
depende de vessel é nova mas válida na hierarquia).

### Relevant Files

- `libs/simulation/include/ymir/simulation/NavalSimulation.h` — a ser reescrito
- `libs/simulation/src/NavalSimulation.cpp` — a ser reescrito
- `libs/simulation/CMakeLists.txt` — adicionar `ymir_vessel` em target_link_libraries
- `tests/simulation/TestNavalInfra.cpp` — a ser migrado
- `tests/simulation/TestNavalIntegration.cpp` — a ser migrado
- `tests/simulation/TestSimulationIntegration.cpp` — verificar e migrar
- `libs/vessel/include/ymir/vessel/DynamicVessel.h` (task_08)

### Dependent Files

- Todos os cenários e apps que instanciam `NavalSimulation` com o construtor antigo precisarão migrar

### Related ADRs

- [ADR-004: Refatoração de NavalSimulation para N Corpos](adrs/adr-004.md) — Motiva e define esta task; detalha BodyEntry e justificativa do breaking change

## Deliverables

- `libs/simulation/include/ymir/simulation/NavalSimulation.h` (reescrito)
- `libs/simulation/src/NavalSimulation.cpp` (reescrito)
- `libs/simulation/CMakeLists.txt` (atualizado: adiciona `ymir_vessel`)
- `tests/simulation/TestNavalInfra.cpp` (migrado)
- `tests/simulation/TestNavalIntegration.cpp` (migrado)
- `tests/simulation/TestVesselIntegration.cpp` (novo) **(REQUIRED)**
- Todos os testes existentes passando após migração

## Tests

- Unit tests (novos — testar API N-body diretamente):
  - [ ] `addBody(0, body0); addBody(1, body1); state(0)` retorna estado do body 0 (não body 1)
  - [ ] `state(99)` sem `addBody(99)` lança `std::out_of_range`
  - [ ] `step(0.1)` com dois corpos avança `time()` para 0.1 independentemente de cada corpo
  - [ ] `initialize()` define `q_avg` de cada entry com a posição inicial do corpo correspondente
  - [ ] `registerVessel(0, vessel, tf, rf)` sem crash; `step()` chama `vessel.updateControl()` (verificar via spy/mock)
- Testes migrados (regressão):
  - [ ] Todos os testes de `TestNavalInfra.cpp` passam com nova API (apenas mudança de construtor e `state(0)`)
  - [ ] Todos os testes de `TestNavalIntegration.cpp` passam com nova API
- Integration tests (end-to-end — `TestVesselIntegration.cpp`):
  - [ ] `ManeuverController` end-to-end: navio em (0,0), waypoints A=(500,0), B=(500,500), captureRadius=50m; após N steps de 0.5s, `state(0).x()` converge em direção a A
  - [ ] `PrescribedController` replay: série temporal de RPM crescente aplicada; `thruster(0).state().currentRPM` após 60s de steps de 0.1s é consistente com o filtro de 1ª ordem
  - [ ] N=2 corpos independentes: body 0 com ManeuverController, body 1 sem vessel; ambos avançam sem interferência mútua nos estados
  - [ ] `BerthManeuverSystem` + `TugParametricForces`: navio próximo do berth entra na fase Sideway; `tugForces` não-zero aparecem no output de força do corpo
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Nenhum uso do construtor antigo `NavalSimulation(unique_ptr<RigidBody6DOF>)` permanece no codebase
- Dois corpos em um NavalSimulation têm estados independentes — verificado por teste
- Critério de aceitação da Fase 1: navio com ManeuverController navega A→B sem desvio > captureRadius
