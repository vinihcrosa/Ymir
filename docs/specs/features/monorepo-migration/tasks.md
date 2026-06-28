# Monorepo Migration — Tasks

**Design:** `.specs/features/monorepo-migration/design.md`
**Status:** Draft

---

## Execution Plan

### Phase 1 — C++ Core Refactor (Sequential)

Refatorar o motor C++ antes de mover qualquer arquivo. Testes devem passar a cada passo.

```
T01 → T02 → T03 → T04 → T05
```

### Phase 2 — Estrutura Monorepo + Build WASM (Paralelo após T05)

```
T05 ──┬──→ T06 [P]
      └──→ T07 [P]
```

### Phase 3 — WASM Bindings + Shared Types (Paralelo após T06+T07)

```
T06 + T07 ──┬──→ T08
             └──→ T09 [P com T08]
```

### Phase 4 — API + Web Skeletons (Paralelo após T09)

```
T09 ──┬──→ T10 [P]
      └──→ T14 [P]
```

### Phase 5 — Implementação API + Web Worker (Paralelo após T10/T14)

```
T10 ──→ T11 ──┬──→ T12 [P]
               └──→ T13 [P]

T14 + T08 ──┬──→ T15 [P]
             └──→ T16 [P]
```

### Phase 6 — Frontend Component (após T15 + T16)

```
T15 + T16 ──→ T17
```

### Phase 7 — Integração E2E (Sequential, após tudo)

```
T12 + T13 + T17 ──→ T18 ──→ T19
```

---

## Task Breakdown

---

### T01: Extrair `IIntegrator` como interface pura

**What:** Separar a interface `IIntegrator` do arquivo `CvodeIntegrator.h` para um header próprio `IIntegrator.h`, sem alterar nenhuma implementação.
**Where:** `libs/physics/include/ymir/physics/integrator/IIntegrator.h` (novo)
**Depends on:** Nenhuma
**Reuses:** Interface já declarada em `libs/physics/include/ymir/physics/integrator/CvodeIntegrator.h`
**Requirement:** MONO-02

**Done when:**

- [ ] `IIntegrator.h` contém apenas a classe abstrata pura (sem includes SUNDIALS)
- [ ] `CvodeIntegrator.h` inclui `IIntegrator.h` e herda dela (sem mudança de comportamento)
- [ ] `cmake --build build --parallel` passa sem erros
- [ ] `ctest --test-dir build --output-on-failure` — mesmo número de testes passando (≥ 35)

**Tests:** build (verificação de compilação)
**Gate:** `cmake --build build --parallel && ctest --test-dir build --output-on-failure`

**Commit:** `refactor(physics): extract IIntegrator pure interface to own header`

---

### T02: Implementar `RK45Integrator` (Dormand-Prince)

**What:** Criar `RK45Integrator` implementando `IIntegrator` com método Dormand-Prince 4(5), step adaptation, e zero heap allocation no hot path.
**Where:**
- `libs/physics/include/ymir/physics/integrator/RK45Integrator.h` (novo)
- `libs/physics/src/integrator/RK45Integrator.cpp` (novo)
- `tests/physics/TestIntegratorRK45.cpp` (novo — testa precisão e adaptação)
**Depends on:** T01
**Reuses:** `IIntegrator` (T01), `Vector6` de `ymir/common/Types.h`
**Requirement:** MONO-02

**Done when:**

- [ ] `RK45Integrator` compila sem warnings
- [ ] Buffers `k1..k7` são membros (não alocados no heap por step)
- [ ] Teste de oscilador harmônico: erro vs solução analítica < `1e-5` com `rtol=1e-6, atol=1e-8`
- [ ] Teste de step adaptation: `dt` reduz automaticamente quando erro excede tolerância
- [ ] `ctest --test-dir build --output-on-failure` — testes de integrador passam (≥ 38 total)

**Tests:** unit (TestIntegratorRK45.cpp)
**Gate:** `cmake --build build --parallel && ctest --test-dir build -R TestIntegratorRK45 --output-on-failure`

**Commit:** `feat(physics): add Dormand-Prince RK45 adaptive integrator`

---

### T03: Substituir CVODE por RK45 e remover SUNDIALS

**What:** Trocar todas as instanciações de `CvodeIntegrator` por `RK45Integrator`, remover `CvodeIntegrator.h/cpp`, e remover dependência SUNDIALS do `cmake/Dependencies.cmake`.
**Where:**
- `libs/physics/src/integrator/CvodeIntegrator.cpp` (deletar)
- `libs/physics/include/ymir/physics/integrator/CvodeIntegrator.h` (deletar)
- `cmake/Dependencies.cmake` (remover bloco SUNDIALS)
- Todos os arquivos que incluem/instanciam `CvodeIntegrator` (buscar com grep)
**Depends on:** T02
**Reuses:** `RK45Integrator` (T02)
**Requirement:** MONO-02

**Done when:**

- [ ] `grep -r "CvodeIntegrator\|SUNDIALS\|cvode" libs/ cmake/` retorna zero resultados
- [ ] `cmake -B build -S . -GNinja -DYMIR_BUILD_TESTS=ON` configura sem erros de dependência ausente
- [ ] `cmake --build build --parallel` compila sem erros
- [ ] `ctest --test-dir build --output-on-failure` — todos os testes passam (≥ 38 total, incluindo TestIntegratorRK45)
- [ ] Build funciona em sistema sem SUNDIALS instalado

**Tests:** integration (suite completa de testes C++)
**Gate:** `cmake -B build-clean -S . -GNinja -DYMIR_BUILD_TESTS=ON && cmake --build build-clean --parallel && ctest --test-dir build-clean --output-on-failure`

**Commit:** `refactor(physics): replace CVODE with RK45, remove SUNDIALS dependency`

---

### T04: Adicionar `NavalSimulation::serializeStateJson()`

**What:** Adicionar método `serializeStateJson() const` em `NavalSimulation` que retorna um JSON string com o estado (`t`, `vessels[]` com posição e velocidade 6-DOF) de todos os bodies da simulação, usando `nlohmann_json`.
**Where:**
- `libs/simulation/include/ymir/simulation/NavalSimulation.h` (adicionar declaração)
- `libs/simulation/src/NavalSimulation.cpp` (implementar)
- `tests/simulation/TestNavalSimulation.cpp` ou arquivo existente (adicionar testes do método)
**Depends on:** T03
**Reuses:** `nlohmann_json` já usado em `persistence/`; `BodyState` de `ymir/physics/`
**Requirement:** MONO-03

**Done when:**

- [ ] `serializeStateJson()` retorna JSON válido para simulação com 1+ vessels
- [ ] JSON contém campos: `t` (double), `vessels` (array com `id`, `x`, `y`, `z`, `phi`, `theta`, `psi`, `u`, `v`, `r`)
- [ ] Método é `const` e não aloca heap extra além do string de retorno
- [ ] Teste verifica que JSON parseia sem exceção e tem os campos esperados
- [ ] `ctest --test-dir build --output-on-failure` — todos passam (≥ 39 total)

**Tests:** unit
**Gate:** `cmake --build build --parallel && ctest --test-dir build --output-on-failure`

**Commit:** `feat(simulation): add serializeStateJson for WASM state export`

---

### T05: Mover C++ para `core/` e atualizar todos os paths

**What:** Mover `libs/`, `tests/`, `cmake/`, `docker/`, `apps/` (stubs), `docs/`, `CMakeLists.txt` e `Makefile` para o subdiretório `core/`. Atualizar todos os CMakeLists.txt com os novos paths relativos. Criar `core/.gitkeep` não é necessário — mover é suficiente.
**Where:** Raiz do repositório (reestruturação física)
**Depends on:** T04
**Reuses:** Todos os arquivos C++ existentes
**Requirement:** MONO-01

**Done when:**

- [ ] `ls` na raiz mostra `core/` como diretório (sem `libs/`, `tests/`, `cmake/` na raiz)
- [ ] `cmake -B core/build -S core -GNinja -DYMIR_BUILD_TESTS=ON` configura sem erros
- [ ] `cmake --build core/build --parallel` compila sem erros
- [ ] `ctest --test-dir core/build --output-on-failure` — todos passam (≥ 39 total)
- [ ] `core/package.json` criado com `name: "@ymir/core"` e script `build:wasm`

**Tests:** integration
**Gate:** `cmake -B core/build -S core -GNinja -DYMIR_BUILD_TESTS=ON && cmake --build core/build --parallel && ctest --test-dir core/build --output-on-failure`

**Commit:** `refactor(build): move C++ core to core/ subdirectory`

---

### T06: Setup monorepo raiz (pnpm + Turborepo) [P]

**What:** Criar os 3 arquivos de configuração do monorepo raiz: `package.json` (workspace root), `pnpm-workspace.yaml`, e `turbo.json` com pipeline correto.
**Where:**
- `package.json` (raiz — novo)
- `pnpm-workspace.yaml` (raiz — novo)
- `turbo.json` (raiz — novo)
**Depends on:** T05
**Reuses:** Estrutura de diretórios criada em T05
**Requirement:** MONO-01

**Done when:**

- [ ] `pnpm-workspace.yaml` lista `apps/*` e `packages/*` (não `core/`)
- [ ] `turbo.json` define pipelines: `core#build:wasm`, `build`, `dev`, `test`, `typecheck` com dependências corretas
- [ ] `pnpm install` na raiz completa sem erro (mesmo que `apps/` e `packages/` ainda não existam)
- [ ] `turbo.json` tem `"core#build:wasm"` com `inputs` apontando para `core/libs/**` e `outputs` para `apps/web/public/wasm/**`

**Tests:** none (configuração)
**Gate:** `pnpm install && pnpm turbo --version`

**Commit:** `chore(monorepo): setup pnpm workspaces and Turborepo pipeline`

---

### T07: Criar Emscripten CMake target e build script [P]

**What:** Criar `core/cmake/Emscripten.cmake` com o target `ymir_wasm` usando Embind, e `core/scripts/build-wasm.sh` que invoca `emcmake cmake` e copia artefatos para `apps/web/public/wasm/`.
**Where:**
- `core/cmake/Emscripten.cmake` (novo)
- `core/scripts/build-wasm.sh` (novo, executável)
- `core/CMakeLists.txt` (adicionar `include(cmake/Emscripten.cmake)`)
**Depends on:** T05
**Reuses:** CMakeLists.txt existente em `core/`
**Requirement:** MONO-01

**Done when:**

- [ ] `core/cmake/Emscripten.cmake` guarda o bloco `if(EMSCRIPTEN)` isolado do build nativo
- [ ] `core/scripts/build-wasm.sh` invoca: `emcmake cmake -B core/build-wasm -S core -DCMAKE_BUILD_TYPE=Release -DYMIR_BUILD_TESTS=OFF && cmake --build core/build-wasm --target ymir_wasm --parallel`
- [ ] Script cria `apps/web/public/wasm/` se não existir e copia `ymir_wasm.wasm` → `ymir.wasm`, `ymir_wasm.js` → `ymir.js`
- [ ] Build nativo (`cmake -B core/build -S core -GNinja`) NÃO inclui targets Emscripten (guards corretos)
- [ ] `bash core/scripts/build-wasm.sh` completa sem erro (requer Emscripten instalado — gate é estrutura, não execução em CI sem Emscripten)

**Tests:** build (verificação de estrutura do cmake)
**Gate:** `cmake -B core/build -S core -GNinja -DYMIR_BUILD_TESTS=ON && cmake --build core/build --parallel` (nativo não quebra)

**Commit:** `build(core): add Emscripten CMake target and build-wasm script`

---

### T08: Criar `YmirBindings.cpp` — Embind surface

**What:** Criar `core/src/wasm/YmirBindings.cpp` com a classe `SimulationHandle` exposta via Embind, wrappando `NavalSimulation` com métodos `loadScenario`, `step`, `getStateJson`, `reset`.
**Where:**
- `core/src/wasm/YmirBindings.cpp` (novo)
- `core/CMakeLists.txt` ou `core/cmake/Emscripten.cmake` (adicionar source ao target)
**Depends on:** T04 (precisa de `serializeStateJson`), T07 (precisa do cmake target)
**Reuses:** `NavalSimulation`, `ScenarioReader` sem modificação
**Requirement:** MONO-03

**Done when:**

- [ ] `YmirBindings.cpp` compila via `cmake --build core/build-wasm --target ymir_wasm` sem erros
- [ ] Binding expõe: `SimulationHandle` com `.loadScenario(string)`, `.step(double)`, `.getStateJson() → string`, `.reset()`
- [ ] `--emit-tsd` gera `ymir.d.ts` com tipos corretos para os 4 métodos
- [ ] `apps/web/public/wasm/ymir.wasm` e `ymir.js` existem após build (ou `ymir.d.ts` no build-wasm dir)

**Tests:** build (verificação de compilação WASM)
**Gate:** `bash core/scripts/build-wasm.sh` (requer Emscripten; documentar fallback se não disponível em CI)

**Commit:** `feat(core): add Embind WASM surface for SimulationHandle`

---

### T09: Criar `packages/types` com TypeBox schemas [P com T08]

**What:** Criar o workspace `packages/types` com schemas TypeBox para `VesselDTO`, `ScenarioDTO`, e `SimulationStateDTO`, com build configurado para emitir `.d.ts`.
**Where:**
- `packages/types/package.json` (novo)
- `packages/types/tsconfig.json` (novo)
- `packages/types/src/vessel.ts` (novo)
- `packages/types/src/scenario.ts` (novo)
- `packages/types/src/simulation.ts` (novo)
- `packages/types/src/index.ts` (novo — re-exporta tudo)
**Depends on:** T06 (workspace raiz precisa existir)
**Reuses:** Campos de `BodyState` (via `serializeStateJson` shape definido em T04)
**Requirement:** MONO-05

**Done when:**

- [ ] `pnpm --filter @ymir/types build` completa e gera `dist/` com `.d.ts`
- [ ] `VesselDTO` tem campos: `id`, `name`, `mass`, `length`, `beam`, `draft`, `Cx`, `Cy`, `Cn`
- [ ] `SimulationStateDTO` tem `t: number` e `vessels[]` com 10 campos de estado 6-DOF
- [ ] `ScenarioDTO` tem `id`, `name`, `description`, `configJson`
- [ ] Importar `@ymir/types` em arquivo TS externo compila sem `any` implícito

**Tests:** build (tsc sem erros)
**Gate:** `pnpm --filter @ymir/types build && pnpm --filter @ymir/types tsc --noEmit`

**Commit:** `feat(types): add TypeBox schemas for VesselDTO, ScenarioDTO, SimulationStateDTO`

---

### T10: Setup `apps/api` — Fastify skeleton [P]

**What:** Criar o workspace `apps/api` com Fastify v4, TypeBox, Drizzle, TypeScript configurado e servidor iniciando em `localhost:3000`.
**Where:**
- `apps/api/package.json` (novo)
- `apps/api/tsconfig.json` (novo)
- `apps/api/src/app.ts` (novo — Fastify instance + register plugins)
- `apps/api/src/server.ts` (novo — entry point: `app.listen()`)
**Depends on:** T09 (`@ymir/types` precisa estar buildado)
**Reuses:** `@ymir/types` para imports de DTOs
**Requirement:** MONO-04

**Done when:**

- [ ] `pnpm --filter @ymir/api dev` inicia servidor sem erro
- [ ] `curl localhost:3000/health` retorna `{ "status": "ok" }` com HTTP 200
- [ ] TypeScript compila sem erros (`tsc --noEmit`)
- [ ] `@fastify/websocket` registrado mas sem rotas WS ainda

**Tests:** unit (Fastify inject no `/health`)
**Gate:** `pnpm --filter @ymir/api build && pnpm --filter @ymir/api test`

**Commit:** `feat(api): add Fastify skeleton with health route`

---

### T11: Drizzle schema + SQLite + seed de vessels

**What:** Criar schema Drizzle para `vessels` e `scenarios`, configurar SQLite, e adicionar seed com pelo menos 2 vessels de exemplo (baseados nos cenários JSON existentes em `core/`).
**Where:**
- `apps/api/src/db/schema.ts` (novo)
- `apps/api/src/db/index.ts` (novo — Drizzle client)
- `apps/api/src/db/seed.ts` (novo — seed data)
- `apps/api/drizzle.config.ts` (novo)
**Depends on:** T10
**Reuses:** Schema shape de `VesselDTO` em `@ymir/types`
**Requirement:** MONO-04

**Done when:**

- [ ] `pnpm --filter @ymir/api db:migrate` aplica migrations sem erro
- [ ] `pnpm --filter @ymir/api db:seed` insere ≥ 2 vessels e ≥ 1 scenario no SQLite
- [ ] Schema `vessels` e `scenarios` batem campo-a-campo com `VesselDTO` e `ScenarioDTO` de `@ymir/types`
- [ ] `apps/api/ymir.db` criado com dados de seed
- [ ] Teste: query `db.select().from(vessels)` retorna ≥ 2 registros

**Tests:** integration (query real ao SQLite)
**Gate:** `pnpm --filter @ymir/api db:migrate && pnpm --filter @ymir/api db:seed && pnpm --filter @ymir/api test`

**Commit:** `feat(api): add Drizzle schema, SQLite setup, and vessel seed data`

---

### T12: Rotas `/vessels` (CRUD parcial) [P]

**What:** Implementar rotas `GET /vessels`, `GET /vessels/:id`, `POST /vessels` em `apps/api/src/routes/vessels.ts` com schema TypeBox e respostas validadas.
**Where:**
- `apps/api/src/routes/vessels.ts` (novo)
- `apps/api/src/routes/vessels.test.ts` (novo — testes via `fastify.inject`)
**Depends on:** T11, T09
**Reuses:** `VesselDTO` de `@ymir/types`, Drizzle client de T11
**Requirement:** MONO-04

**Done when:**

- [ ] `GET /vessels` retorna `200` com array `VesselDTO[]`
- [ ] `GET /vessels/:id` com ID válido retorna `200` com `VesselDTO`
- [ ] `GET /vessels/:id` com ID inválido retorna `404`
- [ ] `POST /vessels` com body inválido retorna `400` com erros TypeBox
- [ ] `POST /vessels` com body válido retorna `201` com vessel criado
- [ ] `pnpm --filter @ymir/api test` — ≥ 5 testes de vessel passam

**Tests:** unit (fastify.inject — sem HTTP real)
**Gate:** `pnpm --filter @ymir/api build && pnpm --filter @ymir/api test`

**Commit:** `feat(api): add vessels CRUD routes with TypeBox validation`

---

### T13: Rotas `/scenarios` (CRUD parcial) [P]

**What:** Implementar rotas `GET /scenarios`, `GET /scenarios/:id`, `POST /scenarios` em `apps/api/src/routes/scenarios.ts`.
**Where:**
- `apps/api/src/routes/scenarios.ts` (novo)
- `apps/api/src/routes/scenarios.test.ts` (novo)
**Depends on:** T11, T09
**Reuses:** `ScenarioDTO` de `@ymir/types`, Drizzle client de T11; padrão idêntico a T12
**Requirement:** MONO-04

**Done when:**

- [ ] `GET /scenarios` retorna `200` com array `ScenarioDTO[]`
- [ ] `GET /scenarios/:id` com ID válido retorna `200` com `ScenarioDTO`
- [ ] `GET /scenarios/:id` com ID inválido retorna `404`
- [ ] `POST /scenarios` com body inválido retorna `400`
- [ ] `pnpm --filter @ymir/api test` — ≥ 5 testes de scenario passam (total ≥ 11)

**Tests:** unit (fastify.inject)
**Gate:** `pnpm --filter @ymir/api build && pnpm --filter @ymir/api test`

**Commit:** `feat(api): add scenarios CRUD routes with TypeBox validation`

---

### T14: Setup `apps/web` — Vite + React skeleton [P]

**What:** Criar workspace `apps/web` com Vite 5 + React 18 + TypeScript, configurar `vite.config.ts` com headers COOP/COEP, e criar estrutura de diretórios.
**Where:**
- `apps/web/package.json` (novo)
- `apps/web/tsconfig.json` + `tsconfig.node.json` (novo)
- `apps/web/vite.config.ts` (novo — COOP/COEP headers, worker format: 'es')
- `apps/web/src/main.tsx` (novo — entry point mínimo)
- `apps/web/src/App.tsx` (novo — placeholder "Ymir")
- `apps/web/public/wasm/.gitkeep` (novo — placeholder para artefatos)
**Depends on:** T06
**Reuses:** nenhum código existente
**Requirement:** MONO-03

**Done when:**

- [ ] `pnpm --filter @ymir/web dev` inicia Vite sem erro
- [ ] Browser abre `localhost:5173` e renderiza "Ymir" sem erro no console
- [ ] Headers de resposta incluem `Cross-Origin-Opener-Policy: same-origin`
- [ ] `pnpm --filter @ymir/web build` gera `dist/` sem erros TypeScript
- [ ] `apps/web/tsconfig.json` tem `strict: true`

**Tests:** build (tsc + vite build)
**Gate:** `pnpm --filter @ymir/web build`

**Commit:** `feat(web): add Vite React skeleton with COOP/COEP headers`

---

### T15: Criar `simulation.worker.ts` — Web Worker WASM loader [P]

**What:** Implementar o Web Worker que carrega o módulo WASM Emscripten, expõe `SimulationHandle`, e responde a mensagens `load`, `start`, `stop`, `reset` postando estado a 20Hz.
**Where:**
- `apps/web/src/workers/simulation.worker.ts` (novo)
**Depends on:** T14, T08 (precisa dos tipos gerados pelo Embind — usar `ymir.d.ts`)
**Reuses:** Tipos de `SimulationStateDTO` de `@ymir/types`
**Requirement:** MONO-03

**Done when:**

- [ ] Worker importa módulo WASM via `import('/wasm/ymir.js')` (dynamic import)
- [ ] Worker posta `{ type: 'ready' }` quando módulo inicializa
- [ ] Mensagem `{ type: 'start' }` inicia `setInterval` com `dt=0.05` (20Hz)
- [ ] Cada tick posta `{ type: 'state', data: SimulationStateDTO }` (JSON.parse do `getStateJson()`)
- [ ] Mensagem `{ type: 'stop' }` cancela o interval
- [ ] Worker trata erro de carregamento WASM e posta `{ type: 'error', code: 'WASM_LOAD_FAILED' }`
- [ ] `pnpm --filter @ymir/web build` compila sem erros TypeScript

**Tests:** build (tsc — não testamos Worker unitariamente, coberto pelo smoke test T19)
**Gate:** `pnpm --filter @ymir/web build`

**Commit:** `feat(web): add simulation Web Worker with WASM loader`

---

### T16: Criar Zustand store `simulationStore` [P]

**What:** Criar `apps/web/src/stores/simulationStore.ts` com Zustand gerenciando o estado da simulação (status do worker, último estado recebido, ações start/stop/reset).
**Where:**
- `apps/web/src/stores/simulationStore.ts` (novo)
**Depends on:** T14, T09 (`SimulationStateDTO` de `@ymir/types`)
**Reuses:** `SimulationStateDTO` de `@ymir/types`
**Requirement:** MONO-06

**Done when:**

- [ ] Store tem estado: `status: 'idle' | 'loading' | 'running' | 'stopped' | 'error'`
- [ ] Store tem: `lastState: SimulationStateDTO | null`
- [ ] Store tem actions: `startSimulation()`, `stopSimulation()`, `resetSimulation()`, `onWorkerMessage(msg)`
- [ ] `onWorkerMessage` atualiza `lastState` e `status` corretamente para cada tipo de mensagem
- [ ] TypeScript compila sem `any` implícito
- [ ] `pnpm --filter @ymir/web build` passa

**Tests:** build (tsc)
**Gate:** `pnpm --filter @ymir/web build`

**Commit:** `feat(web): add Zustand simulation store`

---

### T17: Criar `TelemetryPanel` — painel de estado em tempo real

**What:** Criar componente React `TelemetryPanel` que lê do `simulationStore`, conecta ao Worker, e exibe posição (x, y, z), velocidade (u, v, r) e heading (psi) do primeiro vessel em tempo real. Incluir botões Start/Stop/Reset.
**Where:**
- `apps/web/src/components/TelemetryPanel.tsx` (novo)
- `apps/web/src/components/TelemetryPanel.test.tsx` (novo — testa renderização com estado mockado)
- `apps/web/src/App.tsx` (modificar — montar `TelemetryPanel`)
- `apps/web/src/lib/workerBridge.ts` (novo — instancia Worker e conecta ao store)
**Depends on:** T15, T16
**Reuses:** `simulationStore` (T16), `SimulationStateDTO` (T09)
**Requirement:** MONO-06

**Done when:**

- [ ] `TelemetryPanel` renderiza tabela com campos: `x`, `y`, `z`, `psi`, `u`, `v`, `r` (N/A quando sem simulação)
- [ ] Botão "Start" chama `startSimulation()`, "Stop" chama `stopSimulation()`, "Reset" chama `resetSimulation()`
- [ ] `workerBridge.ts` instancia `simulation.worker.ts` e conecta `onmessage` ao `onWorkerMessage` do store
- [ ] Teste de renderização: com `lastState` mockado, painel exibe os valores corretos
- [ ] `pnpm --filter @ymir/web test` — ≥ 3 testes do TelemetryPanel passam
- [ ] `pnpm --filter @ymir/web build` passa

**Tests:** unit (React Testing Library)
**Gate:** `pnpm --filter @ymir/web build && pnpm --filter @ymir/web test`

**Commit:** `feat(web): add TelemetryPanel with start/stop/reset controls`

---

### T18: Pipeline Turborepo end-to-end — `pnpm build`

**What:** Garantir que `pnpm build` na raiz executa todo o pipeline na ordem correta: `core#build:wasm` → `packages/types#build` → `api#build` + `web#build`, com cache funcionando.
**Where:**
- `turbo.json` (ajuste fino de pipeline se necessário)
- `core/package.json` (script `build:wasm` verificado)
- Verificação de que artefatos WASM chegam em `apps/web/public/wasm/`
**Depends on:** T06, T07, T08, T09, T10, T11, T12, T13, T14, T15, T16, T17
**Reuses:** Toda a infraestrutura das tasks anteriores
**Requirement:** MONO-01

**Done when:**

- [ ] `pnpm build` do zero (sem cache) completa sem erros
- [ ] `apps/web/public/wasm/ymir.wasm` existe após o build
- [ ] Segunda execução de `pnpm build` sem mudanças: Turborepo usa cache (`>>> FULL TURBO`)
- [ ] `pnpm --filter @ymir/api test` passa (≥ 11 testes)
- [ ] `pnpm --filter @ymir/web test` passa (≥ 3 testes)
- [ ] `ctest --test-dir core/build --output-on-failure` — todos os testes C++ passam (≥ 39)

**Tests:** integration (pipeline completo)
**Gate:** `pnpm build && pnpm test`

**Commit:** `chore(monorepo): verify and fix Turborepo build pipeline end-to-end`

---

### T19: Smoke test WASM no browser

**What:** Verificar manualmente (ou com Playwright) que o WASM carrega no browser, o Worker inicializa, e a simulação roda 100 steps sem NaN/Inf no estado.
**Where:**
- `apps/web/src/lib/wasmSmoke.ts` (opcional — helper de debug)
- Teste Playwright em `apps/web/e2e/smoke.spec.ts` (opcional se Playwright disponível)
**Depends on:** T18
**Reuses:** Setup completo do T18
**Requirement:** MONO-03

**Done when:**

- [ ] `pnpm dev` inicia API (`localhost:3000`) e Web (`localhost:5173`) sem erro
- [ ] Browser abre `localhost:5173`, console não mostra erros de WASM
- [ ] Clicar "Start" → Worker posta `{ type: 'ready' }` → painel exibe valores numéricos
- [ ] Após 5 segundos de simulação (100 steps a 20Hz), nenhum campo do painel mostra `NaN` ou `Infinity`
- [ ] Clicar "Stop" → simulação para (valores congelam)
- [ ] Clicar "Reset" → painel volta para estado N/A

**Tests:** e2e (manual ou Playwright)
**Gate:** Verificação visual no browser (documentar resultado no STATE.md)

**Commit:** `test(web): document WASM smoke test results`

---

## Parallel Execution Map

```
Phase 1 (Sequential — C++ Refactor):
  T01 → T02 → T03 → T04 → T05

Phase 2 (Parallel — após T05):
  T05 ──┬──→ T06 [P]
        └──→ T07 [P]

Phase 3 (Paralelo — após T06+T07):
  T06+T07 ──┬──→ T08
             └──→ T09 [P com T08]

Phase 4 (Paralelo — após T09):
  T09 ──┬──→ T10 [P]
        └──→ T14 [P]

Phase 5 (Paralelo — após T10/T14):
  T10 ──→ T11 ──┬──→ T12 [P]
                 └──→ T13 [P]
  T14+T08 ──┬──→ T15 [P]
             └──→ T16 [P]

Phase 6 (Sequential):
  T15 + T16 ──→ T17

Phase 7 (Sequential):
  T12 + T13 + T17 ──→ T18 ──→ T19
```

---

## Task Granularity Check

| Task | Escopo | Status |
|------|--------|--------|
| T01: Extrair IIntegrator | 1 header | ✅ |
| T02: Implementar RK45Integrator | 1 classe + 1 test file | ✅ |
| T03: Wiring RK45 + remover SUNDIALS | ~4 arquivos (mechanical replace) | ✅ |
| T04: serializeStateJson | 1 método | ✅ |
| T05: Mover para core/ | reestruturação de dirs | ✅ |
| T06: Monorepo root configs | 3 arquivos de config cohesivos | ✅ |
| T07: Emscripten cmake + script | 2 arquivos de build cohesivos | ✅ |
| T08: YmirBindings.cpp | 1 arquivo | ✅ |
| T09: packages/types | 4 schema files cohesivos + package.json | ✅ |
| T10: apps/api skeleton | setup files cohesivos | ✅ |
| T11: Drizzle + seed | schema + migrations + seed cohesivos | ✅ |
| T12: Rotas vessels | 1 route file + tests | ✅ |
| T13: Rotas scenarios | 1 route file + tests | ✅ |
| T14: apps/web skeleton | setup files cohesivos | ✅ |
| T15: simulation.worker.ts | 1 arquivo | ✅ |
| T16: simulationStore.ts | 1 arquivo | ✅ |
| T17: TelemetryPanel | 1 component + 1 bridge + tests | ✅ |
| T18: Pipeline e2e | verificação de integração | ✅ |
| T19: Smoke test | verificação e2e | ✅ |

---

## Diagram-Definition Cross-Check

| Task | Depends On (body) | Diagrama mostra | Status |
|------|------------------|-----------------|--------|
| T01 | Nenhuma | Start | ✅ |
| T02 | T01 | T01 → T02 | ✅ |
| T03 | T02 | T02 → T03 | ✅ |
| T04 | T03 | T03 → T04 | ✅ |
| T05 | T04 | T04 → T05 | ✅ |
| T06 | T05 | T05 → T06 [P] | ✅ |
| T07 | T05 | T05 → T07 [P] | ✅ |
| T08 | T04, T07 | T06+T07 → T08 | ✅ |
| T09 | T06 | T06+T07 → T09 [P] | ✅ |
| T10 | T09 | T09 → T10 [P] | ✅ |
| T11 | T10 | T10 → T11 | ✅ |
| T12 | T11, T09 | T11 → T12 [P] | ✅ |
| T13 | T11, T09 | T11 → T13 [P] | ✅ |
| T14 | T06 | T09 → T14 [P] | ✅ |
| T15 | T14, T08 | T14+T08 → T15 [P] | ✅ |
| T16 | T14, T09 | T14+T08 → T16 [P] | ✅ |
| T17 | T15, T16 | T15+T16 → T17 | ✅ |
| T18 | T06..T17 (todos) | T12+T13+T17 → T18 | ✅ |
| T19 | T18 | T18 → T19 | ✅ |

---

## Test Co-location Validation

| Task | Layer criada/modificada | Matrix requer | Task diz | Status |
|------|------------------------|---------------|----------|--------|
| T01 | Refactor header (sem lógica nova) | build | build | ✅ |
| T02 | Nova implementação C++ | unit (Catch2) | unit | ✅ |
| T03 | Wiring + remoção (suite existente cobre) | integration | integration | ✅ |
| T04 | Novo método C++ | unit | unit | ✅ |
| T05 | Reestruturação de dirs | integration | integration | ✅ |
| T06 | Config files | none | none | ✅ |
| T07 | CMake + script | build | build | ✅ |
| T08 | WASM bindings C++ | build (WASM gate) | build | ✅ |
| T09 | TypeBox schemas | build (tsc) | build | ✅ |
| T10 | Fastify skeleton + health route | unit (inject) | unit | ✅ |
| T11 | Drizzle schema + seed | integration (SQLite real) | integration | ✅ |
| T12 | Rotas vessels | unit (inject) | unit | ✅ |
| T13 | Rotas scenarios | unit (inject) | unit | ✅ |
| T14 | Vite skeleton | build | build | ✅ |
| T15 | Web Worker | build (Worker não testável unit) | build | ✅ |
| T16 | Zustand store | build (lógica pura — sem DOM) | build | ✅ |
| T17 | React component + bridge | unit (RTL) | unit | ✅ |
| T18 | Pipeline completo | integration | integration | ✅ |
| T19 | Smoke test e2e | e2e | e2e | ✅ |
