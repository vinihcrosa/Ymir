---
status: completed
title: '`WsGateway` + `MessageRouter` + rota `/ws`'
type: backend
complexity: high
dependencies:
  - task_02
  - task_04
---

# Task 5: `WsGateway` + `MessageRouter` + rota `/ws`

## Overview
Expor o canal de tempo real do servidor: registrar `@fastify/websocket` no `apps/api`, criar a rota
`GET /ws`, validar `ClientMessage` (TypeBox) no `MessageRouter` e despachar para o
`SimulationManager` (task_04); serializar `ServerMessage` de volta ao cliente. Fecha o caminho
servidor de ponta a ponta.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST registrar `@fastify/websocket` na versão compatível com Fastify 4 (linha v8.x — NÃO v10, que exige Fastify 5).
- MUST criar a rota `GET /ws` que faz upgrade e delega ao `MessageRouter`.
- MUST validar todo `ClientMessage` recebido com TypeBox; mensagem inválida → `ServerMessage{Error}` (não derrubar a conexão).
- MUST despachar por `type` para o `SimulationManager` e associar a conexão ao `sim_id` em Create/Attach.
- MUST manter o REST existente inalterado.
- MUST emitir logs estruturados com `sim_id` (sim_created/attached/detached/stopped, command_invalid, ws_error).
</requirements>

## Subtasks
- [x] 5.1 Adicionar `@fastify/websocket` (v8.x) e registrar o plugin no bootstrap.
- [x] 5.2 Criar a rota `GET /ws` e o handler de conexão.
- [x] 5.3 Implementar o `MessageRouter` (validação TypeBox + despacho por `type`).
- [x] 5.4 Ligar Create/Attach à associação conexão↔`sim_id` no `SimulationManager`.
- [x] 5.5 Serializar `ServerMessage` de volta e tratar desconexão (detach).
- [x] 5.6 Escrever testes de roundtrip com cliente WebSocket real e motor real.

## Implementation Details
Ver TechSpec seções "System Architecture" (WsGateway/MessageRouter), "API Endpoints" (`GET /ws`),
"Integration Points" (`@fastify/websocket` v8.x) e "Development Sequencing" (passo 5). Seguir o
padrão de registro de plugins/rotas de `apps/api/src/index.ts`.

### Relevant Files
- `apps/api/src/index.ts` — registro de plugins e rotas (adicionar `/ws`).
- `apps/api/src/routes/*.ts` — padrão de rota Fastify existente.
- `apps/api/src/simulation/simulation-manager.ts` — alvo do despacho (task_04).
- `packages/types/src/protocol.ts` — contratos validados (task_02).

### Dependent Files
- `apps/web/src/lib/simulation-socket.ts` (task_06) — cliente que fala com `/ws`.

### Related ADRs
- [ADR-004: Transporte WebSocket + JSON](../adrs/adr-004.md) — transporte, contrato e versão do plugin.
- [ADR-006: Ciclo de vida](../adrs/adr-006.md) — Create/Attach/detach na conexão.

## Deliverables
- Plugin `@fastify/websocket` registrado e rota `GET /ws` funcional.
- `MessageRouter` com validação e despacho por `type`.
- Unit tests com 80%+ de cobertura **(REQUIRED)**
- Integration tests de roundtrip comando→evento com motor real **(REQUIRED)**

## Tests
- Unit tests:
  - [ ] `MessageRouter` roteia `CreateSimulation` → `manager.create` e responde `SimulationCreated{sim_id}`.
  - [ ] `ClientMessage` inválido (`type` desconhecido / campo faltando) → `ServerMessage{Error}`, conexão permanece aberta.
  - [ ] `AttachSimulation` de `sim_id` inexistente → `Error` claro.
  - [ ] Desconexão do WebSocket chama `manager.detach` para o `sim_id` associado.
- Integration tests:
  - [ ] Cliente WS real: `CreateSimulation`→`Play`→`SetActuator` e recepção de `State` refletindo a física real em < ~100 ms de roundtrip local (critério da PRD).
  - [ ] Duas conexões no mesmo `sim_id` recebem os mesmos snapshots.
  - [ ] REST existente (`GET /vessels`) continua respondendo com o WebSocket registrado.
- Test coverage target: >=80%
- All tests must pass

## Success Criteria
- All tests passing
- Test coverage >=80%
- Roundtrip comando→evento < ~100 ms local.
- REST inalterado; WebSocket e REST convivem no mesmo `apps/api`.
</content>
