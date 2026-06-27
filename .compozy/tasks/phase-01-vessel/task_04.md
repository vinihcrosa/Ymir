---
status: completed
title: Criar VesselState
type: backend
complexity: low
dependencies: []
---

# Task 4: Criar VesselState

## Overview

Cria o header `VesselState.h` em `libs/vessel/` com enums e structs para representar
o estado operacional visível de uma embarcação: luzes de navegação, shapes COLREGS, e
modo operacional. É um contêiner puramente passivo (nenhuma lógica, nenhum método de
transição) que `DynamicVessel` expõe por tick. Esta task pode ser desenvolvida em
paralelo com qualquer outra task.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `NavigationLight` DEVE ser enum class com valores `Off` e `On`
- `ColregsShape` DEVE ser enum class com valores `Ball`, `Cone`, `Cylinder`, `Diamond`
- `OperationalState` DEVE ser enum class com valores: `Underway`, `Anchored`, `Moored`, `Aground`, `RestrictedManeuverability`, `NotUnderCommand`
- `NavigationLights` DEVE ser struct com campos: `mast`, `port`, `starboard`, `stern`, `anchor` (todos `NavigationLight`, padrão `Off`)
- `VesselState` DEVE ser struct com campos: `lights` (`NavigationLights`), `shapes` (`std::vector<ColregsShape>`), `operationalState` (`OperationalState`, padrão `Underway`)
- `VesselState` DEVE ser header-only (nenhum arquivo `.cpp` necessário)
- Nenhum método de transição automática — transições são responsabilidade da aplicação
</requirements>

## Subtasks

- [x] 4.1 Criar `libs/vessel/include/ymir/vessel/VesselState.h` com todos os enums e structs definidos
- [x] 4.2 Verificar que o header compila sem dependências além de `<vector>` e `<cstdint>`
- [x] 4.3 Criar `libs/vessel/tests/TestVesselState.cpp` com testes de construção padrão e atribuição de campos

## Implementation Details

Ver TechSpec seção "VesselState" para a definição completa dos tipos.

Header-only: nenhuma linha de código C++ com implementação (nenhum método com corpo
além dos valores padrão de struct). Todos os campos usam inicialização in-class.

O campo `shapes` usa `std::vector<ColregsShape>` para permitir que múltiplos shapes
COLREGS coexistam (e.g., navio com restrição de manobra pode ter ball + diamond).

### Relevant Files

- `libs/vessel/include/ymir/vessel/VesselState.h` — arquivo a ser criado

### Dependent Files

- `libs/vessel/include/ymir/vessel/DynamicVessel.h` (task_08) — possui `VesselState` como membro

### Related ADRs

Nenhum ADR específico para VesselState. Ver PRD seção "Funcionalidade 6" para contexto.

## Deliverables

- `libs/vessel/include/ymir/vessel/VesselState.h` (novo, header-only)
- `libs/vessel/tests/TestVesselState.cpp` (novo) **(REQUIRED)**

## Tests

- Unit tests:
  - [x] `VesselState` default-construído tem `operationalState == OperationalState::Underway`
  - [x] `VesselState` default-construído tem todos os campos de `NavigationLights` como `Off`
  - [x] `VesselState` default-construído tem `shapes.empty() == true`
  - [x] Atribuir `operationalState = OperationalState::Anchored` preserva outros campos
  - [x] `shapes.push_back(ColregsShape::Ball)` adiciona um shape; `shapes.size() == 1`
  - [x] Dois `VesselState` default-construídos são iguais (se `operator==` for definido, ou verificar campo a campo)
- Integration tests:
  - [ ] `VesselState` incluído em `DynamicVessel.h` sem conflito de includes (verificado na task_08)
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- Header compila standalone com apenas `<vector>` como dependência externa
- Nenhum método de transição implementado (apenas dados)
