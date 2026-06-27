# Roadmap — Ymir Simulator

## Escopo deste roadmap

Hidrodinâmica naval, colisão entre corpos, e controle via API. Sem mooring, anchoring,
batimetria complexa, fast-time, SQLite, telemetria, ou domínios não-navais.

---

## Estado Atual — Fase 0 concluída

| Módulo | Status | Localização |
| --- | --- | --- |
| `RigidBody6DOF` + integrador CVODE BDF | ✅ | `libs/physics/` |
| `InertialForces` | ✅ | `libs/physics/forces/` |
| `RestoringForces` (hidrostática) | ✅ | `libs/physics/forces/` |
| `DampingForces` (potencial + linear + quadrático) | ✅ | `libs/physics/forces/` |
| `SquatForces` (ICORELS) | ✅ | `libs/physics/forces/` |
| `CurrentForces` (Obokata + Regular/Van der Pol) | ✅ | `libs/physics/forces/` |
| `WindForces` (Cd por ângulo) | ✅ | `libs/physics/forces/` |
| `ThrustForces` (Kt/Kq open-water) | ✅ | `libs/physics/forces/` |
| `RudderForces` (foil Cl/Cd) | ✅ | `libs/physics/forces/` |
| `TugForces` (PUSH/PULL/ESCORTING simplificado) | ✅ | `libs/physics/forces/` |
| `WaveForces` + `WaveSpectrum` (JONSWAP/Pierson/Regular) | ✅ | `libs/world/wave/` |
| `NavalSimulation` (orquestrador básico, 1 corpo) | ✅ | `libs/simulation/` |
| `NavalContext` / `VesselConfig` | ✅ | `libs/vessel/` |
| JSON scenario reader | ✅ | `libs/persistence/` |
| Math utils + PhysicalConstants | ✅ | `libs/common/` |

---

## Fases

### Fase 1 — Camada de Embarcação

**O que entrega:** uma embarcação com propulsores, lemes e rebocadores como entidades reais,
capaz de navegar autonomamente por waypoints ou reproduzir comandos gravados.

**Bloqueado por:** nada — depende apenas da Fase 0.

**Componentes:**

- **Thruster / Rudder como entidades** (`libs/vessel/`)
  - Separação entre config estática (parâmetros físicos) e estado dinâmico (RPM atual, ângulo atual)
  - Cada entidade aplica seus próprios filtros de 1ª ordem e limitadores de taxa por tick
  - `ThrustForces` e `RudderForces` passam a ler do estado da entidade, não da config
  - Ver decisão D8

- **`DynamicVessel`** — aggregate root que possui `Thruster[]`, `Rudder[]`, controlador ativo
  - `updateControl(t, dt)` — executa controlador, escreve demands nas entidades
  - `updateStates(t, dt)` — avança dinâmica de cada entidade, computa cinemática derivada (SOG, COG, deriva)

- **Controladores** — selecionados por `std::variant`, dispatch por `std::visit`
  - `ManeuverController` — LOS + PID de heading e velocidade, lista de waypoints
  - `PrescribedController` — replay de série temporal (leme e RPM por tempo)
  - `BerthManeuverSystem` — FSM de 3 estados com rebocadores paramétricos (tugs ainda não são corpos independentes)

- **`VesselState`** — estado operacional visível (luzes de navegação, shapes COLREGS, modo operacional)
  - Transições manuais via API; sem eventos de âncora ou amarração (ver D9)

- **NavalSimulation multi-corpo** — corrige limitação de 1 corpo; suporta N `RigidBody6DOF`
  - Ver decisão D11

---

### Fase 2 — World + Multi-domínio naval

**O que entrega:** todos os corpos navais coexistindo no mesmo mundo com acoplamento de forças
entre eles. `NavalSimulation` é substituído por `World + NavalDomain`.

**Bloqueado por:** Fase 1.

**Componentes:**

- **`World`** — orquestrador de domínios (ver D1, D3, `world-architecture.md`)
  - Relógio global
  - Contém `Simulation` (container de corpos) e `Environment` (ambiente compartilhado)
  - Coordena tick multi-domínio com acoplamento fraco Jacobi (ver D2)

- **`NavalDomain`** — substitui `NavalSimulation` (ver D5)
  - Gerencia N corpos navais, um `NavalContext` por corpo
  - Registra `DynamicVessel` para cada corpo naval

- **`Environment`** (global) — corrente, vento, ondas, profundidade (scalar)
  - Sobe de `NavalSimulation` para o `World`; todos os domínios lêem daqui

- **`CouplingRegistry`** — acoplamento fraco entre corpos via `CouplingPort`
  - Força + posição + velocidade no ponto de interface (ver D6)
  - Permite rebocadores como corpos independentes trocando força com o navio

- **Queries espaciais básicas** — posição de todos os corpos, distância entre pares
  - Necessário para collision (Fase 4) e para `BerthManeuverSystem` com tugs independentes

---

### Fase 3 — API de controle

**O que entrega:** controle e observação do simulador via rede. Cenários carregados e
controlados externamente. Estado transmitido em tempo real.

**Bloqueado por:** Fase 2 (API transmite estado do World).

**Componentes:**

- **`apps/server`** — WebSocket + Protobuf
  - Carrega cenário JSON e inicializa o `World`
  - Aceita comandos de controle: trocar waypoints, mudar modo, alterar ambiente
  - Transmite stream de estado: `BodyState` de todos os corpos + `VesselState` por embarcação

- **Protocolo** — Protobuf
  - Mensagens: `ScenarioLoad`, `ControlCommand`, `StateUpdate`, `VesselStateUpdate`
  - Sem autenticação, sem multi-tenant — conexão local ou LAN confiável

- **Extensão do JSON reader** — suporte a waypoints e configuração de controlador no cenário

---

### Fase 4 — Colisão entre corpos

**O que entrega:** detecção e resposta a colisões entre corpos rígidos no mundo.

**Bloqueado por:** Fase 2 (queries espaciais do World são pré-requisito para broadphase).

**Componentes:**

- **Geometria de colisão** — representação simplificada por corpo
  - Casco: hull convex ou OBB (oriented bounding box) configurável
  - Configurado via `VesselConfig` (seção de colisão)

- **Broadphase** — filtragem de pares candidatos por AABB
  - Atualizado a cada tick com posições do World

- **Narrowphase** — GJK para detecção de contato entre convexos

- **Resposta a colisão** — impulso com restituição e fricção configuráveis
  - Força de separação aplicada via `CouplingPort` do `CouplingRegistry`
  - Mesma interface de acoplamento da Fase 2 — sem novo mecanismo

---

## Dependências entre fases

```text
Fase 0 (concluída)
  └── Fase 1: Vessel Layer
        └── Fase 2: World + NavalDomain
              ├── Fase 3: API de controle
              └── Fase 4: Colisão
```

Fases 3 e 4 são independentes entre si — podem ser desenvolvidas em paralelo após Fase 2.

---

## Fora do escopo deste roadmap

- Mooring e Anchoring (linhas de amarração, catenária, fundeio)
- Batimetria como campo espacial (profundidade é scalar global)
- Domínios não-navais (estrutural, elétrico, aerodinâmico)
- Fast-time (runner batch acelerado)
- SQLite (JSON reader é suficiente)
- Telemetria

Esses itens podem ser adicionados em roadmap futuro sem breaking change na arquitetura,
graças ao modelo de domínios extensíveis do World (ver `world-architecture.md` e `decisions.md`).
