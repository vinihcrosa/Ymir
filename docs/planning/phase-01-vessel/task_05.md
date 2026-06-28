---
status: completed
title: Implementar PrescribedController
type: backend
complexity: medium
dependencies:
  - task_02
  - task_03
---

# Task 5: Implementar PrescribedController

## Overview

Cria `PrescribedController` que reproduz séries temporais de comandos de RPM e ângulo
de leme por interpolação linear, alimentando diretamente as entidades `Thruster` e
`Rudder`. É o mecanismo de validação contra dados experimentais: erro de interpolação
< 1e-6 é requisito hard. Esta task é paralela à task_06 (ManeuverController) pois ambas
dependem apenas de task_02 e task_03.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `PrescribedController::TimeSeries` DEVE ser struct com `std::vector<double> times` (monotônico crescente) e `std::vector<double> values`
- `PrescribedController::Config` DEVE conter `std::vector<TimeSeries> thrusterRPM` (um por thruster) e `std::vector<TimeSeries> rudderAngle_rad` (um por rudder)
- `update()` DEVE chamar `setDemand()` em cada thruster com o valor interpolado de `thrusterRPM[i]` em `t`
- `update()` DEVE chamar `setDemand()` em cada rudder com o valor interpolado de `rudderAngle_rad[i]` em `t`
- `interpolate(ts, t)` DEVE usar `std::lower_bound` para localizar o intervalo e interpolar linearmente
- Para `t < times.front()`: DEVE retornar `values.front()`
- Para `t > times.back()`: DEVE retornar `values.back()`
- Erro de interpolação DEVE ser < 1e-6 para pontos exatos da tabela e para interpolação entre pontos
- `update()` DEVE ter a assinatura: `void update(double t, double dt, const NavalContext& ctx, std::vector<Thruster>& thrusters, std::vector<Rudder>& rudders)`
- Se `Config.thrusterRPM.size() != thrusters.size()`, DEVE usar `std::min` (não crash)
</requirements>

## Subtasks

- [x] 5.1 Criar `libs/vessel/include/ymir/vessel/controllers/PrescribedController.h` com `TimeSeries`, `Config`, e declarações de método
- [x] 5.2 Criar `libs/vessel/src/controllers/PrescribedController.cpp` com implementação de `update()` e `interpolate()`
- [x] 5.3 Adicionar `PrescribedController.cpp` ao `libs/vessel/CMakeLists.txt`
- [x] 5.4 Criar `libs/vessel/tests/TestPrescribedController.cpp` com os casos de teste especificados

## Implementation Details

Ver TechSpec seção "PrescribedController" para assinatura e algoritmo de interpolação.

`interpolate` estático e `noexcept`: `std::lower_bound` retorna iterator para o primeiro
elemento `>= t`. Se o iterator apontar para o início, retorna `values.front()`. Se
apontar para além do final, retorna `values.back()`. Caso contrário, interpola entre
o elemento anterior e o atual.

`dt` e `ctx` são ignorados por este controlador (a série temporal já incorpora a física
gravada); o parâmetro existe apenas para satisfazer o conceito `VesselController`.

### Relevant Files

- `libs/vessel/include/ymir/vessel/Thruster.h` (task_02) — `setDemand()` chamado em `update()`
- `libs/vessel/include/ymir/vessel/Rudder.h` (task_03) — `setDemand()` chamado em `update()`
- `libs/vessel/include/ymir/vessel/NavalContext.h` — parâmetro do método `update()`

### Dependent Files

- `libs/vessel/include/ymir/vessel/DynamicVessel.h` (task_08) — usa PrescribedController como alternativa no variant

### Related ADRs

- [ADR-002: std::variant para Despacho de Controlador](adrs/adr-002.md) — PrescribedController satisfaz o conceito implícito requerido pelo std::visit

## Deliverables

- `libs/vessel/include/ymir/vessel/controllers/PrescribedController.h` (novo)
- `libs/vessel/src/controllers/PrescribedController.cpp` (novo)
- `libs/vessel/tests/TestPrescribedController.cpp` (novo) **(REQUIRED)**
- `libs/vessel/CMakeLists.txt` atualizado

## Tests

- Unit tests:
  - [x] Interpolação em ponto exato da tabela: `t == times[2]` → `result == values[2]` com erro = 0
  - [x] Interpolação entre dois pontos: `t = (times[0]+times[1])/2` → `result == (values[0]+values[1])/2` com `Approx(...).epsilon(1e-6)`
  - [x] `t < times.front()` → retorna `values.front()`
  - [x] `t > times.back()` → retorna `values.back()`
  - [x] `update()` com thruster RPM series chama `setDemand(rpm, ...)` com valor correto no thruster 0
  - [x] `update()` com rudder series chama `setDemand(angle)` com valor correto no rudder 0
  - [x] `Config.thrusterRPM.size() < thrusters.size()` → apenas os primeiros thrusters são atualizados (sem crash)
  - [x] Série temporal com um único ponto: qualquer `t` retorna o único valor
- Integration tests:
  - [x] PrescribedController com série cosseno (50 pontos em 60s) + Thruster + ThrustForces: RPM atual após 60s steps de 0.1s é verificável analiticamente com tolerância de integração
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Erro de interpolação < 1e-6 verificado em casos de teste com pontos exatos e medianos
- Sem crash para séries de tamanho inconsistente com número de thrusters/rudders
