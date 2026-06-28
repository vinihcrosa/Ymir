# Monorepo Migration — Specification

**Feature:** Migração do Ymir para monorepo com WASM core + API + Frontend
**Milestone:** M0
**Status:** Aguardando aprovação

---

## Problem Statement

O motor C++ atual é tecnicamente sólido mas inacessível: requer compilação nativa, Docker, e operação via CLI. A arquitetura atual está no momento ideal de migração — Phase 3 (WebSocket API) é um stub puro, o C API boundary já é mandatado pelo AGENTS.md, e o motor tem zero I/O no integration loop. Migrar agora para WASM + monorepo evita retrabalho futuro e abre o motor para uso interativo via browser.

---

## Goals

- [ ] Motor C++ compila para WASM via Emscripten e roda no navegador
- [ ] Monorepo unificado com um comando de build (`pnpm build`)
- [ ] API Fastify serve dados de vessels e cenários
- [ ] Frontend React carrega WASM em Web Worker e exibe estado de simulação
- [ ] Todos os testes C++ existentes continuam passando no target nativo

---

## Out of Scope

| Feature | Razão |
|---------|-------|
| Visualização 3D (Three.js/WebGPU) | Fase separada (M2+) |
| Autenticação e multi-usuário | Decisão adiada |
| Fast-time batch runner | Fora de scope do roadmap atual |
| Collision detection | Phase 4 do roadmap original |
| Mooring e anchoring | Fora de scope |
| Deploy e infra de produção | Separado — M0 é dev environment |

---

## User Stories

### P1: Monorepo funcional com build unificado ⭐ MVP

**User Story**: Como desenvolvedor, quero rodar `pnpm build` na raiz e compilar todo o projeto (C++ WASM + API + Web) de forma determinística, para reduzir o overhead de manter múltiplos build systems separados.

**Why P1:** Sem isso, nada mais funciona — é a fundação.

**Acceptance Criteria:**

1. WHEN desenvolvedor roda `pnpm build` na raiz THEN Turborepo SHALL executar `core#build:wasm`, `api#build`, `web#build` na ordem correta com cache
2. WHEN `core#build:wasm` completa THEN SHALL existir `apps/web/public/wasm/ymir.wasm` e `ymir.js`
3. WHEN nenhum arquivo C++ mudou THEN `core#build:wasm` SHALL usar cache do Turborepo (< 5s)
4. WHEN desenvolvedores rodam `pnpm test` THEN SHALL rodar testes C++ nativos + testes TypeScript
5. WHEN `pnpm dev` é executado THEN SHALL iniciar `apps/api` e `apps/web` em modo watch concorrentemente

**Independent Test:** `pnpm build` do zero completa sem erros; `apps/web/public/wasm/ymir.wasm` existe.

---

### P1: Integrador RK45 substitui CVODE ⭐ MVP

**User Story**: Como engenheiro do motor, quero que o integrador ODE seja uma implementação pura C++ sem dependências de sistema, para que o core compile para WASM via Emscripten sem modificações no Makefile do sistema.

**Why P1:** SUNDIALS/CVODE é o único bloqueador técnico para WASM. Sem resolver isso, Emscripten não compila.

**Acceptance Criteria:**

1. WHEN código C++ compila com Emscripten toolchain THEN SHALL compilar sem erros relacionados a SUNDIALS ou dependências de sistema
2. WHEN integrador RK45 roda um oscilador harmônico com solução analítica conhecida THEN erro SHALL ser < `1e-5` com `rtol=1e-6, atol=1e-8`
3. WHEN step size adaptation é necessário THEN integrador SHALL reduzir dt automaticamente para manter erro dentro de tolerância
4. WHEN todos os testes existentes rodam com RK45 THEN SHALL passar (mesmos casos de integração)
5. WHEN dependência SUNDIALS é removida do `cmake/Dependencies.cmake` THEN build nativo SHALL continuar funcionando sem ela

**Independent Test:** `ctest --test-dir build` passa com 0 falhas; SUNDIALS não está mais nas dependências.

---

### P1: WASM roda no browser via Web Worker ⭐ MVP

**User Story**: Como desenvolvedor frontend, quero carregar o motor de simulação como um módulo WASM em um Web Worker, para que a simulação não bloqueie a UI e o estado seja atualizado em tempo real.

**Why P1:** Sem Worker, simulation loop congela o browser. Requisito de UX não-negociável.

**Acceptance Criteria:**

1. WHEN página carrega THEN Worker SHALL inicializar módulo WASM sem erro no console
2. WHEN Worker está pronto THEN SHALL postMessage `{ type: 'ready' }` para main thread
3. WHEN main thread envia `{ type: 'start' }` THEN Worker SHALL iniciar loop de simulação
4. WHEN simulação roda THEN Worker SHALL postMessage estado a ≥ 10Hz sem bloquear main thread
5. WHEN WASM não suporta SharedArrayBuffer no browser THEN SHALL mostrar mensagem de erro clara (COOP/COEP missing)
6. WHEN Worker termina inesperadamente THEN main thread SHALL detectar e exibir erro

**Independent Test:** Abre browser em `localhost:5173`, DevTools não mostra jank na main thread enquanto simulação roda.

---

### P1: API Fastify com rotas de vessels e cenários ⭐ MVP

**User Story**: Como desenvolvedor frontend, quero buscar a lista de vessels e cenários da API, para que o frontend possa listar e carregar configurações sem hardcode.

**Why P1:** API é necessária mesmo no MVP — o frontend precisa de dados de algum lugar.

**Acceptance Criteria:**

1. WHEN `GET /vessels` é chamado THEN SHALL retornar array de `VesselDTO` com status 200
2. WHEN `GET /vessels/:id` com ID inexistente é chamado THEN SHALL retornar 404 com mensagem
3. WHEN `GET /scenarios` é chamado THEN SHALL retornar array de `ScenarioDTO` com status 200
4. WHEN body inválido é enviado para `POST /scenarios` THEN SHALL retornar 400 com erros de validação do TypeBox
5. WHEN servidor inicia THEN SHALL carregar dados iniciais do SQLite (seed de vessels de exemplo)
6. WHEN `GET /health` é chamado THEN SHALL retornar `{ status: 'ok' }` com 200

**Independent Test:** `curl localhost:3000/vessels` retorna JSON com pelo menos um vessel.

---

### P1: packages/types compartilhado ⭐ MVP

**User Story**: Como desenvolvedor, quero tipos TypeScript compartilhados entre API e Web derivados do mesmo schema, para que não haja drift entre o que a API retorna e o que o frontend espera.

**Why P1:** Sem types compartilhados, a API pode mudar e quebrar o frontend silenciosamente.

**Acceptance Criteria:**

1. WHEN `packages/types` é importado em `apps/api` THEN TypeScript SHALL compilar sem erros
2. WHEN `packages/types` é importado em `apps/web` THEN TypeScript SHALL compilar sem erros
3. WHEN schema TypeBox de `VesselDTO` é alterado THEN TypeScript SHALL mostrar erro em todos os places que usam o tipo desatualizado
4. WHEN API serializa response de vessel THEN SHALL ser type-safe contra `VesselDTO` de `packages/types`
5. WHEN `packages/types` é buildado THEN SHALL emitir declarações `.d.ts`

**Independent Test:** Alterar um campo em `VesselDTO` causa erro de tipo em `apps/api` e `apps/web` sem rodar nenhum teste.

---

### P2: Frontend React com painel de telemetria

**User Story**: Como usuário, quero ver os valores numéricos de posição, velocidade e heading do vessel em tempo real, para acompanhar o estado da simulação sem precisar da CLI.

**Why P2:** Útil mas a simulação já funciona sem — é feedback visual, não funcionalidade core.

**Acceptance Criteria:**

1. WHEN simulação está rodando THEN painel SHALL mostrar posição (x, y, z), velocidade e heading atualizados ≥ 10Hz
2. WHEN simulação está pausada THEN painel SHALL manter últimos valores exibidos
3. WHEN não há simulação ativa THEN painel SHALL mostrar estado "aguardando"

**Independent Test:** Abre browser, inicia simulação, valores mudam visivelmente no painel.

---

### P3: Hot reload do WASM em desenvolvimento

**User Story**: Como desenvolvedor C++, quero que alterar um arquivo `.cpp` no core recompile o WASM e recarregue o browser automaticamente, para ter feedback rápido sem rodar build manualmente.

**Why P3:** Nice-to-have para DX — build manual funciona para MVP.

**Acceptance Criteria:**

1. WHEN arquivo `.cpp` do core é salvo THEN Turborepo SHALL detectar mudança e recompilar WASM
2. WHEN WASM é recompilado THEN Vite SHALL detectar mudança em `public/wasm/` e recarregar

---

## Edge Cases

- WHEN browser não suporta SharedArrayBuffer (COOP/COEP ausentes) THEN SHALL exibir mensagem explicativa sobre configuração de headers
- WHEN WASM excede memória disponível THEN Worker SHALL capturar exceção e notificar main thread
- WHEN simulação roda por > 1 hora THEN integridade numérica SHALL ser mantida (sem NaN/Inf)
- WHEN API está offline THEN frontend SHALL mostrar estado de erro sem crash
- WHEN SQLite não existe na primeira execução THEN API SHALL criar database e aplicar seed

---

## Requirement Traceability

| ID | Story | Fase | Status |
|----|-------|------|--------|
| MONO-01 | P1: Monorepo + build unificado | Design | Pendente |
| MONO-02 | P1: Integrador RK45 | Design | Pendente |
| MONO-03 | P1: WASM em Web Worker | Design | Pendente |
| MONO-04 | P1: API Fastify vessels/cenários | Design | Pendente |
| MONO-05 | P1: packages/types compartilhado | Design | Pendente |
| MONO-06 | P2: Frontend painel telemetria | Design | Pendente |
| MONO-07 | P3: Hot reload WASM dev | — | Pendente |

---

## Success Criteria

- [ ] `pnpm build` do zero completa em < 5 min (com cache Turborepo ativado na segunda run: < 30s)
- [ ] Todos os 35+ testes C++ passam no target nativo pós-migração
- [ ] Smoke test WASM no browser: simulação roda 100 steps sem NaN/Inf no estado
- [ ] API retorna vessels e cenários com schema validado
- [ ] Frontend carrega WASM e exibe telemetria sem jank na UI thread
