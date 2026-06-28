# PRD — Ymir Simulator

## Visão Geral

Ymir é um simulador naval de alta fidelidade, capaz de rodar múltiplas simulações isoladas em paralelo.
Expõe uma API via WebSocket + Protobuf para clientes externos controlarem, observarem e visualizarem simulações.

Dois modos de operação, dois apps distintos:

| App | Modo | Clientes | Isolamento |
|-----|------|----------|-----------|
| `apps/server` | Real-time | Manager, Viewer (eventos ao vivo) | Thread por simulação |
| `apps/fast-time` | Batch acelerado | Nenhum ao vivo | Processo por simulação |

---

## Clientes Externos

O Ymir não tem interface visual — é um servidor. Os programas abaixo são externos ao repositório.

| Cliente | Função |
|---------|--------|
| **Manager** | Cria scenarios, carrega e controla simulações, acompanha eventos em real-time |
| **Viewer** | Renderiza a simulação em 3D, consome todos os eventos de estado |

Ambos se conectam ao `apps/server` via WebSocket + Protobuf.
Fast-time não tem clientes ao vivo — resultados são persistidos e consultados depois.

---

## Protocolo de Comunicação

**WebSocket + Protobuf** (schemas add-only, sem versionamento formal por ora)

- Binário: eficiente para updates de posição em alta frequência
- Schemas tipados: contrato claro entre server e clientes C++, C#, Node.js, Python
- WebSocket: simples de deployar, sem HTTP/2

---

## Persistência

**SQLite** — embutido, sem dependência externa, suporta queries.

| Dado | Descrição |
|------|-----------|
| Scenarios | Configurações de simulação reutilizáveis |
| Entidades | Navios, boias, âncoras, equipamentos e suas configurações |
| Histórico de simulações | Cada step registrado. Escrita em lote se volume for problema. |
| Logs e performance | Dados internos do servidor para debug e análise |

---

## Scenario

Configuração completa de uma simulação:
- Terreno e batimetria inicial
- Embarcações e configurações iniciais
- Condições ambientais iniciais
- Parâmetros de tempo e modo

O server persiste scenarios internamente. Clientes também podem enviar um scenario via API.
Um mesmo scenario pode gerar múltiplas runs independentes.

---

## Bounded Contexts

Cada contexto tem seu arquivo de detalhamento em `modules/`.

| Contexto | Módulos | Arquivo |
|----------|---------|---------|
| Física | Physics, Hydrodynamics, Mooring, Anchoring | [modules/physics.md](modules/physics.md) |
| Mundo | World, Environment, Terrain | [modules/world.md](modules/world.md) |
| Embarcação | Vessel, VesselState | [modules/vessel.md](modules/vessel.md) |
| Orquestração | Simulation, Events | [modules/simulation.md](modules/simulation.md) |
| Infraestrutura | Server, Persistence, Telemetry | [modules/infrastructure.md](modules/infrastructure.md) |

---

## Estrutura de Pastas

```
ymir/
│
├── apps/
│   ├── server/              # Real-time: WebSocket, Protobuf, API handlers
│   └── fast-time/           # Batch: submissão de scenarios, paralelismo por processo
│
├── libs/
│   ├── physics/             # [Física] Physics, Hydrodynamics, Mooring, Anchoring
│   ├── world/               # [Mundo] World, Environment, Terrain
│   ├── vessel/              # [Embarcação] Vessel, VesselState
│   ├── simulation/          # [Orquestração] Simulation, Events
│   ├── persistence/         # [Infraestrutura] SQLite, repositórios
│   └── common/              # Tipos compartilhados: vetores, unidades, interfaces base
│
├── services/                # Reservado — processos auxiliares futuros se necessário
│
├── tests/
│
└── docs/
```

---

## Prioridade de Desenvolvimento

| Fase | Escopo |
|------|--------|
| **1** | `libs/physics` — corpos rígidos, integração de movimento, forças, torques |
| **2** | `apps/server` — WebSocket + Protobuf, controle básico de simulação |
| **3** | `libs/world/environment` — vento, ondas, correntes, condições ambientais |
| **4** | Colisões e interações entre corpos |
| **5** | Hydrodynamics, Mooring, Anchoring |
| **6** | `libs/persistence`, `apps/fast-time`, Telemetry |
