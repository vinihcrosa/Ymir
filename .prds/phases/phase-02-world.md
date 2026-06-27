# Fase 2 — World + Terrain

## Objetivo

Implementar o `World` como fonte única de verdade do estado da simulação, o `Terrain` como representação do ambiente físico, e o tick completo de 12 passos conforme a spec de orquestração. Ao final desta fase, a simulação executa o ciclo determinístico completo — incluindo `Environment` dinâmico, `Terrain`, `Vessel`, todas as forças, integração e eventos — sobre uma entidade real em um mundo com batimetria.

---

## Estado Atual (entrada da fase)

O que já existe:
- Todos os modelos de força (`libs/physics/forces/`)
- `WaveForces` + `WaveSpectrum` (`libs/world/wave/`)
- `NavalEnvironment` (wrapper básico de ambiente)
- `NavalSimulation` com tick parcial
- `ManeuverController`, `BerthManeuverSystem` (Fase 1)

O que falta (esta fase entrega):
- `World` — entity manager com queries espaciais e snapshots
- `Terrain` — batimetria, estruturas fixas, modificações em runtime
- `Environment` completo — vento, corrente, maré como campos dinâmicos
- `Events` — pub/sub completo, integrado ao tick
- Tick de 12 passos em `NavalSimulation` (ou novo `SimulationOrchestrator`)
- `VesselState` integrada ao tick (eventos de estado operacional)

---

## Escopo

### Incluído nesta fase

- `World::addEntity`, `World::removeEntity`, `World::state(id)`, `World::snapshot()`
- Queries espaciais: `World::entitiesInRadius(pos, r)`, `World::depthAt(x, y)`
- `Terrain::depthAt(x, y)` — batimetria por interpolação bilinear em grid
- `Terrain::addStructure`, `Terrain::modify` (dragagem)
- `Environment` dinâmico: vento, corrente, maré, chuva, visibilidade
- `EventBus` — pub/sub interno síncrono + propagação para assinantes externos (preparar interface; sem WebSocket ainda)
- Tick completo de 12 passos
- Integração de `SquatForces` com `Terrain::depthAt`
- Integração de `WaveForces` com `Environment` (estado do mar, direção)
- Testes de tick completo: ciclo determinístico, estado converge, eventos disparados corretamente

### Excluído desta fase

- WebSocket / Protobuf (Fase 3) — `EventBus` tem interface de assinante mas sem wire para rede
- Mooring (Fase 4) — cabos não fazem parte do tick desta fase
- Anchoring (Fase 4) — passos 9 e 10 são stub nesta fase
- Colisões (Fase 5)
- SQLite persistence (Fase 6)

---

## Módulos

### World

Fonte única da verdade do estado da simulação.

**Responsabilidades:**
- Criar e remover entidades por ID
- Armazenar e servir estado atual de cada entidade (q, q_dot)
- Responder queries espaciais
- Gerar snapshots completos (usado por Persistence e Server)

**Interface principal:**
```cpp
class World {
public:
    void addEntity(int id, EntityType type, const BodyState& initialState);
    void removeEntity(int id);

    BodyState  state(int id) const;
    WorldSnapshot snapshot() const;

    // Queries espaciais
    std::vector<int> entitiesInRadius(double x, double y, double radius) const;
    double depthAt(double x, double y) const;  // delega para Terrain

    // Escrita de estado — chamado por Physics após integração
    void updateState(int id, const BodyState& state);
};
```

**WorldSnapshot:**
```cpp
struct WorldSnapshot {
    double                       simTime;
    std::vector<EntitySnapshot>  entities;   // id, type, q[12], velocities
    EnvironmentSnapshot          environment;
};
```

**Notas de design:**
- `World` não executa cálculos — apenas armazena e serve
- Thread-safety não é requisito desta fase (uma simulação por thread)
- `depthAt` é passthrough para `Terrain` — `World` agrega o acesso

---

### Terrain

Ambiente físico fixo e modificável.

**Batimetria:**
- Grid regular de profundidades `depth[nx][ny]` com origem e resolução configuráveis
- `depthAt(x, y)` — interpolação bilinear no grid
- Quando `(x,y)` fora do grid → profundidade padrão (deep water)

**Estruturas fixas:**
```cpp
struct FixedStructure {
    std::string id;
    StructureType type;     // Pier, Buoy, Obstacle, Coast
    std::vector<Point2D> polygon;
    double height;
};
```

**Modificações em runtime:**
```cpp
void Terrain::modify(const TerrainPatch& patch);
// patch: área retangular + delta de profundidade (dragagem)
// publica evento terrain_changed ao EventBus
```

**Consumidores de `depthAt`:**
- `SquatForces` — Froude de profundidade: `Fn = v / sqrt(g · depth)`
- `Anchoring` (Fase 4) — detectar toque de âncora no fundo

---

### Environment (campo dinâmico completo)

Ampliação do `NavalEnvironment` atual para campo dinâmico completo.

**Campos:**

| Campo | Tipo | Unidade |
|-------|------|---------|
| Vento | velocidade + direção | m/s, °náutico |
| Corrente | velocidade + direção | m/s, °náutico |
| Maré | nível de água | m |
| Estado do mar | Hs, Tp, direção | m, s, ° |
| Chuva | intensidade | mm/h |
| Visibilidade | distância | m |

**Variação espacial de corrente:**
- Corrente uniforme (campo escalar) — default
- Spatial field: `currentAt(x, y)` — interpolação em grid (opcional nesta fase, stub aceitável)

**Maré:**
- `tide_level` afeta `depthAt`: `effective_depth = Terrain::depthAt(x,y) + tide_level`
- `SquatForces` usa `effective_depth`

**API de mutação (preparar para Server):**
```cpp
void Environment::setWind(double speed_ms, double dir_deg);
void Environment::setCurrent(double speed_ms, double dir_deg);
void Environment::setTide(double level_m);
void Environment::setSeaState(double Hs, double Tp, double dir_deg);
```

Cada mutação publica evento `environment_changed` ao `EventBus`.

---

### EventBus

Pub/sub síncrono interno. Propagação para clientes externos via interface preparada mas não wired ao WebSocket (Fase 3).

**Eventos desta fase:**

| Evento | Publicado por | Payload |
|--------|--------------|---------|
| `entity_created` | World | id, type, initial_state |
| `entity_removed` | World | id |
| `environment_changed` | Environment | campo, valor |
| `terrain_changed` | Terrain | patch |
| `simulation_started` | Simulation | sim_time |
| `simulation_paused` | Simulation | sim_time |
| `simulation_ended` | Simulation | sim_time, final_snapshot |
| `vessel_state_changed` | VesselState | id, luzes, shapes, estado |

**Interface:**
```cpp
class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    void subscribe(EventType type, Handler h);
    void publish(const Event& event);

    // Para Server (Fase 3): assinante externo recebe todos os eventos
    void addExternalSink(ExternalEventSink* sink);
};
```

Implementação síncrona — handlers executam inline no tick, sem filas ou threads.

---

### Tick Completo de 12 Passos

Após esta fase, `NavalSimulation::step(dt)` executa:

```
1.  environment_.update(t, dt)         -- vento/corrente/maré
2.  terrain_.applyPendingModifications()
3.  vessel_.updateControl(t, dt)       -- seleciona e executa controlador (Fase 1)
4.  vessel_.updateStates(t, dt)        -- atuadores + cinemática derivada (Fase 1)
5.  hydrodynamics_.compute(...)        -- DampingForces, CurrentForces, SquatForces,
                                          ThrustForces, RudderForces, TugForces
6.  waveForces_.compute(...)           -- excitação + drift + RAO
7.  inertialForces_.compute(...)       -- Coriolis/giroscópico
8.  restoringForces_.compute(...)      -- hidrostática
9.  mooring_.compute(...)              -- STUB (Fase 4)
10. anchoring_.compute(...)            -- STUB (Fase 4)
11. physics_.step(dt)                  -- CVODE integra → World atualizado
12. eventBus_.publish(tick_events)     -- estado + eventos do tick
```

**Contrato do passo 11:**
- `physics_.step(dt)` acumula todas as forças dos passos 5–10
- Chama CVODE: `q_new = CVODE(q, q_dot, ΣF, dt)`
- Escreve `world_.updateState(id, new_state)`

---

## Acceptance Criteria

1. `World::snapshot()` retorna estado consistente após cada tick
2. `depthAt(x, y)` com interpolação bilinear — erro < 1e-4 m em pontos de teste conhecidos
3. Maré afeta `effective_depth` e portanto squat: `SquatForces` com `tide=2m` diferente de `tide=0m`
4. `EventBus` entrega eventos corretos na sequência certa por tick
5. `Environment::setWind(...)` durante simulação reflete nas `WindForces` do próximo tick
6. `Terrain::modify(...)` durante simulação reflete no `depthAt` imediatamente
7. Tick completo de 12 passos executa sem erro com 1 embarcação e 1 waypoint
8. `NavalSimulation` com `ManeuverController` (Fase 1) navega 5 waypoints consecutivos em tick completo
9. Cobertura de testes ≥ 80%

---

## Decisões Abertas para TechSpec

- `World` é um singleton por simulação ou passado por referência? (tendência: por referência, um por `NavalSimulation`)
- Grid de batimetria — formato de entrada: arquivo? struct? JSON? (provavelmente JSON no scenario reader)
- `Environment` com spatial field de corrente — necessário desde o início ou stub escalar?
- `EventBus` síncrono ou assíncrono? (síncrono é mais simples e determinístico)
- `EntityType` enum: `Vessel`, `Buoy`, `Anchor`, `FixedStructure`? Definir agora.
- Stub de Mooring/Anchoring no passo 9/10 — retornam `Forces{}` zero ou são ausentes do tick?
