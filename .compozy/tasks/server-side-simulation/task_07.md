---
status: pending
title: Religar `simulationStore` ao `SimulationSocket`
type: frontend
complexity: medium
dependencies:
  - task_06
---

# Task 7: Religar `simulationStore` ao `SimulationSocket`

## Overview
Trocar a implementação interna do `simulationStore` do Web Worker para o `SimulationSocket`
(task_06), preservando a API pública (`play`/`pause`/`reset`/`loadScenario`/`applyEnvironment`). O
estado passa a vir de `ServerMessage{State}`; comandos viram `ClientMessage`. Do ponto de vista dos
componentes React, nada muda.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST preservar a assinatura pública do store (play/pause/reset/loadScenario/applyEnvironment) — sem mudança para os componentes consumidores.
- MUST substituir `Worker`/`postMessage` por chamadas ao `SimulationSocket`, mapeando cada ação a um `ClientMessage`.
- MUST atualizar `state`/`status`/`error` a partir de `ServerMessage` (State/Status/Error) e dos estados de conexão.
- MUST reissuar comandos pendentes (actuadores/ambiente) após (re)conexão, como o store faz hoje no `ready`.
- SHOULD interpolar/suavizar o render entre snapshots (evitar "picote") — ou documentar onde isso será tratado.
- MUST manter os testes de store/estado passando (adaptados ao novo backend).
</requirements>

## Subtasks
- [ ] 7.1 Injetar/instanciar o `SimulationSocket` no store no lugar do `Worker`.
- [ ] 7.2 Mapear play/pause/reset/loadScenario/applyEnvironment → `ClientMessage`.
- [ ] 7.3 Atualizar `state`/`status`/`error` a partir de `ServerMessage` e estados de conexão.
- [ ] 7.4 Reemitir comandos pendentes após (re)conexão.
- [ ] 7.5 Atualizar os testes do store para o novo backend (transporte mockado, sem motor).

## Implementation Details
Ver TechSpec seções "System Architecture" (simulationStore religado) e "Development Sequencing"
(passo 7). Preservar o contrato atual do store descrito em `simulationStore.ts` e a re-sync de
actuadores/ambiente que hoje ocorre no handler `ready`.

### Relevant Files
- `apps/web/src/stores/simulationStore.ts` — store a reescrever internamente.
- `apps/web/src/lib/simulation-socket.ts` — novo backend (task_06).
- `apps/web/src/stores/vesselPanelStore.ts` / `environmentStore.ts` — origem dos comandos pendentes.
- `apps/web/src/stores/simulationStore.test.ts` — testes a adaptar.

### Dependent Files
- Componentes que consomem o store (SimulationControls, VesselPanel, render 3D) — não devem precisar mudar.

### Related ADRs
- [ADR-004: contrato de mensagens](../adrs/adr-004.md).
- [ADR-006: reconexão](../adrs/adr-006.md).

## Deliverables
- `simulationStore` operando sobre `SimulationSocket` com API pública intacta.
- Unit tests com 80%+ de cobertura **(REQUIRED)**
- Integration tests do fluxo store↔socket (transporte mockado) **(REQUIRED)**

## Tests
- Unit tests:
  - [ ] `play()` envia `Play` (ou `CreateSimulation`+`Play` na primeira vez) via socket.
  - [ ] `pause()` envia `Pause` e muda `status` para `paused`.
  - [ ] `reset()` envia `Reset`/`Stop` e zera o estado local.
  - [ ] `loadScenario(vessels)` envia `LoadScenario` com os vessels corretos.
  - [ ] `ServerMessage{State}` atualiza `state`; `Error` seta `status:'error'`.
  - [ ] Comandos pendentes (actuador/ambiente) são reemitidos após reconexão.
- Integration tests:
  - [ ] Fluxo completo store→socket→(mock server)→store reflete estado sem regressão de API.
- Test coverage target: >=80%
- All tests must pass

## Success Criteria
- All tests passing
- Test coverage >=80%
- Componentes consumidores do store não precisam de alteração.
- Paridade de comportamento com o modo atual do cliente.
</content>
