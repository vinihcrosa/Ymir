---
status: pending
title: '`SimulationManager` — ciclo de vida, `sim_id`, TTL, limite'
type: backend
complexity: high
dependencies:
  - task_03
---

# Task 4: `SimulationManager` — ciclo de vida, `sim_id`, TTL, limite

## Overview
Implementar o `SimulationManager` no thread principal do Fastify: cria/encerra `worker_thread`s
(task_03), gera `sim_id`, mantém `sim_id → { worker, conexões, timer TTL }`, aplica o limite de sims
simultâneas e a política de TTL de órfãs (default 30 min). É o dono do ciclo de vida das simulações.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST expor a interface `SimulationManager` (create/attach/detach/command/stop) da seção "Core Interfaces" do TechSpec.
- MUST gerar `sim_id` não adivinhável e devolvê-lo na criação.
- MUST armar TTL configurável (default 30 min) quando a última conexão de uma sim é removida, e cancelá-lo atomicamente em `attach` (ADR-006).
- MUST encerrar o worker_thread ao expirar o TTL e em `stop`/`Reset` explícito (término idempotente).
- MUST recusar criação acima do limite configurável de sims simultâneas.
- MUST reencaminhar snapshots/status do worker para todas as conexões daquela sim.
</requirements>

## Subtasks
- [ ] 4.1 Implementar o registro `sim_id → { worker, conexões, timer }` e a geração de `sim_id`.
- [ ] 4.2 Implementar create/stop (spawn/término de worker_thread) com término idempotente.
- [ ] 4.3 Implementar attach/detach com arme/cancelamento de TTL.
- [ ] 4.4 Aplicar limite de sims simultâneas.
- [ ] 4.5 Reencaminhar mensagens do worker → conexões (fan-out por `sim_id`).
- [ ] 4.6 Escrever testes de ciclo de vida com timers controlados e worker real.

## Implementation Details
Ver TechSpec seções "Core Interfaces" (interface `SimulationManager`), "System Architecture" e
"Development Sequencing" (passo 4). O manager não roda física — só orquestra workers (task_03) e
conexões. TTL configurável por env/config.

### Relevant Files
- `apps/api/src/simulation/simulation-worker.ts` — worker gerenciado (task_03).
- `packages/types/src/protocol.ts` — tipos de comando/evento reencaminhados (task_02).
- `apps/api/src/index.ts` — onde o manager será instanciado no bootstrap.

### Dependent Files
- `apps/api/src/simulation/ws-gateway.ts` / `message-router.ts` (task_05) — chamam o manager.

### Related ADRs
- [ADR-002: worker_thread por simulação](../adrs/adr-002.md) — o que o manager cria/encerra.
- [ADR-006: Ciclo de vida — `sim_id`, reconexão, TTL](../adrs/adr-006.md) — regras de attach/detach/TTL.

## Deliverables
- `SimulationManager` com create/attach/detach/command/stop e política de TTL/limite.
- Unit tests com 80%+ de cobertura **(REQUIRED)**
- Integration tests de ciclo de vida com worker real **(REQUIRED)**

## Tests
- Unit tests:
  - [ ] `create()` retorna `sim_id` único e registra o worker; `create()` acima do limite é recusado.
  - [ ] `attach(simId, conn)` de `sim_id` inexistente retorna `false`.
  - [ ] `detach` da última conexão arma o timer de TTL; `attach` dentro do TTL cancela o timer.
  - [ ] TTL expirado encerra o worker e remove o registro (timer fake/curto no teste).
  - [ ] `stop()` encerra imediatamente e é idempotente (segunda chamada não lança).
  - [ ] Snapshot emitido pelo worker é reencaminhado a todas as conexões registradas do `sim_id`.
- Integration tests:
  - [ ] Criar sim real, anexar 1 conexão, receber `State`; desanexar, aguardar TTL curto, confirmar término do worker.
- Test coverage target: >=80%
- All tests must pass

## Success Criteria
- All tests passing
- Test coverage >=80%
- Nenhuma sim órfã sobrevive além do TTL; nenhum worker vaza em stop/reset.
- Reconexão dentro do TTL retoma a sim sem recriar.
</content>
