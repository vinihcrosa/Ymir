# Ymir

**Vision:** Plataforma de simulação física naval 6-DOF que roda no navegador — motor C++ compilado para WebAssembly, frontend React interativo, backend Node.js para persistência e streaming de estado.

**Para:** Engenheiros navais e simuladores que precisam de uma ferramenta de simulação acessível via browser, sem instalação de software.

**Resolve:** O motor C++ atual é poderoso mas inacessível — requer compilação nativa, Docker, e acesso via linha de comando. A migração para WASM + UI web elimina essa barreira e abre o motor para uso interativo.

---

## Goals

- Motor C++ compilado para WASM roda no navegador com performance adequada para simulação em tempo real (≥ 10Hz de update de estado)
- API Node.js serve dados de cenários, vessels, e streaming de estado via WebSocket
- Frontend React permite carregar cenários, visualizar estado de simulação, e controlar parâmetros em tempo real
- Build unificado: um comando (`pnpm build`) compila core C++ → WASM + API + Web

---

## Tech Stack

**Core (C++ → WASM):**

- Linguagem: C++17
- Compilador WASM: Emscripten (latest stable)
- Build: CMake 3.16+ com toolchain Emscripten
- Integrador ODE: Dormand-Prince RK45 (substitui SUNDIALS/CVODE)
- Bindings: Emscripten Embind com geração de `.d.ts`

**API:**

- Runtime: Node.js 20 LTS
- Framework: Fastify v4
- Schema/Validação: TypeBox
- Database: SQLite via Drizzle ORM + `better-sqlite3`
- WebSocket: `@fastify/websocket`
- Linguagem: TypeScript 5

**Frontend:**

- Framework: React 18
- Build: Vite 5
- Linguagem: TypeScript 5
- Estado: Zustand
- WASM: carregado em Web Worker via `fetch()`

**Monorepo:**

- Workspaces: pnpm
- Pipeline: Turborepo
- Shared types: `packages/types` com TypeBox schemas

---

## Scope

**v1 inclui:**

- Migração do C++ core para `core/` com target WASM via Emscripten
- Substituição do SUNDIALS/CVODE por integrador RK45 Dormand-Prince puro C++
- Skeleton do API Fastify com rotas de vessels e cenários (SQLite)
- Skeleton do frontend React com carregamento do WASM em Web Worker
- `packages/types` com DTOs compartilhados (TypeBox)
- Pipeline Turborepo: `core#build:wasm` → `web#build` + `api#build`
- Todos os testes existentes passando no target nativo

**Explicitamente fora de scope v1:**

- Visualização 3D (Three.js / WebGPU) — fase separada
- Autenticação e multi-usuário
- Fast-time batch runner
- Collision detection (Phase 4 do roadmap original)
- Mooring e anchoring
