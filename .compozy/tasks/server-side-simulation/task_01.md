---
status: completed
title: Pacote `@ymir/wasm` + build Emscripten alvo Node
type: infra
complexity: high
dependencies: []
---

# Task 1: Pacote `@ymir/wasm` + build Emscripten alvo Node

## Overview
Criar o pacote interno `packages/wasm` (`@ymir/wasm`, privado, `workspace:*`) que empacota o motor
de física WASM e expõe um loader Node + a interface `SimulationEngine`. O build Emscripten passa a
mirar `ENVIRONMENT=node` com `MODULARIZE`, produzindo binários dentro do pacote (gerados, não
commitados). Este é o pré-requisito de toda a simulação no servidor.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- MUST criar `packages/wasm` como workspace package privado (`"private": true`), nome `@ymir/wasm`, ESM.
- MUST ajustar o build (Emscripten/CMake) para `-sENVIRONMENT=node -sMODULARIZE=1`, gerando `ymir.js`/`ymir.wasm` dentro do pacote; binários NÃO commitados.
- MUST expor `createSimulationEngine(): Promise<SimulationEngine>` e a interface `SimulationEngine` conforme a seção "Core Interfaces" do TechSpec (espelha `YmirBindings.cpp`).
- MUST validar compatibilidade Node do Embind (ex.: `emscripten::val::global("JSON")` em `getState()`).
- MUST integrar o build ao turbo (`build:wasm` produz em `packages/wasm`).
- Testes MUST usar o motor WASM real (sem mock de engine — ADR-005).
</requirements>

## Subtasks
- [x] 1.1 Criar a estrutura do pacote `packages/wasm` (package.json privado, tsconfig, exports).
- [x] 1.2 Adaptar `core/build-wasm.sh`/`core/CMakeLists.wasm.txt` para alvo Node e saída no pacote.
- [x] 1.3 Implementar o loader (`createSimulationEngine`) e declarar a interface `SimulationEngine`.
- [x] 1.4 Ajustar orquestração turbo/scripts para gerar os binários no pacote. (script `build:wasm` do pacote; fiação turbo/CI completada na task_09)
- [x] 1.5 Escrever testes que carregam o WASM real e exercitam a superfície da interface.

## Implementation Details
Ver TechSpec seções "Core Interfaces" (interface `SimulationEngine` e `createSimulationEngine`),
"System Architecture" e "Development Sequencing" (passo 1). O loader esconde detalhes de
Embind/Emscripten; a interface deve espelhar exatamente a fachada de `YmirBindings.cpp`
(addVesselAt, setRudderAngle, setThrusterCommand, step, getState, loadEnvironment, setWaveConditions,
reset, delete).

### Relevant Files
- `core/src/wasm/YmirBindings.cpp` — fachada `YmirSimulation` a espelhar na interface.
- `core/build-wasm.sh` — script de build atual (alvo browser, saída em `apps/web/public/wasm`).
- `core/CMakeLists.wasm.txt` — configuração Emscripten a ajustar para Node.
- `packages/types/package.json` — modelo de workspace package interno privado.
- `turbo.json` / root `package.json` — orquestração de `build:wasm`.

### Dependent Files
- `apps/api/package.json` — passará a depender de `@ymir/wasm` (task_03/04/05).
- `.github/workflows/ci.yml` — build do WASM no CI (task_09).

### Related ADRs
- [ADR-003: Módulo WASM como pacote interno `@ymir/wasm` com alvo de build Node](../adrs/adr-003.md) — define o pacote e o alvo Node.
- [ADR-001: Motor no servidor via WASM em Node](../adrs/adr-001.md) — reuso do mesmo motor.
- [ADR-005: Testes contra o motor real](../adrs/adr-005.md) — sem mock; WASM real nos testes.

## Deliverables
- Pacote `@ymir/wasm` com loader Node e interface `SimulationEngine` exportada.
- Build Emscripten alvo Node integrado ao turbo, produzindo binários no pacote.
- Unit tests com 80%+ de cobertura **(REQUIRED)**
- Integration tests exercitando o motor WASM real via a interface **(REQUIRED)**

## Tests
- Unit tests:
  - [ ] `createSimulationEngine()` resolve e retorna um objeto com todos os métodos da interface.
  - [ ] `addVesselAt(id, 0, 0, 0)` seguido de `getState()` retorna um `SimulationStateDTO` com a embarcação no ponto inicial e velocidades zero.
  - [ ] `step(dt)` com thruster a 0% mantém a embarcação praticamente parada (limite analítico de repouso).
  - [ ] `setThrusterCommand(id,0,100,0)` + N `step()` produz avanço em surge (u > 0) na direção do heading.
  - [ ] `delete()` libera a instância sem lançar (heap Embind).
- Integration tests:
  - [ ] Carregar o WASM real em Node (não browser) e rodar um cenário curto de ponta a ponta sem erro.
- Test coverage target: >=80%
- All tests must pass

## Success Criteria
- All tests passing
- Test coverage >=80%
- `@ymir/wasm` compila com alvo Node e é importável por um processo Node.
- A interface `SimulationEngine` cobre 100% da superfície de `YmirBindings.cpp`.
</content>
