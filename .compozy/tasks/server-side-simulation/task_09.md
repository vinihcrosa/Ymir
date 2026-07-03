---
status: completed
title: 'CI: build WASM (emsdk) + cache antes dos testes'
type: infra
complexity: medium
dependencies:
  - task_01
---

# Task 9: CI: build WASM (emsdk) + cache antes dos testes

## Overview
Estender o pipeline de CI para compilar o `@ymir/wasm` (Emscripten) antes de rodar os testes que
dependem do motor real, com cache de artefatos. Necessário porque a estratégia de testes proíbe mock
de engine (ADR-005): os testes rodam contra o WASM real, que precisa existir no ambiente de CI.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST disponibilizar a toolchain Emscripten (emsdk) no CI.
- MUST rodar `turbo run build:wasm` (produzindo em `packages/wasm`) antes dos testes que usam o motor.
- MUST cachear a toolchain e/ou os artefatos de build para conter o tempo de pipeline.
- MUST manter o CI verde de ponta a ponta (build WASM → testes de todos os workspaces).
- SHOULD ordenar os estágios para que falha de build do WASM falhe cedo, com log claro.
</requirements>

## Subtasks
- [x] 9.1 Adicionar setup do emsdk no workflow de CI. (`mymindstorm/setup-emsdk@v14`, versão pinada via `EMSDK_VERSION`)
- [x] 9.2 Inserir o estágio `build:wasm` antes dos testes dependentes. (`pnpm build:wasm` antes de `pnpm test`)
- [x] 9.3 Configurar cache (emsdk e/ou artefatos turbo). (cache do emsdk + cache do turbo)
- [x] 9.4 Ajustar a ordem/deps dos jobs para falhar cedo em erro de WASM. (build:wasm explícito antes; `test` dependsOn `build:wasm`/`^build:wasm` no turbo)
- [x] 9.5 Validar o pipeline completo verde. (turbo ordering validado localmente; execução do workflow só no GitHub Actions)

## Implementation Details
Ver TechSpec "Testing Approach" e "Development Sequencing" (passo 9), e ADR-005. Estender
`.github/workflows/ci.yml`. O build do WASM foi definido na task_01 (turbo `build:wasm` → pacote).

### Relevant Files
- `.github/workflows/ci.yml` — pipeline a estender.
- `core/build-wasm.sh` / `core/CMakeLists.wasm.txt` — build invocado (ajustado na task_01).
- `turbo.json` / root `package.json` — target `build:wasm` e dependências de build.
- `packages/wasm` — destino dos artefatos (task_01).

### Dependent Files
- Jobs de teste de `apps/api` e `packages/wasm` — passam a exigir o WASM compilado.

### Related ADRs
- [ADR-005: Testes contra o motor real; WASM compilado no CI](../adrs/adr-005.md).
- [ADR-003: `@ymir/wasm` e alvo de build Node](../adrs/adr-003.md).

## Deliverables
- CI com setup emsdk, estágio `build:wasm` e cache.
- Pipeline verde de ponta a ponta.
- Unit tests com 80%+ de cobertura **(REQUIRED)** — aplicável aos workspaces; o job de CI valida a suíte.
- Integration tests: execução dos testes dependentes de WASM no CI **(REQUIRED)**

## Tests
- Unit tests:
  - [ ] (Validação de pipeline) O job de CI executa `build:wasm` com sucesso e gera os artefatos no pacote.
- Integration tests:
  - [ ] Testes de `packages/wasm` e `apps/api` (que usam o motor real) rodam e passam no CI com o WASM compilado.
  - [ ] Falha proposital de build do WASM faz o pipeline falhar cedo com log claro (verificação manual/documentada).
- Test coverage target: >=80%
- All tests must pass

## Success Criteria
- All tests passing
- Test coverage >=80%
- CI compila o WASM e roda os testes reais de forma verde e com tempo aceitável (cache ativo).
- Nenhum teste depende de mock de engine.
</content>
