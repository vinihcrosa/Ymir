# Plano — Migração para Simulação no Servidor

**Status:** Proposta (nenhum código alterado)
**Escopo:** Restruturação client-server — mover a física do cliente para o servidor
**Autor:** análise assistida por IA
**Data:** 2026-07-03

---

## 1. Objetivo

Hoje a física roda **localmente no browser** (WASM em um Web Worker). Este plano descreve a
mudança para um modelo onde **a simulação roda no servidor** e o cliente apenas:

1. Envia comandos (criar simulação, play/pause, leme, RPM, ambiente, waypoints).
2. Recebe *snapshots* de estado em tempo real e renderiza.

O cliente deixa de ser dono da simulação e passa a ser um **terminal remoto** de uma simulação
autoritativa que vive no servidor.

Boa notícia: **a arquitetura alvo já está planejada no repositório.** `apps/server` existe como
stub, `docs/planning/prds/phases/phase-03-server.md` e `docs/planning/prds/modules/infrastructure.md`
já especificam o servidor WebSocket + Protobuf, e `docs/architecture/README.md` afirma que o Ymir
"exposes a WebSocket + Protobuf API to external clients". Este plano **ativa** esse desenho e
**religa o cliente web** a ele.

---

## 2. Estado atual (diagnóstico)

### Onde a física roda hoje

```
apps/web/src/workers/simulation.worker.ts   ← loop de 20 Hz no browser
  ├─ carrega /wasm/ymir.js (Embind) ............ core C++ compilado p/ WASM
  ├─ fallback: mock-wasm.ts (cinemática JS) .... quando ymir.wasm não existe
  └─ postMessage({type:'state'}) → simulationStore (zustand)
```

- **`apps/web/src/stores/simulationStore.ts`** cria o Worker, faz `play/pause/reset/loadScenario/applyEnvironment`
  e recebe `state` via `worker.onmessage`.
- **`core/src/wasm/YmirBindings.cpp`** expõe a classe `YmirSimulation` (addVesselAt, setRudderAngle,
  setThrusterCommand, step, getState, loadEnvironment, setWaveConditions) — a mesma superfície que o
  servidor precisará expor.
- **`apps/api`** (Fastify/Node) serve **apenas dados** (vessels, areas, scenarios, vessel config).
  **Não roda física.**
- **`apps/server`** (C++) é um stub `INTERFACE` que linka `Ymir::Simulation` + `Ymir::Persistence`.
  Vazio — nenhum WebSocket, nenhum loop, nenhum handler.
- **`packages/types/src/simulation.ts`** define `SimulationStateDTO`, `VesselStateDTO`, `WorkerMessageDTO`
  — hoje o contrato é worker↔UI; vira contrato rede↔UI.

### Problemas do modelo atual (motivação)

| Problema | Consequência |
|----------|--------------|
| Física por cliente | Cada aba/usuário roda sua própria simulação — nada é compartilhado |
| CPU do cliente é o teto | Cenários pesados travam a UI; sem controle de recursos |
| Sem estado autoritativo | Impossível múltiplos observadores da mesma simulação |
| WASM vs mock divergem | Dois motores para manter; risco de comportamento diferente |
| Sem persistência de run | Simulação morre ao fechar a aba |

---

## 3. Arquitetura alvo

```
┌─────────────────────┐         WebSocket + Protobuf          ┌──────────────────────────┐
│      apps/web        │  ◄──────────────────────────────────►│       apps/server (C++)    │
│  (React + Three.js)  │                                       │                            │
│                      │  ClientMessage (comandos) ──────────► │  WebSocketServer           │
│  simulationStore     │                                       │    └─ CommandRouter        │
│    └─ ws client      │  ◄────────── ServerMessage (eventos)  │  SessionManager            │
│       (substitui o   │              WorldSnapshot @ N Hz     │    └─ N × World/Simulation │
│        Web Worker)   │                                       │  EventForwarder            │
└─────────────────────┘                                       │  SimulationClock (realtime)│
        │                                                      └──────────────────────────┘
        │ HTTP (CRUD scenarios/vessels/areas)                              │ usa
        ▼                                                                  ▼
┌─────────────────────┐                                       ┌──────────────────────────┐
│   apps/api (Node)    │  (inalterado — só dados)              │ libs/{simulation,world,     │
└─────────────────────┘                                       │  physics,vessel,persistence}│
                                                               └──────────────────────────┘
```

Dois canais no cliente, com papéis distintos:

- **HTTP → `apps/api`**: CRUD de dados de projeto (scenarios, vessels, areas). **Não muda.**
- **WebSocket → `apps/server`**: ciclo de vida e controle da simulação em tempo real. **Novo.**

---

## 4. Decisão-chave: qual servidor roda a física?

Há duas opções. **O repositório já escolheu a Opção A** (docs + stub `apps/server`); este plano
segue essa escolha e registra o trade-off.

### Opção A — Servidor C++ nativo (`apps/server`) — **recomendada**

O cliente fala WebSocket + Protobuf **direto** com um processo C++ que linka `libs/simulation`,
`libs/world`, etc.

- ✅ Alinhada com docs existentes (phase-03-server, infrastructure, architecture README)
- ✅ Física roda nativa (CVODE completo, sem limites de WASM)
- ✅ Reusa `libs/*` sem reescrever nada de física
- ✅ Múltiplas simulações isoladas por thread (já previsto em data-flow.md)
- ⚠️ Requer escolher lib WebSocket C++ (uWebSockets / libwebsockets / Boost.Beast) — decisão aberta
- ⚠️ Introduz Protobuf no build C++ e um gerador de tipos TS a partir do `.proto`

### Opção B — Estender `apps/api` (Node) como gateway

Node mantém o WebSocket com o browser e chama a física via FFI/child-process (WASM em Node, ou
binário C++ via IPC/N-API).

- ✅ Um só ponto de rede (Node), auth/CRUD/realtime juntos
- ✅ Reusa TypeBox/JSON — sem Protobuf
- ❌ Contradiz a arquitetura documentada
- ❌ Overhead de ponte Node↔C++; WASM em Node teria o mesmo teto de performance de hoje
- ❌ Fica difícil isolar simulações por thread/processo com garantias de tempo real

**Recomendação:** Opção A. Manter `apps/api` para dados; `apps/server` C++ para simulação.
A decisão final (e a escolha da lib WS + Protobuf) deve virar um **ADR** antes de codar
(ver `docs/planning/prds/phases/phase-03-server.md` — "Decisões Abertas para TechSpec").

---

## 5. Contrato de protocolo

O protocolo já está esboçado em `phase-03-server.md`. Consolidação para esta migração:

### Comandos (cliente → servidor) — `ClientMessage`

| Mensagem | Mapeia o que o cliente faz hoje |
|----------|----------------------------------|
| `CreateSimulation{scenario_json}` | `loadScenario()` + boot do worker |
| `StopSimulation{sim_id}` | `reset()` |
| `PauseSimulation` / `ResumeSimulation` | `pause()` / `play()` |
| `SetSpeed{factor}` | (novo) fast-time multiplier |
| `SetRudder{vessel_id, angle_deg[]}` | `setActuator(rudder)` |
| `SetRPM{vessel_id, rpm[]}` | `setActuator(thruster)` — hoje `setThrusterCommand(power%, azimuth)` |
| `SetEnvironment{update}` | `applyEnvironment()` / `loadEnvironment()` + `setWaveConditions()` |
| `SetWaypoints{vessel_id, waypoints[]}` | (novo — depende de controlador de waypoint) |

### Eventos (servidor → cliente) — `ServerMessage`

| Evento | Substitui |
|--------|-----------|
| `WorldSnapshot{entities[]}` @ N Hz | `WorkerMessageDTO{type:'state'}` |
| `SimulationStatus{running/paused/ended}` | estados internos do store |
| `EntityCreated` / `EntityRemoved` | (novo) |
| `EnvironmentChanged` | feedback de `applyEnvironment` |
| erro | `WorkerMessageDTO{type:'error'}` |

### Nota sobre serialização

O plano-fonte usa **Protobuf**. Alternativa mais leve: **JSON sobre WebSocket** reusando os DTOs
TypeBox atuais (`SimulationStateDTO`) — elimina toolchain Protobuf e mantém o contrato TS existente,
ao custo de payloads maiores. Recomendação: **começar com JSON** (menor risco, reusa `packages/types`)
e migrar para Protobuf se o bandwidth do snapshot virar gargalo. Registrar em ADR.

---

## 6. Componentes do servidor (a construir em `apps/server`)

Todos já especificados em `phase-03-server.md`; resumo do que precisa existir:

1. **`WebSocketServer`** — aceita conexões, identifica `Manager` vs `Viewer`, (de)serializa mensagens.
2. **`SessionManager`** — mapeia conexão ↔ `sim_id`; cria/encerra `World`; limite de simulações simultâneas.
3. **`CommandRouter`** — despacha `ClientMessage` para operações na simulação correta.
4. **`EventForwarder`** — assina o `EventBus`/`ExternalEventSink`, serializa eventos, envia aos clientes.
5. **`SimulationClock`** — loop real-time (1× wall clock, `dt_target` 50 ms / 20 Hz), suporta fast-time.
6. **Bootstrap** — `main()` que sobe o servidor, lê porta de config/env, instancia os componentes.

Pré-requisitos de física que **precisam ser verificados** antes (o PRD assume que existem):
- `World::snapshot()` retornando estado completo
- `EventBus` com `ExternalEventSink`
- API de mutação do `Environment`
- Superfície de comando por corpo (leme, RPM) equivalente ao que `YmirBindings.cpp` já expõe

> A superfície de física necessária já existe hoje no wrapper WASM (`YmirBindings.cpp`). O servidor
> pode espelhar exatamente essa fachada `YmirSimulation`, extraída para uma classe reusável em
> `libs/simulation` ou em `apps/server`, evitando divergência entre WASM e servidor.

---

## 7. Componentes do cliente (a alterar em `apps/web`)

O ponto central: **substituir o Web Worker WASM por um cliente WebSocket**. A superfície pública do
`simulationStore` (play/pause/reset/loadScenario/applyEnvironment/setActuator) **permanece igual** —
só a implementação interna troca de `postMessage` para `ws.send`.

| Arquivo | Mudança |
|---------|---------|
| `stores/simulationStore.ts` | Trocar criação de `Worker` por conexão WS; `postMessage`→`ws.send`; `onmessage` passa a decodificar `ServerMessage` |
| `workers/simulation.worker.ts` | **Removido** (ou mantido atrás de flag "modo offline" — ver §9) |
| `workers/mock-wasm.ts` | **Removido** ou reaproveitado como servidor de dev local |
| `packages/types/src/simulation.ts` | `WorkerMessageDTO` → `ServerMessage`/`ClientMessage`; DTOs viram contrato de rede (ou gerados do `.proto`) |
| (novo) `lib/simulation-socket.ts` | Cliente WS: conectar, reconectar, (de)serializar, fila de comandos |
| `.env` / config | `VITE_SIM_WS_URL` (default `ws://localhost:8080`) |

**Preservado sem mudança:** toda a renderização (Ocean, Area3DView, VesselMarker, geo/geo3d),
os stores de UI (vesselPanelStore, environmentStore), e o CRUD via `apps/api`.

### Diferenças de comportamento a tratar no cliente

- **Latência de rede**: hoje o estado é síncrono no worker; via rede há delay. Considerar
  interpolação/extrapolação client-side para render suave a 60 fps sobre snapshots a 20 Hz.
- **Reconexão**: definir se retoma o snapshot atual do servidor ou reinicia (decisão aberta no PRD).
- **Autoridade**: o cliente não muta mais estado local — só envia comando e espera o snapshot.
  Actuadores no `vesselPanelStore` viram "comando pendente" até o servidor confirmar.

---

## 8. Faseamento (migração incremental, sem big-bang)

### Fase 0 — Fundação e decisões (bloqueia o resto)
- ADR: Opção A vs B; lib WebSocket; Protobuf vs JSON; auth (nenhuma nesta fase?).
- Verificar que `libs/{simulation,world}` expõem snapshot + comandos + EventBus necessários.
- Extrair a fachada de `YmirBindings.cpp` para uma classe reusável (fonte única de verdade).

### Fase 1 — Servidor mínimo (echo do WASM atual)
- `apps/server`: WebSocketServer + SessionManager + SimulationClock básicos.
- Suportar: `CreateSimulation`, `Start/Stop/Pause/Resume`, `SetRudder`, `SetRPM`, `SetEnvironment`.
- Emitir `WorldSnapshot` a 20 Hz. **Paridade funcional com o worker atual.**
- Teste de integração: cliente mock → comando → snapshot de volta (< 100 ms wall, critério do PRD).

### Fase 2 — Cliente WebSocket atrás de flag
- Novo `lib/simulation-socket.ts` + `simulationStore` com backend selecionável (`wasm` | `remote`).
- Rodar os dois lado a lado; validar que a UI se comporta igual contra o servidor.
- E2E Playwright existentes (`simulation-run.spec.ts`) devem passar em ambos os modos.

### Fase 3 — Corte e limpeza
- Tornar `remote` o padrão.
- Remover (ou esconder atrás de flag "offline") o Worker WASM e o `mock-wasm`.
- Atualizar docs de arquitetura e data-flow; rodar `graphify update .`.

### Fase 4 — Além da paridade (opcional, futuro)
- Múltiplos viewers da mesma simulação.
- `SetSpeed` / fast-time, waypoints, persistência de run (SQLite — Fase 6 do roadmap).

---

## 9. Decisão: manter WASM como modo offline?

Duas posturas possíveis — **decidir no ADR**:

- **Remover WASM**: um só motor (servidor). Menos manutenção, mas exige servidor sempre online;
  perde o "abrir e rodar" sem backend.
- **Manter WASM atrás de flag**: `simulationStore` com dois backends (`remote` default, `wasm`
  offline). Mantém demo/local sem servidor, ao custo de dois motores para manter sincronizados.

Recomendação: **manter o backend selecionável** durante a migração (necessário para a Fase 2 de
qualquer forma) e decidir a remoção definitiva só depois da paridade comprovada.

---

## 10. Impacto por área (resumo)

| Área | Impacto | Muda física? |
|------|---------|--------------|
| `libs/physics`, `libs/world`, `libs/vessel` | Nenhum (talvez expor snapshot/comandos se faltar) | Não |
| `core/src/wasm/YmirBindings.cpp` | Extrair fachada para classe reusável | Não |
| `apps/server` | **Construção completa** (WS, router, sessão, clock, forwarder) | Não |
| `apps/api` (Node) | Nenhum — continua servindo dados | Não |
| `apps/web` stores/workers | `simulationStore` religado; worker removido/flag | Não |
| `packages/types` | Contrato worker→rede; possivelmente gerado do `.proto` | Não |
| `proto/` (novo, se Protobuf) | Schemas + geração TS/C++ | Não |
| `docs/architecture`, `data-flow` | Atualizar para refletir cliente remoto | Não |

---

## 11. Riscos e decisões abertas

- **Lib WebSocket C++**: uWebSockets vs libwebsockets vs Boost.Beast (peso × maturidade).
- **Protobuf vs JSON**: toolchain × tamanho de payload. Recomendação: JSON primeiro.
- **Thread-safety**: como `CommandRouter` acessa `NavalSimulation` com o `SimulationClock`
  avançando o tick? (lock por simulação / fila de comandos aplicada no início do tick).
- **Frequência de snapshot**: 20 Hz para todos ou configurável por cliente/viewer.
- **Reconexão**: retomar snapshot atual vs reiniciar.
- **Autenticação**: nenhuma nesta fase (LAN confiável) — confirmar.
- **Interpolação no cliente**: necessária para render suave; define o quão "esperto" o cliente fica.
- **Determinismo WASM↔servidor**: se manter os dois, garantir que produzem a mesma trajetória
  (mesmos parâmetros VLCC, mesmo integrador) ou documentar a divergência.

---

## 12. Critérios de aceite (herdados do PRD phase-03-server)

1. Cliente conecta via WS, envia `CreateSimulation`, recebe `sim_id`.
2. `SetRPM` → tick seguinte usa novo RPM; `SetEnvironment(wind)` reflete no próximo tick.
3. Cliente recebe `WorldSnapshot` a cada tick com posição correta.
4. `PauseSimulation` emite `SimulationStatus(paused)`.
5. Duas simulações simultâneas, estados independentes.
6. Cliente desconecta — simulação continua no servidor.
7. Roundtrip comando → evento < 100 ms (wall) em teste de integração.
8. UI web (`apps/web`) renderiza contra o servidor com paridade ao modo WASM atual.
9. Cobertura de testes ≥ 80% (regra global do projeto).

---

## 13. Não-metas

- Persistência de runs (SQLite) — Fase 6 do roadmap.
- `apps/fast-time` — fora deste plano.
- Colisões, mooring, anchoring — fases próprias.
- Multi-tenant / auth robusta — LAN confiável por ora.

---

## 14. Próximos passos

1. Aprovar este plano e a Opção A (servidor C++).
2. Escrever ADR das decisões abertas (§11) e um TechSpec de `apps/server`.
3. Verificar/expor a superfície de física necessária (§6) em `libs/simulation`/`libs/world`.
4. Executar Fases 1→3 do §8.

> Todo o código permanece **inalterado** até a aprovação. Este documento é somente o plano.
</content>
</invoke>
