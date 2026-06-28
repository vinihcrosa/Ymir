# Ymir — Roadmap

## Milestones

| Milestone | Título | Depende de | Status |
|-----------|--------|------------|--------|
| **M0** | Monorepo + WASM core | — | Planejado |
| **M1** | API + Persistência | M0 | Bloqueado |
| **M2** | Frontend interativo | M0, M1 | Bloqueado |
| **M3** | Simulação avançada | M2 | Bloqueado |

---

## M0: Monorepo + WASM Core

**Objetivo:** Estrutura monorepo funcional com C++ core compilando para WASM e rodando no navegador.

### Features

- **[F-M0-01] Monorepo setup** — pnpm workspaces + Turborepo, estrutura `core/`, `apps/`, `packages/`
- **[F-M0-02] Integrador RK45** — substituir CVODE por Dormand-Prince RK45 puro C++, mesma interface `IIntegrator`
- **[F-M0-03] Emscripten build target** — CMake toolchain Emscripten, Embind, saída `.wasm` + `.js` + `.d.ts`
- **[F-M0-04] WASM em Web Worker** — carregamento do módulo WASM em Worker, comunicação via `postMessage`
- **[F-M0-05] packages/types** — TypeBox schemas para DTOs compartilhados
- **[F-M0-06] Pipeline Turborepo** — `core#build:wasm` → copia artefato → `web#build`

**Done when:** `pnpm build` compila tudo; WASM roda smoke test de física no browser; testes nativos passam.

---

## M1: API + Persistência

**Objetivo:** API Fastify com CRUD de vessels/cenários e streaming de estado de simulação.

### Features

- **[F-M1-01] Fastify skeleton** — setup TypeScript, TypeBox, Drizzle ORM, SQLite
- **[F-M1-02] Rotas vessels** — CRUD `/vessels` com schema TypeBox
- **[F-M1-03] Rotas cenários** — CRUD `/scenarios` com schema TypeBox
- **[F-M1-04] WebSocket streaming** — endpoint `/ws/simulation` para streaming de `SimulationState` a 10-60Hz
- **[F-M1-05] Migração JSON → SQLite** — ScenarioReader atualizado para ler do banco

**Done when:** API serve vessels e cenários; WebSocket transmite estado de simulação sintético.

---

## M2: Frontend Interativo

**Objetivo:** Interface React para carregar cenário, iniciar simulação, visualizar estado em tempo real.

### Features

- **[F-M2-01] Layout base** — sidebar + viewport, Zustand store
- **[F-M2-02] Seletor de cenário** — lista e carrega cenários da API
- **[F-M2-03] Painel de controle** — start/stop/reset simulação, parâmetros básicos
- **[F-M2-04] Visualização 2D** — top-down view de vessels com estado (posição, heading, velocidade)
- **[F-M2-05] Telemetria** — painel com valores numéricos de estado em tempo real

**Done when:** Usuário carrega cenário, inicia simulação, vê vessels se movendo em tempo real.

---

## M3: Simulação Avançada

**Objetivo:** Features de simulação naval avançadas.

### Features (planejadas)

- Detecção de colisão (broadphase AABB + narrowphase GJK)
- Condições de onda dinâmicas (JONSWAP parametrizado via UI)
- Manobra de atracação assistida por rebocadores
- Fast-time batch runner (web worker dedicado)
- Export de telemetria (CSV/JSON)

---

## Decisões Arquiteturais Registradas

| ID | Decisão | Data |
|----|---------|------|
| D-MONO-01 | pnpm + Turborepo sobre NX — NX não suporta C++/CMake nativamente | 2026-06-27 |
| D-MONO-02 | Dormand-Prince RK45 sobre cross-compilar SUNDIALS — elimina dependência de sistema, naval dynamics não é stiff o suficiente para exigir BDF | 2026-06-27 |
| D-MONO-03 | Embind sobre C API manual — AGENTS.md já exige C API boundary; Embind gera `.d.ts` automaticamente | 2026-06-27 |
| D-MONO-04 | Fastify + TypeBox sobre Express + Zod — TypeBox integra diretamente no sistema de schema do Fastify, um source of truth | 2026-06-27 |
| D-MONO-05 | Web Worker para WASM — simulation loop não pode bloquear main thread | 2026-06-27 |
| D-MONO-06 | SQLite + Drizzle sobre Postgres — volume baixo, read-heavy, sem necessidade de servidor externo; alinha com planos do AGENTS.md | 2026-06-27 |
| D-MONO-07 | Zustand sobre Redux — overhead de Redux não justificado para state de simulação técnica | 2026-06-27 |
