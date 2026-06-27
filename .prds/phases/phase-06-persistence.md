# Fase 6 — Persistence + Fast-time

## Objetivo

Implementar persistência completa com SQLite e o app `apps/fast-time` para execução batch de simulações aceleradas. Ao final desta fase, runs de simulação são registradas no banco, scenarios são reutilizáveis, e o fast-time executa N simulações em paralelo sem cliente conectado, persistindo resultados consultáveis.

---

## Estado Atual (entrada da fase)

O que já existe:
- JSON scenario reader em `libs/persistence/` (carrega VesselConfig do JSON)
- `World::snapshot()` retorna estado serializável (Fase 2)
- `apps/server` com simulação real-time (Fase 3)
- `EventBus` publicando eventos por tick (Fase 2)

O que falta (esta fase entrega):
- Schema SQLite e repositórios para scenarios, entidades, histórico de steps, logs
- `SQLiteRepository` — acesso ao banco
- `StepWriter` — escrita em lote de histórico de simulação
- `ScenarioStore` — CRUD de scenarios
- `apps/fast-time` — submissão de scenarios, execução isolada por processo, persistência automática
- Telemetria interna (métricas de tick, CPU, memória)

---

## Escopo

### Incluído nesta fase

- Schema SQLite (migrations com versão)
- Repositórios: `ScenarioRepository`, `SimulationRunRepository`, `StepRepository`, `TelemetryRepository`
- `StepWriter` com buffer de escrita em lote
- `apps/fast-time` — executável com runner batch
- Processo filho isolado por simulação fast-time
- API de submissão de fast-time (HTTP ou simples stdin/socket)
- Telemetria: duração de tick, lag de simulação, uso de memória por run
- Migração do JSON scenario reader para SQLite (scenarios criados via API ficam no banco)

### Excluído desta fase

- Interface web para consulta de resultados
- Exportação CSV/HDF5
- Streaming de resultados em tempo real de fast-time (é batch por definição)
- Autenticação avançada
- Compressão de histórico de steps

---

## Módulos

### Schema SQLite

```sql
-- Versioning
CREATE TABLE schema_version (version INTEGER NOT NULL);

-- Scenarios
CREATE TABLE scenarios (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT NOT NULL,
    description TEXT,
    json_blob   TEXT NOT NULL,    -- scenario JSON completo
    created_at  INTEGER NOT NULL  -- unix timestamp
);

-- Simulation runs
CREATE TABLE simulation_runs (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    scenario_id INTEGER NOT NULL REFERENCES scenarios(id),
    mode        TEXT NOT NULL,    -- 'realtime' | 'fasttime'
    started_at  INTEGER NOT NULL,
    ended_at    INTEGER,
    status      TEXT NOT NULL     -- 'running' | 'completed' | 'failed'
);

-- Step history (alto volume)
CREATE TABLE steps (
    run_id      INTEGER NOT NULL REFERENCES simulation_runs(id),
    sim_time    REAL NOT NULL,
    entity_id   INTEGER NOT NULL,
    q0 REAL, q1 REAL, q2 REAL, q3 REAL, q4 REAL, q5 REAL,
    qd0 REAL, qd1 REAL, qd2 REAL, qd3 REAL, qd4 REAL, qd5 REAL,
    PRIMARY KEY (run_id, sim_time, entity_id)
) WITHOUT ROWID;

-- Events (colisões, rupturas, etc.)
CREATE TABLE events (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    run_id      INTEGER NOT NULL REFERENCES simulation_runs(id),
    sim_time    REAL NOT NULL,
    event_type  TEXT NOT NULL,
    payload_json TEXT
);

-- Telemetry
CREATE TABLE telemetry (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    run_id      INTEGER REFERENCES simulation_runs(id),
    ts          INTEGER NOT NULL,
    metric      TEXT NOT NULL,
    value       REAL NOT NULL
);
```

**Índices:**
```sql
CREATE INDEX idx_steps_run_time ON steps(run_id, sim_time);
CREATE INDEX idx_events_run ON events(run_id, sim_time);
```

---

### StepWriter (escrita em lote)

Acumula steps em buffer em memória, faz flush em transação única.

```cpp
class StepWriter {
public:
    explicit StepWriter(SQLiteRepository& db,
                        std::size_t       flush_interval = 100); // steps

    void write(int run_id, double sim_time, int entity_id,
               const BodyState& state);
    void flush(); // chamado no destructor e explicitamente ao final do run
};
```

**Flush strategy:**
- A cada `flush_interval` steps acumulados → `BEGIN TRANSACTION; INSERT ...; COMMIT`
- Ao final da run → flush imediato
- Em caso de crash → steps não flushed são perdidos (aceitável para V1)

---

### ScenarioRepository

```cpp
class ScenarioRepository {
public:
    int64_t save(const Scenario& s);
    Scenario load(int64_t id);
    std::vector<ScenarioSummary> list();
    void     remove(int64_t id);
};
```

`Scenario` inclui JSON completo + metadados. Substituição progressiva do JSON file reader.

---

### SimulationRunRepository

```cpp
class SimulationRunRepository {
public:
    int64_t  start(int64_t scenario_id, const std::string& mode);
    void     end(int64_t run_id, RunStatus status);
    RunStatus status(int64_t run_id);

    // Consulta de histórico
    std::vector<StepRecord> stepsInRange(int64_t run_id,
                                          double t_start, double t_end);
    std::vector<EventRecord> events(int64_t run_id);
};
```

---

### apps/fast-time

**Fluxo:**
```
1. Cliente submete scenario → fast-time server aceita (HTTP ou unix socket)
2. fast-time cria run record no banco
3. fast-time faz fork() ou spawn processo filho
4. Processo filho:
   a. Carrega scenario do banco
   b. Cria NavalSimulation
   c. Loop: step(dt) até sim_time >= duration
   d. Cada N steps → StepWriter.write()
   e. Ao final → StepWriter.flush(), marca run como completed
5. Processo pai continua aceitando novas submissions
```

**Isolamento por processo:**
- `fork()` (Unix) ou `CreateProcess` (Windows) — processo separado por run
- Crash em uma run não afeta outras
- Comunicação pai-filho: apenas código de saída (0 = ok, != 0 = falhou)

**Paralelismo:**
- Máximo N processos filhos simultâneos, configurável (default: `nCPU - 1`)
- Fila de submissions: se > N rodando, fica na fila

**Sem streaming:**
- Fast-time não tem WebSocket
- Resultados consultados depois via `SimulationRunRepository::stepsInRange`

**Interface de submissão (simples — HTTP REST ou socket):**
```
POST /submit   { "scenario_id": 42, "duration_s": 3600 }
→ { "run_id": 17 }

GET /status/17
→ { "status": "running", "progress": 0.45 }

GET /results/17?t_start=100&t_end=200&entity_id=1
→ [ {sim_time, q[12]}, ... ]
```

---

### Telemetria

Métricas internas gravadas periodicamente.

```cpp
class TelemetryRecorder {
public:
    void record(int64_t run_id, const std::string& metric, double value);
    // Chamado após cada tick:
    //   "tick_duration_ms", "cvode_steps", "memory_mb"
};
```

Métricas por tick:
- `tick_duration_ms` — wall time de NavalSimulation::step
- `cvode_steps` — número de sub-passos adaptativos do CVODE no tick
- `memory_mb` — RSS do processo (lido de `/proc/self/status` ou `getrusage`)

Flush: a cada 1000 métricas ou ao final da run.

---

## Acceptance Criteria

1. Scenario salvo via `ScenarioRepository::save` → carregado corretamente via `load`
2. Run registrada com `started_at` e `ended_at` corretos
3. `stepsInRange(run_id, 100, 200)` retorna todos e somente os steps entre t=100s e t=200s
4. Fast-time executa 1 hora de simulação (3600 steps × dt=1s) sem crash
5. Dois fast-time runs do mesmo scenario rodam em paralelo com resultados independentes
6. Crash de processo filho não afeta run do processo pai
7. `tick_duration_ms` disponível em telemetria após cada run
8. Cobertura de testes ≥ 80% (repositórios com SQLite em memória para testes)

---

## Decisões Abertas para TechSpec

- Biblioteca SQLite C++: wrapper próprio (sqlite3.h direto) ou sqlitecpp / sqlite_orm?
- `apps/fast-time` usa fork() ou std::process (C++23)? E no Windows?
- Batch size do `StepWriter`: 100 steps? Configurável?
- Compressão de steps (delta encoding, float32 vs float64) — necessário desde V1?
- API de fast-time: HTTP simples (pistache/crow) ou unix socket binário?
- Schema migration: rodar na startup ou ferramenta separada?
- Steps de entidades que não se moveram — escrever mesmo assim ou delta-only?
