---
status: pending
title: '`SimulationSocket` (web) — WS client, reconexão, `localStorage`'
type: frontend
complexity: medium
dependencies:
  - task_02
  - task_05
---

# Task 6: `SimulationSocket` (web) — WS client, reconexão, `localStorage`

# Overview
Criar o cliente WebSocket do front (`apps/web/src/lib/simulation-socket.ts`): conecta ao `/ws`,
(de)serializa `ClientMessage`/`ServerMessage`, guarda o `sim_id` em `localStorage`, reconecta e
reanexa via `AttachSimulation` ao reabrir. É a peça que substitui o transporte do Web Worker.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST conectar ao endpoint WebSocket via URL configurável (`VITE_SIM_WS_URL`, default `ws://localhost:8080`/host do api).
- MUST enviar `ClientMessage` e decodificar `ServerMessage` (tipos de task_02).
- MUST persistir o `sim_id` em `localStorage` ao receber `SimulationCreated` e enviar `AttachSimulation{sim_id}` na reabertura.
- MUST limpar o `localStorage` e recomeçar quando `AttachSimulation` falhar (sim inexistente).
- MUST reconectar automaticamente em queda de conexão, expondo estados de conexão (conectando/conectado/reconectando/indisponível).
- MUST NOT executar física nem depender de WASM (cliente é terminal puro).
</requirements>

## Subtasks
- [ ] 6.1 Implementar conexão/reconexão ao `/ws` com URL configurável.
- [ ] 6.2 (De)serializar mensagens e expor callbacks de estado/status/erro.
- [ ] 6.3 Persistir/ler `sim_id` no `localStorage` e reanexar na reabertura.
- [ ] 6.4 Tratar falha de attach (limpar storage, recomeçar) e expor estados de conexão.
- [ ] 6.5 Escrever testes com WebSocket de teste e `localStorage` mockado.

## Implementation Details
Ver TechSpec seções "System Architecture" (SimulationSocket), "Integration Points" e "Development
Sequencing" (passo 6). A URL segue o padrão de `VITE_API_URL` já usado em hooks (`use-vessels.ts`).
O transporte (WebSocket) é mockável no teste; nenhum motor entra aqui.

### Relevant Files
- `apps/web/src/features/scenario-creator/hooks/use-vessels.ts` — padrão de env `VITE_*` e fetch.
- `apps/web/vite.config.ts` — config de env/worker (headers COOP/COEP a revisar na task_08).
- `packages/types/src/protocol.ts` — contratos de mensagem (task_02).
- `apps/web/src/stores/simulationStore.ts` — consumidor futuro (task_07).

### Dependent Files
- `apps/web/src/stores/simulationStore.ts` (task_07) — passa a usar o `SimulationSocket`.

### Related ADRs
- [ADR-004: Transporte WebSocket + JSON](../adrs/adr-004.md).
- [ADR-006: reconexão via `sim_id`/`localStorage`](../adrs/adr-006.md).

## Deliverables
- `simulation-socket.ts` com conexão, reconexão, (de)serialização e persistência de `sim_id`.
- Unit tests com 80%+ de cobertura **(REQUIRED)**
- Integration tests da máquina de reconexão/attach **(REQUIRED)**

## Tests
- Unit tests:
  - [ ] Ao receber `SimulationCreated{sim_id}`, grava o `sim_id` no `localStorage`.
  - [ ] Na inicialização com `sim_id` salvo, envia `AttachSimulation{sim_id}`.
  - [ ] `ServerMessage{State}` recebido dispara o callback de estado com o payload correto.
  - [ ] Envio de comando serializa um `ClientMessage` válido.
  - [ ] `Error` de attach inexistente limpa o `localStorage`.
- Integration tests:
  - [ ] Queda de conexão dispara reconexão e reanexa com o `sim_id` salvo (WebSocket de teste).
- Test coverage target: >=80%
- All tests must pass

## Success Criteria
- All tests passing
- Test coverage >=80%
- Reconexão automática funciona; `sim_id` persiste entre reaberturas.
- Nenhuma dependência de WASM/física no cliente.
</content>
