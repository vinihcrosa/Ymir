---
status: completed
title: Remoção do motor no cliente (worker, mock, public/wasm, tipos)
type: refactor
complexity: medium
dependencies:
  - task_07
---

# Task 8: Remoção do motor no cliente (worker, mock, public/wasm, tipos)

## Overview
Concluir o corte para motor único: remover o Web Worker de simulação, o mock cinemático, os assets
WASM do cliente, os tipos aposentados e os headers COOP/COEP se não forem mais necessários. Só
executar após a paridade validada pela task_07.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST remover `apps/web/src/workers/simulation.worker.ts` e `apps/web/src/workers/mock-wasm.ts` (+ `mock-wasm.test.ts`).
- MUST remover `apps/web/public/wasm/` (o cliente não carrega mais WASM).
- MUST remover de `packages/types` o `WorkerMessageDTO` e a union `SimulationEngine = 'wasm'|'mock'`, ajustando todos os importadores.
- MUST remover os headers COOP/COEP de `vite.config.ts` e a config `worker.format` se não forem usados por outra funcionalidade.
- MUST garantir que o build do web e a suíte de testes passam sem nenhuma referência remanescente ao motor local.
- MUST NOT alterar comportamento observável do usuário (feito na task_07).
</requirements>

## Subtasks
- [x] 8.1 Deletar worker, mock e seus testes.
- [x] 8.2 Remover `apps/web/public/wasm/` e referências de carregamento.
- [x] 8.3 Remover tipos aposentados de `packages/types` e corrigir importadores.
- [x] 8.4 Limpar `vite.config.ts` (COOP/COEP, worker.format) se aplicável.
- [x] 8.5 Rodar build + testes do web e confirmar ausência de referências ao motor local.

## Implementation Details
Ver TechSpec "Impact Analysis" (linhas marcadas `deprecated`) e "Development Sequencing" (passo 8).
Verificar todos os importadores de `WorkerMessageDTO`/union `mock` antes de remover (grep no
monorepo). Confirmar se COOP/COEP era usado só para SharedArrayBuffer do WASM.

### Relevant Files
- `apps/web/src/workers/simulation.worker.ts` — remover.
- `apps/web/src/workers/mock-wasm.ts` / `mock-wasm.test.ts` — remover.
- `apps/web/public/wasm/` — remover.
- `packages/types/src/simulation.ts` — remover `WorkerMessageDTO` e union `mock`.
- `apps/web/vite.config.ts` — limpar headers COOP/COEP e `worker.format` se sobrarem sem uso.

### Dependent Files
- Qualquer importador de `WorkerMessageDTO`/`SimulationEngine` — ajustar/remover import.
- `apps/web/src/stores/simulationStore.ts` — já religado (task_07); confirmar sem resíduo de worker.

### Related ADRs
- [ADR-001: cliente como terminal remoto](../adrs/adr-001.md).
- [ADR-005: sem mock de engine](../adrs/adr-005.md) — motiva a remoção do mock.

## Deliverables
- Cliente sem worker, mock, assets WASM ou tipos aposentados.
- `vite.config.ts` limpo (se COOP/COEP não mais necessário).
- Unit tests com 80%+ de cobertura **(REQUIRED)**
- Integration/E2E confirmando o fluxo do usuário sem o motor local **(REQUIRED)**

## Tests
- Unit tests:
  - [ ] Build/typecheck do web passa sem referências a `WorkerMessageDTO`/union `mock`.
  - [ ] Nenhum import remanescente de `simulation.worker`/`mock-wasm` (verificação por busca no CI/teste).
- Integration tests:
  - [ ] E2E existente (`simulation-run.spec.ts`) passa contra o backend de servidor (sem motor local).
  - [ ] App inicia sem tentar carregar `/wasm/ymir.js`.
- Test coverage target: >=80%
- All tests must pass

## Success Criteria
- All tests passing
- Test coverage >=80%
- Zero referências ao motor local no cliente; um único motor no repositório.
- Comportamento do usuário idêntico ao pós-task_07.
</content>
