# Fase 3 — Server

## Objetivo

Implementar `apps/server` — o ponto de entrada externo do Ymir em modo real-time. Ao final desta fase, Manager e Viewer conseguem se conectar via WebSocket, criar e controlar simulações, enviar comandos a embarcações, alterar condições ambientais, e receber todos os eventos e snapshots de estado em tempo real via Protobuf.

---

## Estado Atual (entrada da fase)

O que já existe:
- Simulação completa com tick de 12 passos (Fase 2)
- `EventBus` com interface de `ExternalEventSink` preparada
- `World::snapshot()` retorna estado completo
- `Environment` com API de mutação

O que falta (esta fase entrega):
- `apps/server` — processo real-time com WebSocket
- Schemas Protobuf para todos os comandos e eventos
- Handlers de API (criar/encerrar/pausar/retomar simulação, comandos a embarcações, etc.)
- Propagação de eventos do `EventBus` para clientes via WebSocket
- Loop real-time com sincronização de clock
- Gerenciamento de múltiplas simulações simultâneas (1–2 por instância)

---

## Escopo

### Incluído nesta fase

- `apps/server` — executável com loop real-time
- Biblioteca de schemas Protobuf (`proto/`)
- `WebSocketServer` — aceita conexões de Manager e Viewer
- `SessionManager` — associa conexões a simulações
- `CommandRouter` — roteia mensagens recebidas para handlers corretos
- `EventForwarder` — assina `EventBus`, serializa eventos, envia para clientes
- `SimulationClock` — sincronização real-time (1×) com wall clock
- API completa conforme spec de infraestrutura
- Testes de integração: cliente mock → servidor → simulação → eventos de volta

### Excluído desta fase

- Autenticação de clientes (decidir em TechSpec se é necessário desde o início)
- `apps/fast-time` (Fase 6)
- SQLite persistence (Fase 6) — servidor desta fase não persiste runs
- Mooring/Anchoring no tick (Fases 4 e 5)
- Colisões (Fase 5)

---

## Módulos

### Schemas Protobuf (`proto/`)

Localização: `proto/` na raiz do repositório. Add-only versioning — nunca remover ou renumerar campos.

**Mensagens de comando (Manager → Server):**
```protobuf
message CreateSimulation { string scenario_json = 1; }
message StopSimulation   { uint32 sim_id = 1; }
message PauseSimulation  { uint32 sim_id = 1; }
message ResumeSimulation { uint32 sim_id = 1; }
message SetSpeed         { uint32 sim_id = 1; float factor = 2; }

message SetRudder        { uint32 sim_id = 1; uint32 vessel_id = 2;
                           repeated float angle_deg = 3; }
message SetRPM           { uint32 sim_id = 1; uint32 vessel_id = 2;
                           repeated float rpm = 3; }
message SetWaypoints     { uint32 sim_id = 1; uint32 vessel_id = 2;
                           repeated Waypoint waypoints = 3; }
message SetEnvironment   { uint32 sim_id = 1; EnvironmentUpdate update = 2; }
message SetTerrain       { uint32 sim_id = 1; TerrainPatch patch = 2; }

message Waypoint         { float x = 1; float y = 2; float v = 3; }
message EnvironmentUpdate {
    oneof field {
        WindUpdate   wind = 1;
        CurrentUpdate current = 2;
        TideUpdate   tide = 3;
        SeaStateUpdate sea_state = 4;
    }
}
```

**Mensagens de evento (Server → Clientes):**
```protobuf
message SimulationEvent {
    uint32 sim_id  = 1;
    double sim_time = 2;
    oneof payload {
        WorldSnapshot      snapshot       = 3;
        CollisionEvent     collision      = 4;
        MooringRuptured    mooring        = 5;
        AnchorHolding      anchor_holding = 6;
        AnchorDragging     anchor_dragging = 7;
        EnvironmentChanged env_changed    = 8;
        TerrainChanged     terrain_changed = 9;
        SimulationStatus   status         = 10;
        EntityCreated      entity_created = 11;
        EntityRemoved      entity_removed = 12;
    }
}

message WorldSnapshot {
    repeated EntitySnapshot entities = 1;
    EnvironmentSnapshot environment = 2;
}

message EntitySnapshot {
    uint32 id    = 1;
    string type  = 2;
    repeated float q     = 3;   // 6 doubles
    repeated float qdot  = 4;   // 6 doubles
}
```

**Envelope WebSocket:**
```protobuf
message ClientMessage { oneof msg { /* todos os comandos */ } }
message ServerMessage  { oneof msg { /* todos os eventos */ } }
```

---

### WebSocketServer

Baseado em biblioteca WebSocket C++ (a decidir em TechSpec: uWebSockets, libwebsockets, beast/asio).

**Responsabilidades:**
- Aceitar conexões na porta configurável (default: 8080)
- Identificar tipo de cliente: `Manager` ou `Viewer` (via handshake ou primeiro payload)
- Deserializar `ClientMessage` Protobuf recebido
- Encaminhar ao `CommandRouter`
- Enviar `ServerMessage` Protobuf serializado

**Lifecycle de conexão:**
```
Cliente conecta → SessionManager::onConnect(conn)
Cliente manda CreateSimulation → Simulation criada → sim_id retornado
Cliente manda StopSimulation → Simulation encerrada
Cliente desconecta → SessionManager::onDisconnect(conn)
```

---

### SessionManager

Mantém mapeamento conexão ↔ simulação.

```cpp
class SessionManager {
public:
    uint32_t createSimulation(const std::string& scenario_json);
    void     stopSimulation(uint32_t sim_id);
    void     attachClient(ConnectionId conn, uint32_t sim_id, ClientRole role);
    void     detachClient(ConnectionId conn);

    NavalSimulation* simulation(uint32_t sim_id);
    std::vector<ConnectionId> clientsOf(uint32_t sim_id);
};
```

Limites: máximo 2 simulações simultâneas em real-time (configurável).

---

### CommandRouter

Despacha comandos recebidos para operações na simulação correta.

```cpp
class CommandRouter {
public:
    void handle(ConnectionId from, const ClientMessage& msg);

private:
    void onSetRudder(uint32_t sim_id, const SetRudder& cmd);
    void onSetRPM(uint32_t sim_id, const SetRPM& cmd);
    void onSetWaypoints(uint32_t sim_id, const SetWaypoints& cmd);
    void onSetEnvironment(uint32_t sim_id, const EnvironmentUpdate& upd);
    // ...
};
```

---

### EventForwarder

Assina `EventBus` de cada simulação. Serializa e envia para clientes conectados.

```cpp
class EventForwarder : public ExternalEventSink {
public:
    void onEvent(uint32_t sim_id, const Event& event) override;
    // serializa → ServerMessage Protobuf → envia para clientsOf(sim_id)
};
```

**Frequência de snapshots:**
- `WorldSnapshot` a cada tick (configurável: 1× por tick ou N Hz)
- Outros eventos: imediatos no tick em que ocorrem

---

### SimulationClock

Garante que cada simulação real-time avança 1 segundo de tempo simulado para 1 segundo de wall clock.

```
Loop principal:
  t_wall_next = t_wall_now + dt_target
  simulation.step(dt_target)
  t_elapsed = clock() - t_wall_before
  sleep(t_wall_next - t_elapsed)   // não dormir se atrasado
```

`dt_target` padrão: 50 ms (20 Hz). Configurável.

**Fast-time multiplier (fator N×):**
- `dt_target` de simulação inalterado
- Sleep omitido (ou dt_wall = dt_sim / factor)
- Limite: CPU-bound, sem garantia de realismo temporal

---

## Acceptance Criteria

1. Manager conecta via WebSocket, envia `CreateSimulation`, recebe `sim_id`
2. Manager envia `SetRPM` → tick seguinte usa novo RPM
3. Manager envia `SetEnvironment(wind)` → `WindForces` do próximo tick refletem
4. Viewer recebe `WorldSnapshot` a cada tick com posição correta
5. `SimulationStatus(paused)` emitido quando Manager envia `PauseSimulation`
6. Segunda simulação pode ser criada simultaneamente — estados independentes
7. Cliente desconecta — simulação continua rodando
8. Simulação com waypoints navega corretamente com cliente conectado
9. Testes de integração: cliente mock completa roundtrip comando → evento em < 100 ms (wall)
10. Cobertura de testes ≥ 80%

---

## Decisões Abertas para TechSpec

- Biblioteca WebSocket: uWebSockets vs. libwebsockets vs. Boost.Beast? (trade-off: peso de dependência vs. maturidade)
- Autenticação de clientes: sem auth nesta fase ou token simples?
- Como `CommandRouter` acessa `NavalSimulation` thread-safely? (lock por simulação?)
- Frequência padrão de `WorldSnapshot`: todo tick (20 Hz) ou configurável por cliente?
- Porta e configuração do server: hardcoded, env var, arquivo de config?
- Reconexão de cliente: retoma snapshot do estado atual ou começa do zero?
- `Viewer` e `Manager` diferem no que recebem? (Viewer talvez receba todos os ticks; Manager apenas eventos)
