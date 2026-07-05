# TechSpec — Simulação no Servidor (Server-Side Simulation)

## Executive Summary

Move a simulação de física do browser para o servidor. O `apps/api` (Fastify, Node, ESM) passa a
hospedar cada simulação em um **`worker_thread`** dedicado (ADR-002), que carrega o **mesmo motor
WASM** de hoje — empacotado como `@ymir/wasm`, um pacote interno do monorepo compilado com alvo Node
(ADR-003). O thread principal do Fastify só roteia: recebe `ClientMessage` via **WebSocket + JSON**,
repassa ao worker correto, e devolve `ServerMessage` (snapshots, status, erros) aos clientes
(ADR-004). O cliente web deixa de rodar qualquer física: o Web Worker e o fallback mock são
removidos; o `simulationStore` troca `postMessage` por uma conexão WebSocket, preservando sua API
pública. Simulações sobrevivem à desconexão e são reencontradas por `sim_id` guardado em
`localStorage`, com TTL de limpeza de órfãs (ADR-006). Testes rodam contra o motor real, com WASM
compilado no CI (ADR-005).

**Principal trade-off:** ganhamos motor único, estado autoritativo e fundação para persistência ao
custo de (a) dependência de servidor sempre online, (b) latência de rede entre comando e estado
(mitigada por interpolação client-side), e (c) CI mais pesado por compilar Emscripten.

## System Architecture

### Component Overview

```
┌───────────────────────── apps/web (React) ─────────────────────────┐
│  simulationStore (API pública inalterada: play/pause/reset/          │
│    loadScenario/applyEnvironment)                                    │
│        └─ SimulationSocket (novo)  ── WebSocket + JSON ──┐           │
│  render (Three.js) interpola entre snapshots             │           │
└──────────────────────────────────────────────────────────┼──────────┘
                                                             │
              HTTP REST (dados: vessels/areas/scenarios)     │ ws://…/ws
                          │                                  │
┌─────────────────────────┼──────────────────────────────────┼──────────┐
│  apps/api (Fastify, thread principal)                       │          │
│    rotas REST (inalteradas)          WsGateway (@fastify/websocket)     │
│                                          └─ MessageRouter               │
│                                               └─ SimulationManager      │
│                                                    │ sim_id → worker    │
│   ┌──────────────── worker_thread (por sim) ───────┼──────────────┐    │
│   │  SimulationWorker: loop 20 Hz                   ▼              │    │
│   │    └─ SimulationEngine (de @ymir/wasm) ── WASM (ymir.js/.wasm) │    │
│   │    vessel config: lido via vessel-service (dado), não HTTP     │    │
│   └────────────────────────────────────────────────────────────┘      │
└────────────────────────────────────────────────────────────────────────┘

packages/types  →  ClientMessage / ServerMessage / SimulationStateDTO (compartilhados)
packages/wasm   →  @ymir/wasm: loader + SimulationEngine + binários (gerados)
```

**Responsabilidades:**

- **`SimulationSocket` (web, novo):** abre/mantém o WebSocket, (de)serializa mensagens, reconecta,
  entrega estado ao `simulationStore`. Substitui o `Worker` interno do store.
- **`WsGateway` (api):** rota WebSocket via `@fastify/websocket`; associa conexão a `sim_id`.
- **`MessageRouter` (api):** valida `ClientMessage` (TypeBox) e despacha para o `SimulationManager`.
- **`SimulationManager` (api, thread principal):** cria/encerra `worker_thread`s, mantém
  `sim_id → { worker, conexões, timer TTL }`, aplica limite de sims e política de TTL.
- **`SimulationWorker` (worker_thread):** roda o loop de tick a 20 Hz, aplica comandos ao motor,
  publica snapshots via `postMessage`.
- **`SimulationEngine` (@ymir/wasm):** interface fina sobre o WASM (espelha `YmirBindings.cpp`).

### Data Flow

1. Cliente envia `CreateSimulation{scenario}` → `MessageRouter` valida → `SimulationManager` cria
   worker, gera `sim_id` → responde `SimulationCreated{sim_id}` (cliente grava em `localStorage`).
2. `Play` → manager repassa ao worker → worker roda loop; a cada tick emite `State{SimulationStateDTO}`
   → manager reencaminha a todas as conexões daquele `sim_id`.
3. `SetActuator`/`LoadEnvironment` → worker aplica no motor no próximo tick.
4. Cliente desconecta → manager arma TTL; reconexão (`AttachSimulation{sim_id}`) cancela o TTL e
   devolve o snapshot atual.

## Implementation Design

### Core Interfaces

Interface do motor, exposta por `@ymir/wasm` e consumida pelo worker (espelha a fachada
`YmirSimulation` de `core/src/wasm/YmirBindings.cpp`):

```typescript
// packages/wasm/src/index.ts
export interface SimulationEngine {
  addVesselAt(id: number, x: number, y: number, psi: number): void
  setRudderAngle(vesselId: number, rudderId: number, angleDeg: number): void
  setThrusterCommand(vesselId: number, thrusterId: number, powerPct: number, azimuthDeg: number): void
  step(dt: number): void
  getState(): SimulationStateDTO
  loadEnvironment(json: string): void
  setWaveConditions(hs: number, tp: number, dirDeg: number, gamma: number, spectrum: number): void
  reset(): void
  delete(): void // libera heap Embind
}

/** Carrega o módulo WASM (ENVIRONMENT=node, MODULARIZE) e instancia o motor. */
export function createSimulationEngine(): Promise<SimulationEngine>
```

Fronteira do gerenciador de simulações (thread principal):

```typescript
// apps/api/src/simulation/simulation-manager.ts
export interface SimulationManager {
  create(scenario: ScenarioInput): Promise<SimId>
  attach(simId: SimId, conn: WsConnection): boolean   // false se sim_id não existe
  detach(simId: SimId, conn: WsConnection): void       // arma TTL se ficou sem conexões
  command(simId: SimId, cmd: ClientMessage): void      // repassa ao worker
  stop(simId: SimId): void                             // encerra worker imediatamente
}
```

### Data Models

Novos DTOs em `packages/types` (TypeBox), reaproveitando os payloads existentes:

```typescript
// packages/types/src/protocol.ts (novo)
export const ClientMessage = Type.Union([
  Type.Object({ type: Type.Literal('CreateSimulation'), scenario: ScenarioInput }),
  Type.Object({ type: Type.Literal('AttachSimulation'), simId: Type.String() }),
  Type.Object({ type: Type.Literal('Play'), simId: Type.String(), dt: Type.Optional(Type.Number()) }),
  Type.Object({ type: Type.Literal('Pause'), simId: Type.String() }),
  Type.Object({ type: Type.Literal('Reset'), simId: Type.String() }),
  Type.Object({ type: Type.Literal('LoadScenario'), simId: Type.String(), vessels: Type.Array(ScenarioDraftVessel) }),
  Type.Object({ type: Type.Literal('SetActuator'), simId: Type.String(), vesselId: Type.Number(),
                deviceType: Type.Union([Type.Literal('rudder'), Type.Literal('thruster')]),
                deviceId: Type.Number(), value: Type.Number(), value2: Type.Optional(Type.Number()) }),
  Type.Object({ type: Type.Literal('LoadEnvironment'), simId: Type.String(), json: Type.String() }),
])

export const ServerMessage = Type.Union([
  Type.Object({ type: Type.Literal('SimulationCreated'), simId: Type.String() }),
  Type.Object({ type: Type.Literal('State'), payload: SimulationStateDTO }),
  Type.Object({ type: Type.Literal('Status'), status: Type.Union([
    Type.Literal('running'), Type.Literal('paused'), Type.Literal('ended')]) }),
  Type.Object({ type: Type.Literal('VesselConfig'), payload: VesselConfigDTO }),
  Type.Object({ type: Type.Literal('Error'), message: Type.String() }),
])
```

- `SimulationStateDTO`, `VesselStateDTO`, `VesselConfigDTO`, `ScenarioDraftVessel` já existem e são
  reutilizados.
- `WorkerMessageDTO` e `SimulationEngine = 'wasm'|'mock'` (union) são **removidos** de
  `packages/types/src/simulation.ts` junto com o worker/mock.

### API Endpoints

- **WebSocket:** `GET /ws` — upgrade para WebSocket; canal único de comando/estado. Toda a
  interação de simulação passa por aqui via `ClientMessage`/`ServerMessage`.
- **REST (inalterado):** `/vessels`, `/vessels/:id`, `/vessels/:id/config`, `/areas`, `/scenarios`,
  `/health` permanecem exatamente como estão (dados de projeto).

## Integration Points

- **`@fastify/websocket`** (linha compatível com Fastify 4 — v8.x, **não** v10 que exige Fastify 5).
  Registrado como plugin no `apps/api/src/index.ts`.
- **`vessel-service` (interno):** o worker/manager obtém a config de embarcação chamando o service
  **diretamente** (o dado vive no mesmo processo), eliminando o `fetch` HTTP que o worker faz hoje.
- **`@ymir/wasm`:** dependência `workspace:*` de `apps/api`; binários gerados pelo `build:wasm`.
- **Autenticação:** nenhuma nesta fase (rede local/confiável).

## Impact Analysis

| Component | Impact Type | Description and Risk | Required Action |
|-----------|-------------|----------------------|-----------------|
| `packages/wasm` (`@ymir/wasm`) | new | Novo pacote: loader + `SimulationEngine` + binários Node. Risco médio (build Emscripten Node) | Criar pacote, ajustar CMake/`build-wasm.sh` p/ `ENVIRONMENT=node,MODULARIZE` |
| `packages/types/src/protocol.ts` | new | `ClientMessage`/`ServerMessage`. Risco baixo | Criar DTOs TypeBox |
| `packages/types/src/simulation.ts` | modified | Remover `WorkerMessageDTO` e union `mock`. Risco baixo | Remover tipos aposentados |
| `apps/api` — `simulation/` | new | `WsGateway`, `MessageRouter`, `SimulationManager`, `SimulationWorker`. Risco alto (core da feature) | Implementar módulo de simulação + rota `/ws` |
| `apps/api/src/index.ts` | modified | Registrar `@fastify/websocket` e a rota `/ws`. Risco baixo | Registrar plugin/rota |
| `apps/web/src/stores/simulationStore.ts` | modified | Troca `Worker` por `SimulationSocket`; API pública mantida. Risco médio | Reescrever implementação interna |
| `apps/web/src/lib/simulation-socket.ts` | new | Cliente WebSocket + reconexão + `localStorage(sim_id)`. Risco médio | Criar |
| `apps/web/src/workers/simulation.worker.ts` | deprecated | Removido | Deletar + testes |
| `apps/web/src/workers/mock-wasm.ts` | deprecated | Removido (ADR-005) | Deletar + `mock-wasm.test.ts` |
| `apps/web/public/wasm/` | deprecated | Cliente não carrega WASM | Remover pasta e headers COOP/COEP se não usados p/ outra coisa |
| `.github/workflows/ci.yml` | modified | Estágio de build Emscripten antes dos testes. Risco médio (tempo/toolchain) | Adicionar setup emsdk + `build:wasm` + cache |
| `turbo.json` / scripts | modified | `build:wasm` produz em `packages/wasm`; deps de build | Ajustar pipeline turbo |

## Testing Approach

Regra: **testes contra o motor real; sem mock de engine** (ADR-005). Dados (cenários, configs) podem
ser mockados; o motor não. Cobertura ≥ 80%.

### Unit Tests

- **`MessageRouter`:** validação de `ClientMessage` (TypeBox) — mensagens válidas/ inválidas,
  despacho correto por `type`. Dados mockados.
- **`SimulationManager`:** criação/registro/`attach`/`detach`, limite de sims, arme/cancelamento de
  TTL (timers fake), término idempotente. O worker pode ser exercitado de verdade (thread real) já
  que o motor é real.
- **`@ymir/wasm` loader:** instancia o motor real e verifica a superfície da interface
  (`addVesselAt`, `step`, `getState`, …) contra valores conhecidos/limites analíticos.
- **`simulation-socket.ts` (web):** máquina de reconexão, leitura/escrita de `localStorage`,
  (de)serialização — com um WebSocket de teste (o transporte é mockável; o motor não entra aqui).

### Integration Tests

- **Roundtrip real:** subir o `apps/api` com `/ws`, um cliente WebSocket de teste envia
  `CreateSimulation` → `Play` → `SetActuator` e assere que os `State` recebidos refletem a física
  real (motor WASM). Critério da PRD: roundtrip comando→efeito < ~100 ms local.
- **Reconexão:** criar sim, desconectar, reconectar com `AttachSimulation{sim_id}`, assere retomada
  do snapshot atual; e expiração por TTL encerra a sim após o prazo (timer configurável curto no teste).
- **Paridade:** um cenário conhecido produz a mesma trajetória do comportamento atual do WASM.
- **Dependência de ambiente:** exige `@ymir/wasm` compilado (CI compila; ver ADR-005).

## Development Sequencing

### Build Order

1. **`@ymir/wasm`** — criar pacote; ajustar CMake/`build-wasm.sh` p/ alvo Node (`ENVIRONMENT=node`,
   `MODULARIZE`); expor `createSimulationEngine()` + `SimulationEngine`. *Sem dependências.*
2. **`packages/types` — protocolo** — adicionar `ClientMessage`/`ServerMessage`; remover
   `WorkerMessageDTO`/union `mock`. *Depende de 1 (reusa tipos de estado; alinhado à interface).*
3. **`SimulationWorker`** — loop 20 Hz + aplicação de comandos usando a interface do passo 1.
   *Depende de 1 e 2.*
4. **`SimulationManager`** — ciclo de vida de workers, `sim_id`, TTL, limite de sims. *Depende de 3.*
5. **`WsGateway` + `MessageRouter` + rota `/ws`** — `@fastify/websocket`, validação e despacho;
   registrar no `index.ts`. *Depende de 2 e 4.*
6. **`simulation-socket.ts` (web)** — cliente WebSocket, reconexão, `localStorage`. *Depende de 2 e 5.*
7. **`simulationStore` religado** — trocar `Worker` por `SimulationSocket`, manter API pública.
   *Depende de 6.*
8. **Remoção** — deletar `simulation.worker.ts`, `mock-wasm.ts`, `apps/web/public/wasm/`, tipos
   aposentados. *Depende de 7 (só remover após paridade validada).*
9. **CI** — estágio de build WASM + cache; rodar testes reais. *Depende de 1; fecha com 3–5.*

### Technical Dependencies

- **Toolchain Emscripten (emsdk)** disponível localmente e no CI (bloqueia passos 1 e 9).
- **`@fastify/websocket` v8.x** compatível com Fastify 4 (bloqueia passo 5).
- Compatibilidade Node do Embind (`emscripten::val::global("JSON")`) — validar no passo 1.

## Monitoring and Observability

- **Métricas por simulação:** tempo médio de `step()`, taxa efetiva de tick (Hz real vs 20 Hz alvo),
  nº de conexões, idade da sim.
- **Métricas do servidor:** nº de sims ativas, workers vivos, sims encerradas por TTL.
- **Logs estruturados (logger do Fastify):** `sim_created`, `sim_attached`, `sim_detached`,
  `sim_ttl_expired`, `sim_stopped`, `ws_error`, `command_invalid` — com `sim_id` como campo.
- **Alertas (futuro):** nº de workers acima do limite; tick real caindo abaixo de um piso.

## Technical Considerations

### Key Decisions

- **Decisão:** um `worker_thread` por simulação (ADR-002). **Rationale:** não bloquear o event loop
  do Fastify; isolar CPU; base para multiusuário. **Trade-off:** serialização main↔worker e memória
  por sim. **Rejeitado:** in-process (bloqueia rede); processo separado (orquestração demais).
- **Decisão:** WASM como `@ymir/wasm` com alvo Node (ADR-003). **Rationale:** fronteira limpa, alvo
  de build correto. **Trade-off:** um pacote a mais. **Rejeitado:** WASM em `apps/api` direto; reusar
  a pasta do web.
- **Decisão:** WebSocket + JSON, `ClientMessage`/`ServerMessage` (ADR-004). **Rationale:** reusa
  DTOs/TypeBox, sem toolchain nova. **Trade-off:** payload maior que binário. **Rejeitado:** Protobuf;
  reusar `WorkerMessageDTO`.
- **Decisão:** testes reais, WASM no CI (ADR-005). **Rationale:** sem fake divergente; motor único.
  **Trade-off:** CI mais lento/pesado. **Rejeitado:** fake de engine; mock como fixture.
- **Decisão:** `sim_id` + `localStorage` + TTL 30 min (ADR-006). **Rationale:** reconexão sem UI
  nova; não vaza workers. **Trade-off:** reconexão presa ao navegador. **Rejeitado:** lista de sims;
  nunca-encerrar; encerrar-imediato.

### Known Risks

- **CI/Emscripten (média):** build pesado. *Mitigação:* imagem com emsdk + cache de artefatos turbo.
- **Overhead de snapshot a 20 Hz por sim (média):** *Mitigação:* payload enxuto (DTO atual); validar
  eventos só em dev; migrar p/ binário se necessário (ADR-001/004).
- **Render "picotado" pela taxa de rede (média):** *Mitigação:* interpolação/extrapolação
  client-side entre snapshots.
- **Compatibilidade Embind↔Node (média):** *Mitigação:* protótipo do loader no passo 1 antes de
  seguir.
- **Race TTL × reconexão (baixa):** *Mitigação:* cancelamento atômico do timer no `attach`; término
  idempotente.

## Architecture Decision Records

- [ADR-001: Motor de simulação no servidor via WASM em Node, cliente como terminal remoto](adrs/adr-001.md)
  — Rodar o mesmo WASM em processo Node; cliente vira terminal remoto (vs. servidor C++/Protobuf).
- [ADR-002: Uma simulação por `worker_thread` do Node](adrs/adr-002.md)
  — Isolar cada simulação em thread própria para não bloquear o event loop do Fastify.
- [ADR-003: Módulo WASM como pacote interno `@ymir/wasm` com alvo de build Node](adrs/adr-003.md)
  — Empacotar o WASM em `packages/wasm` (privado), compilado para `ENVIRONMENT=node`.
- [ADR-004: Transporte WebSocket + JSON com contrato `ClientMessage`/`ServerMessage`](adrs/adr-004.md)
  — WebSocket via `@fastify/websocket`, JSON reusando DTOs TypeBox (vs. Protobuf).
- [ADR-005: Testes contra o motor real; sem mock de engine; WASM compilado no CI](adrs/adr-005.md)
  — Nenhum fake de motor; CI compila o WASM; `mock-wasm` removido.
- [ADR-006: Ciclo de vida de simulação — `sim_id`, reconexão via `localStorage`, TTL de órfãs](adrs/adr-006.md)
  — Reconexão por `sim_id` guardado no cliente; simulações órfãs expiram após TTL (default 30 min).
</content>
