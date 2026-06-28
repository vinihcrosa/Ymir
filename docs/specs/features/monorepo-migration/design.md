# Monorepo Migration — Design

**Spec:** `.specs/features/monorepo-migration/spec.md`
**Status:** Draft — Aguardando aprovação do usuário

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│ Monorepo Root (pnpm workspaces + Turborepo)                         │
│                                                                     │
│  ┌───────────────┐    ┌──────────────────┐    ┌─────────────────┐   │
│  │  core/        │    │  apps/api/       │    │  apps/web/      │   │
│  │  C++ → WASM   │    │  Fastify + TS    │    │  React + Vite   │   │
│  │               │───▶│                  │◀───│                 │   │
│  │  CMake +      │    │  SQLite/Drizzle  │    │  Zustand        │   │
│  │  Emscripten   │    │  TypeBox         │    │  Web Worker     │   │
│  └───────┬───────┘    └──────────────────┘    └────────┬────────┘   │
│          │ build:wasm                                   │           │
│          │ copia artefatos                             │            │
│          ▼                                             │            │
│  apps/web/public/wasm/ ◀──────────────────────────────┘             │
│                                                                     │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │  packages/types/  (TypeBox schemas — VesselDTO, ScenarioDTO)  │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

### Fluxo de runtime no browser

```
Browser Main Thread
│
├── carrega React app (Vite)
│   └── monta SimulationWorker
│
└── Web Worker (simulation.worker.ts)
    │
    ├── fetch('/wasm/ymir.js') → inicializa módulo Emscripten
    ├── postMessage({ type: 'ready' }) → main thread
    │
    └── on({ type: 'start' })
        └── loop: step() → postMessage({ type: 'state', data: SimulationState })
```

---

## Tech Decisions

| Decisão | Escolha | Rationale |
|---------|---------|-----------|
| ODE Solver | Dormand-Prince RK45 puro C++ | SUNDIALS não compila com Emscripten sem setup complexo; naval dynamics com dt típico (0.01-0.1s) não é stiff o suficiente para exigir BDF |
| WASM bindings | Emscripten Embind | AGENTS.md já exige C API boundary; Embind gera `.d.ts` automaticamente com `--emit-tsd`; alternativa (manual JS glue) é mais frágil |
| WASM threading | Web Worker + postMessage | SharedArrayBuffer requer COOP/COEP em todos os servidores; postMessage é suficiente para 10-60Hz e evita overhead de configuração no dev server |
| Monorepo tool | pnpm + Turborepo | NX não tem suporte nativo a C++/CMake; Turborepo pipeline é suficiente para 3 apps; pnpm workspaces gerencia dependências JS |
| API framework | Fastify v4 + TypeBox | TypeBox schemas = validação runtime + tipos TS sem camada extra; integra direto no sistema de schema do Fastify; 30-40% mais rápido que Express |
| Database | SQLite + Drizzle ORM | Volume baixo, read-heavy, sem servidor externo; alinha com "future SQLite" mencionado no AGENTS.md; Drizzle ORM é TypeScript-first |
| State management | Zustand | Leve, zero boilerplate, adequado para estado técnico de simulação; Redux overhead não justificado |
| WASM em dev | Cópia estática para public/wasm | Vite serve como asset estático; evita `vite-plugin-wasm` que tem problemas em Worker context |

---

## Component 1: Integrador RK45 (core/libs/physics)

- **Purpose:** Substituir `CvodeIntegrator` por implementação pura C++ sem dependências externas
- **Location:** `core/libs/physics/src/integrator/RK45Integrator.cpp` + `.h`
- **Interface:** Mantém `IIntegrator` existente — zero mudança nos callers

```cpp
// Interface existente (não muda)
class IIntegrator {
public:
    virtual void step(RealType t, RealType dt, BodyState& state,
                      const ForceCallback& forces) = 0;
    virtual void setTolerances(RealType rtol, RealType atol) = 0;
    virtual ~IIntegrator() = default;
};

// Nova implementação
class RK45Integrator : public IIntegrator {
    // Dormand-Prince: 6 stages, 4th+5th order error estimator
    // Adaptive step: halve dt quando erro > atol + rtol*|y|
    // Sem heap allocation no hot path — buffers k1..k7 são membros
    RealType m_rtol{1e-6}, m_atol{1e-8};
    Vector6 m_k1, m_k2, m_k3, m_k4, m_k5, m_k6, m_k7; // pré-alocados
};
```

- **Dependencies:** `ymir_common` (Vector6, RealType)
- **Reuses:** `IIntegrator` interface existente; `CvodeConfig.h` pode ser adaptado para `RK45Config.h`
- **Remove:** `CvodeIntegrator.h/cpp`, dependência SUNDIALS em `cmake/Dependencies.cmake`

---

## Component 2: Emscripten Build Target (core/CMakeLists.txt)

- **Purpose:** Adicionar target WASM ao CMake existente sem quebrar target nativo
- **Location:** `core/CMakeLists.txt` + `core/cmake/Emscripten.cmake`

```cmake
# core/cmake/Emscripten.cmake — só ativado quando CMAKE_TOOLCHAIN_FILE=Emscripten

if(EMSCRIPTEN)
    add_executable(ymir_wasm src/wasm/YmirBindings.cpp)
    target_link_libraries(ymir_wasm PRIVATE ymir_simulation ymir_persistence)
    target_link_options(ymir_wasm PRIVATE
        -lembind
        --emit-tsd ymir.d.ts
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
        -sENVIRONMENT=web,worker
        -sINITIAL_MEMORY=67108864   # 64MB
        -sALLOW_MEMORY_GROWTH=1
        -sEXPORT_NAME=YmirModule
        -O2
    )
    # Copia artefatos para apps/web/public/wasm/
    add_custom_command(TARGET ymir_wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/ymir_wasm.wasm
            ${CMAKE_SOURCE_DIR}/../apps/web/public/wasm/ymir.wasm
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/ymir_wasm.js
            ${CMAKE_SOURCE_DIR}/../apps/web/public/wasm/ymir.js
    )
endif()
```

- **Build command WASM:**
```bash
emcmake cmake -B build-wasm -S . -DCMAKE_BUILD_TYPE=Release -DYMIR_BUILD_TESTS=OFF
cmake --build build-wasm --target ymir_wasm --parallel
```

- **Build nativo (sem mudança):**
```bash
cmake -B build -S . -GNinja -DCMAKE_BUILD_TYPE=Release -DYMIR_BUILD_TESTS=ON
```

---

## Component 3: WASM Bindings (core/src/wasm/YmirBindings.cpp)

- **Purpose:** Expor surface mínima do motor para JavaScript via Embind
- **Location:** `core/src/wasm/YmirBindings.cpp`

```cpp
#include <emscripten/bind.h>
#include "ymir/simulation/NavalSimulation.h"
#include "ymir/persistence/json/ScenarioReader.h"

using namespace emscripten;

// Wrapper leve — não expõe internals, só operações de ciclo de vida
class SimulationHandle {
    std::unique_ptr<NavalSimulation> m_sim;
public:
    void loadScenario(const std::string& json) {
        auto scenario = ScenarioReader::fromJson(json);
        m_sim = std::make_unique<NavalSimulation>(std::move(scenario));
    }
    void step(double dt) { m_sim->step(dt); }
    std::string getStateJson() const { return m_sim->serializeStateJson(); }
    void reset() { m_sim->reset(); }
};

EMSCRIPTEN_BINDINGS(ymir) {
    class_<SimulationHandle>("SimulationHandle")
        .constructor()
        .function("loadScenario", &SimulationHandle::loadScenario)
        .function("step", &SimulationHandle::step)
        .function("getStateJson", &SimulationHandle::getStateJson)
        .function("reset", &SimulationHandle::reset);
}
```

**Nota:** `serializeStateJson()` precisa ser adicionado em `NavalSimulation` — retorna JSON string do `BodyState` de todos os vessels. Esta é a única adição necessária ao motor existente.

---

## Component 4: Web Worker (apps/web/src/workers/simulation.worker.ts)

- **Purpose:** Carregar e executar o módulo WASM fora da main thread
- **Location:** `apps/web/src/workers/simulation.worker.ts`

```typescript
import type { SimulationHandle } from '../../../public/wasm/ymir'

let sim: SimulationHandle | null = null

// Carrega módulo WASM dinamicamente
async function init() {
    const YmirModule = await import('/wasm/ymir.js')
    const module = await YmirModule.default()
    sim = new module.SimulationHandle()
    self.postMessage({ type: 'ready' })
}

self.onmessage = (e: MessageEvent) => {
    const { type, payload } = e.data
    switch (type) {
        case 'load':
            sim?.loadScenario(JSON.stringify(payload))
            break
        case 'start':
            startLoop()
            break
        case 'stop':
            stopLoop()
            break
        case 'reset':
            sim?.reset()
            break
    }
}

let loopId: ReturnType<typeof setInterval> | null = null

function startLoop() {
    const DT = 0.05 // 20Hz
    loopId = setInterval(() => {
        if (!sim) return
        sim.step(DT)
        const state = JSON.parse(sim.getStateJson())
        self.postMessage({ type: 'state', data: state })
    }, DT * 1000)
}

function stopLoop() {
    if (loopId) clearInterval(loopId)
}

init()
```

---

## Component 5: packages/types (TypeBox schemas)

- **Purpose:** Source of truth para DTOs compartilhados entre API e Web
- **Location:** `packages/types/src/`

```typescript
// packages/types/src/vessel.ts
import { Type, Static } from '@sinclair/typebox'

export const VesselDTO = Type.Object({
    id: Type.String({ format: 'uuid' }),
    name: Type.String(),
    mass: Type.Number({ minimum: 0 }),
    length: Type.Number({ minimum: 0 }),
    beam: Type.Number({ minimum: 0 }),
    draft: Type.Number({ minimum: 0 }),
    // coeficientes hidrodinâmicos
    Cx: Type.Number(),
    Cy: Type.Number(),
    Cn: Type.Number(),
})

export type VesselDTO = Static<typeof VesselDTO>

// packages/types/src/simulation.ts
export const SimulationStateDTO = Type.Object({
    t: Type.Number(),            // tempo de simulação (s)
    vessels: Type.Array(Type.Object({
        id: Type.String(),
        x: Type.Number(),        // posição N (m)
        y: Type.Number(),        // posição E (m)
        z: Type.Number(),        // heave (m)
        phi: Type.Number(),      // roll (rad)
        theta: Type.Number(),    // pitch (rad)
        psi: Type.Number(),      // heading (rad)
        u: Type.Number(),        // velocidade surge (m/s)
        v: Type.Number(),        // velocidade sway (m/s)
        r: Type.Number(),        // yaw rate (rad/s)
    }))
})

export type SimulationStateDTO = Static<typeof SimulationStateDTO>
```

---

## Component 6: API Fastify (apps/api)

- **Purpose:** Servir vessels, cenários, e streaming de estado via WebSocket
- **Location:** `apps/api/src/`

```typescript
// apps/api/src/routes/vessels.ts
import { FastifyInstance } from 'fastify'
import { VesselDTO } from '@ymir/types'
import { Type } from '@sinclair/typebox'

export async function vesselRoutes(fastify: FastifyInstance) {
    fastify.get('/vessels', {
        schema: { response: { 200: Type.Array(VesselDTO) } }
    }, async () => {
        return db.select().from(vessels)  // Drizzle ORM
    })

    fastify.get<{ Params: { id: string } }>('/vessels/:id', {
        schema: { params: Type.Object({ id: Type.String() }) }
    }, async (request, reply) => {
        const vessel = await db.select().from(vessels).where(eq(vessels.id, request.params.id)).get()
        if (!vessel) return reply.status(404).send({ error: 'Not found' })
        return vessel
    })
}
```

**Estrutura de rotas:**
- `GET /health` — healthcheck
- `GET /vessels` — lista vessels
- `GET /vessels/:id` — vessel por ID
- `POST /vessels` — criar vessel
- `GET /scenarios` — lista cenários
- `GET /scenarios/:id` — cenário por ID
- `POST /scenarios` — criar cenário
- `WS /ws/simulation` — streaming de estado (M1)

---

## Component 7: Turborepo Pipeline

```json
// turbo.json
{
    "$schema": "https://turbo.build/schema.json",
    "pipeline": {
        "core#build:wasm": {
            "outputs": ["../apps/web/public/wasm/**"],
            "inputs": ["libs/**/*.cpp", "libs/**/*.h", "src/wasm/**"]
        },
        "build": {
            "dependsOn": ["core#build:wasm", "^build"],
            "outputs": ["dist/**"]
        },
        "test": {
            "dependsOn": ["build"]
        },
        "dev": {
            "cache": false,
            "persistent": true
        },
        "typecheck": {
            "dependsOn": ["^build"]
        }
    }
}
```

```yaml
# pnpm-workspace.yaml
packages:
  - 'apps/*'
  - 'packages/*'
  # core não é workspace JS — CMake gerencia o build
```

**Nota sobre `core/`:** O core C++ não é um pnpm workspace — seu build é gerenciado pelo CMake/Emscripten. O `core#build:wasm` é um pipeline task customizado que chama um script shell que invoca `emcmake cmake`.

```json
// core/package.json (só para integração com Turborepo)
{
    "name": "@ymir/core",
    "scripts": {
        "build:wasm": "bash scripts/build-wasm.sh"
    }
}
```

---

## Data Models

### Drizzle Schema (SQLite)

```typescript
// apps/api/src/db/schema.ts
import { text, real, sqliteTable } from 'drizzle-orm/sqlite-core'
import { createId } from '@paralleldrive/cuid2'

export const vessels = sqliteTable('vessels', {
    id: text('id').primaryKey().$defaultFn(() => createId()),
    name: text('name').notNull(),
    mass: real('mass').notNull(),
    length: real('length').notNull(),
    beam: real('beam').notNull(),
    draft: real('draft').notNull(),
    Cx: real('cx').notNull(),
    Cy: real('cy').notNull(),
    Cn: real('cn').notNull(),
    createdAt: text('created_at').$defaultFn(() => new Date().toISOString()),
})

export const scenarios = sqliteTable('scenarios', {
    id: text('id').primaryKey().$defaultFn(() => createId()),
    name: text('name').notNull(),
    description: text('description'),
    configJson: text('config_json').notNull(), // JSON do cenário completo
    createdAt: text('created_at').$defaultFn(() => new Date().toISOString()),
})
```

---

## Code Reuse Analysis

| Componente existente | Localização | Como reutilizar |
|---------------------|-------------|-----------------|
| `IIntegrator` interface | `libs/physics/integrator/CvodeIntegrator.h` | Extrair interface pura → `IIntegrator.h`; `RK45Integrator` implementa |
| `NavalSimulation` | `libs/simulation/NavalSimulation.h` | Adicionar `serializeStateJson()` apenas |
| `ScenarioReader` | `libs/persistence/json/ScenarioReader.h` | Reutilizar sem mudança nos bindings WASM |
| Todos os `ForceModel`s | `libs/physics/forces/` | Zero mudança — compilam com Emscripten |
| `Types.h`, `PhysicalConstants.h` | `libs/common/` | Zero mudança |
| Todos os 35 testes | `tests/` | Rodam no target nativo sem mudança |
| `nlohmann_json` | FetchContent | Continua para persistence; WASM usa `getStateJson()` string |

---

## Error Handling Strategy

| Cenário | Handling | Impacto no usuário |
|---------|----------|-------------------|
| WASM falha ao carregar (rede) | Worker postMessage `{ type: 'error', code: 'WASM_LOAD_FAILED' }` | Frontend exibe banner de erro |
| WASM sem SharedArrayBuffer | Worker detecta, postMessage `{ type: 'error', code: 'MISSING_COOP_COEP' }` | Frontend exibe instrução de configuração |
| `step()` gera NaN/Inf | SimulationHandle::step valida output antes de serializar | Simulação para, frontend exibe estado inválido |
| API offline | Frontend fetch com retry 3x, fallback para estado de erro | Painel mostra "API indisponível" |
| Schema inválido na API | Fastify retorna 400 com erros TypeBox formatados | Resposta JSON com array de erros |
| SQLite corrompido na primeira run | API cria novo database + aplica seed | Transparente para usuário |

---

## Estrutura Final de Diretórios

```
ymir/
├── core/                           # C++ (movido)
│   ├── libs/                       # 6 bounded contexts (inalterados)
│   ├── tests/                      # Testes C++ (inalterados)
│   ├── cmake/
│   │   ├── Dependencies.cmake      # Remove SUNDIALS, mantém nlohmann + Catch2
│   │   ├── CompilerWarnings.cmake
│   │   └── Emscripten.cmake        # NOVO — targets WASM
│   ├── src/wasm/
│   │   └── YmirBindings.cpp        # NOVO — Embind surface
│   ├── scripts/
│   │   └── build-wasm.sh           # NOVO — invoca emcmake cmake
│   ├── CMakeLists.txt
│   └── package.json                # NOVO — para integração Turborepo
├── apps/
│   ├── api/
│   │   ├── src/
│   │   │   ├── routes/             # vessels.ts, scenarios.ts, health.ts
│   │   │   ├── services/
│   │   │   ├── db/                 # schema.ts, migrations/
│   │   │   └── app.ts              # Fastify instance
│   │   ├── package.json
│   │   └── tsconfig.json
│   └── web/
│       ├── src/
│       │   ├── components/
│       │   ├── stores/             # simulationStore.ts (Zustand)
│       │   ├── workers/            # simulation.worker.ts
│       │   └── lib/
│       ├── public/
│       │   └── wasm/               # ymir.wasm, ymir.js (gerado)
│       ├── package.json
│       └── vite.config.ts          # headers COOP/COEP para dev server
├── packages/
│   └── types/
│       ├── src/
│       │   ├── vessel.ts
│       │   ├── scenario.ts
│       │   ├── simulation.ts
│       │   └── index.ts
│       └── package.json
├── .specs/                         # Este diretório
├── turbo.json
├── pnpm-workspace.yaml
└── package.json
```

---

## Vite Config (COOP/COEP para SharedArrayBuffer)

Embora a decisão seja usar `postMessage` ao invés de SharedArrayBuffer, os headers são necessários para outros contextos WASM e são boa prática:

```typescript
// apps/web/vite.config.ts
export default defineConfig({
    plugins: [react()],
    server: {
        headers: {
            'Cross-Origin-Opener-Policy': 'same-origin',
            'Cross-Origin-Embedder-Policy': 'require-corp',
        }
    },
    worker: {
        format: 'es'
    }
})
```

---

## Risks e Mitigações

| Risco | Probabilidade | Impacto | Mitigação |
|-------|--------------|---------|-----------|
| RK45 insuficiente para cenários stiff (molas rígidas em BerthManeuver) | Baixa | Alto | Interface `IIntegrator` permite plugar CVODE de volta no target nativo se necessário |
| Emscripten gera WASM muito grande (> 10MB) | Média | Médio | Link-time optimization (`-O2`), tree-shake com `EMSCRIPTEN_FORCE_LONG_DOUBLE=0` |
| `postMessage` a 20Hz causa latência perceptível | Baixa | Baixo | Aumentar DT ou usar Transferable objects para arrays grandes |
| Drizzle ORM mudanças breaking | Baixa | Baixo | Pinnar versão; a abstração SQL-like facilita migração |
| nlohmann_json no hot path (getStateJson) | Média | Médio | Pré-alocar buffer; em M1 avaliar serialização binária (msgpack) |
