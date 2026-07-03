---
status: completed
title: Protocolo `ClientMessage`/`ServerMessage` em `packages/types` (add-only)
type: backend
complexity: low
dependencies: []
---

# Task 2: Protocolo `ClientMessage`/`ServerMessage` em `packages/types` (add-only)

## Overview
Adicionar os DTOs de protocolo de rede (`ClientMessage`, `ServerMessage`) em `packages/types`,
reaproveitando os payloads existentes (`SimulationStateDTO`, `VesselConfigDTO`, `ScenarioDraftVessel`).
É estritamente aditivo — NÃO remove `WorkerMessageDTO` nem a union `mock` (removidos só na task_08,
após o cliente migrar), para não quebrar o build do web no meio.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST adicionar `ClientMessage` e `ServerMessage` como unions discriminadas por `type`, em TypeBox, conforme a seção "Data Models" do TechSpec.
- MUST reutilizar os DTOs existentes como payloads; NÃO duplicar suas definições.
- MUST exportar os novos tipos via `packages/types/src/index.ts`.
- MUST NOT remover `WorkerMessageDTO` nem a union `SimulationEngine = 'wasm'|'mock'` nesta task.
- MUST cobrir os comandos atuais (Create/Attach/Play/Pause/Reset/LoadScenario/SetActuator/LoadEnvironment) e eventos (SimulationCreated/State/Status/VesselConfig/Error).
</requirements>

## Subtasks
- [x] 2.1 Criar `packages/types/src/protocol.ts` com `ClientMessage` e `ServerMessage`.
- [x] 2.2 Reusar `SimulationStateDTO`/`VesselConfigDTO`/`ScenarioDraftVessel` como payloads.
- [x] 2.3 Exportar o novo módulo no `index.ts`.
- [x] 2.4 Escrever testes de validação de shape (aceite/rejeição) para ambos os contratos.

## Implementation Details
Ver TechSpec seção "Data Models" (definições de `ClientMessage`/`ServerMessage`). Seguir o padrão de
`packages/types/src/simulation.ts` (TypeBox + `Static`). O `ScenarioDraftVessel` usado no
`LoadScenario` deve ser tipado (hoje ele é declarado ad-hoc no worker/store); definir/reusar um DTO
compartilhado.

### Relevant Files
- `packages/types/src/simulation.ts` — padrão TypeBox; `WorkerMessageDTO` (referência, não remover aqui).
- `packages/types/src/vessel.ts` — `VesselConfigDTO` reutilizado.
- `packages/types/src/scenario.ts` — base para `ScenarioDraftVessel`.
- `packages/types/src/index.ts` — barril de exports.

### Dependent Files
- `apps/api` módulo de simulação (task_03/05) — consome os DTOs para validar comandos.
- `apps/web/src/lib/simulation-socket.ts` (task_06) — usa os DTOs no cliente.

### Related ADRs
- [ADR-004: Transporte WebSocket + JSON com contrato `ClientMessage`/`ServerMessage`](../adrs/adr-004.md) — define o contrato.

## Deliverables
- `protocol.ts` com `ClientMessage`/`ServerMessage` exportados.
- DTO compartilhado para `ScenarioDraftVessel` (se ainda não existir).
- Unit tests com 80%+ de cobertura **(REQUIRED)**
- Integration tests: validação dos contratos contra exemplos representativos **(REQUIRED)**

## Tests
- Unit tests:
  - [ ] `ClientMessage` aceita `{type:'SetActuator', simId, vesselId, deviceType:'rudder', deviceId, value}` válido.
  - [ ] `ClientMessage` rejeita `deviceType:'invalid'` e `type` desconhecido.
  - [ ] `ClientMessage` rejeita `Play` sem `simId`.
  - [ ] `ServerMessage` aceita `{type:'State', payload: <SimulationStateDTO válido>}` e rejeita payload malformado.
  - [ ] `ServerMessage` aceita `{type:'SimulationCreated', simId}`.
- Integration tests:
  - [ ] Serializar → JSON → desserializar → validar um `ClientMessage` e um `ServerMessage` sem perda.
- Test coverage target: >=80%
- All tests must pass

## Success Criteria
- All tests passing
- Test coverage >=80%
- `@ymir/types` compila; `WorkerMessageDTO` e union `mock` permanecem intactos.
- Todos os comandos/eventos do TechSpec estão representados.
</content>
