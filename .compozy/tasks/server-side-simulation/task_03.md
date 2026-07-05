---
status: completed
title: '`SimulationWorker` — loop 20 Hz em worker_thread'
type: backend
complexity: high
dependencies:
  - task_01
  - task_02
---

# Task 3: `SimulationWorker` — loop 20 Hz em worker_thread

## Overview
Implementar o `SimulationWorker`, executado em um `worker_thread` do Node, que carrega o motor real
(`@ymir/wasm`), roda o loop de tick a 20 Hz e aplica os comandos recebidos. Publica snapshots
(`State`) e status via `postMessage` ao thread principal e recebe comandos por `postMessage`.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST rodar em `worker_thread` (ADR-002), carregando `createSimulationEngine()` de `@ymir/wasm`.
- MUST executar o loop a 20 Hz (dt padrão 0.05 s) e emitir `State{SimulationStateDTO}` por tick.
- MUST aplicar comandos ao motor: LoadScenario, Play, Pause, Reset, SetActuator (rudder/thruster), LoadEnvironment — paridade com o worker atual.
- MUST reconstruir o motor do zero em LoadScenario (addVessel após step é comportamento indefinido — ver comentário no worker atual).
- MUST obter a config de embarcação via chamada direta ao `vessel-service` (não HTTP).
- MUST usar os DTOs de protocolo (task_02) nas mensagens de/para o thread principal.
- Testes MUST usar o motor WASM real (ADR-005).
</requirements>

## Subtasks
- [x] 3.1 Criar o entrypoint do worker_thread e o carregamento do motor real. (`simulation-worker.entry.ts`)
- [x] 3.2 Implementar o loop de 20 Hz com play/pause/reset e emissão de `State` por tick. (`SimulationRunner`)
- [x] 3.3 Mapear comandos → ações no motor (leme, thruster, ambiente, cenário). (bridge `attachRunnerToPort`)
- [x] 3.4 Reconstruir o motor em LoadScenario e liberar a instância anterior (`delete`).
- [x] 3.5 Integrar a leitura de config de embarcação via `vessel-service`. (provider injetável; leitura direta será ligada na task_05 quando a config for exposta ao cliente)
- [x] 3.6 Escrever testes exercitando o worker real (thread + motor WASM). (runner com motor real + ponte via MessageChannel)

## Implementation Details
Ver TechSpec seções "System Architecture" (SimulationWorker) e "Development Sequencing" (passo 3).
A lógica de comando espelha `apps/web/src/workers/simulation.worker.ts` (loop 50 ms, setActuator,
loadScenario reconstruindo a simulação, loadEnvironment + setWaveConditions), mas roda no servidor e
lê a config direto do service em vez de `fetch`.

### Relevant Files
- `apps/web/src/workers/simulation.worker.ts` — referência de comportamento (loop, comandos, reconstrução em loadScenario).
- `apps/api/src/services/vessel-service.ts` — fonte da config de embarcação (chamada direta).
- `packages/wasm` (`SimulationEngine`, `createSimulationEngine`) — motor consumido (task_01).
- `packages/types/src/protocol.ts` — DTOs de comando/estado (task_02).

### Dependent Files
- `apps/api/src/simulation/simulation-manager.ts` (task_04) — cria e fala com o worker.

### Related ADRs
- [ADR-002: Uma simulação por `worker_thread` do Node](../adrs/adr-002.md) — modelo de execução.
- [ADR-003: `@ymir/wasm`](../adrs/adr-003.md) — origem do motor.
- [ADR-005: Testes contra o motor real](../adrs/adr-005.md).

## Deliverables
- `SimulationWorker` (entrypoint do worker_thread) com loop 20 Hz e mapeamento de comandos.
- Leitura de config de embarcação via `vessel-service`.
- Unit tests com 80%+ de cobertura **(REQUIRED)**
- Integration tests do worker real (thread + WASM) **(REQUIRED)**

## Tests
- Unit tests:
  - [ ] Comando `SetActuator(thruster,100%)` faz `u` crescer após ticks (surge positivo).
  - [ ] Comando `SetActuator(rudder,+35°)` com avanço produz taxa de guinada `r` não nula.
  - [ ] `Pause` congela o estado (mesmo `t` e posição entre leituras); `Play` retoma.
  - [ ] `Reset` volta tempo e estado ao inicial.
  - [ ] `LoadScenario` com 2 embarcações resulta em `getState().vessels.length === 2` nas posições dadas.
  - [ ] `LoadEnvironment` com corrente altera a deriva da embarcação sem thrust.
- Integration tests:
  - [ ] Worker_thread real: enviar sequência Create→Play→SetActuator e receber `State` coerentes por ~1 s simulado.
- Test coverage target: >=80%
- All tests must pass

## Success Criteria
- All tests passing
- Test coverage >=80%
- Worker roda o motor real em thread separada sem bloquear o chamador.
- Paridade de comandos com o worker atual do cliente.
</content>
