# Ymir — Agent Guidelines

Ground rules for AI agents (Claude, Copilot, etc.) working on this codebase.

---

## Project context

Ymir is a general-purpose physics engine written from scratch in C++17. It is designed to simulate the dynamics of physical bodies under arbitrary force systems, and to be extended to any physical domain.

**Current scope: naval domain.** The first implementation covers 6-DOF hydrodynamics for ships and floating platforms. All architecture decisions must remain domain-agnostic — the naval module is one application of the engine, not the engine itself.

There is a reference implementation in `docs/legacy/` (legacy naval project). Use it to understand domain behavior and physics intent — do not copy its code structure or repeat its known problems.

---

## Architecture overview

See [docs/architecture/README.md](docs/architecture/README.md) for the full architecture wiki, including the repository structure, bounded-context map, and ADR index.

See [docs/architecture/bounded-contexts.md](docs/architecture/bounded-contexts.md) for the authoritative table of bounded contexts, CMake targets, include prefixes, and dependency rules.

### Repository layout

```
ymir/
├── apps/
│   ├── server/        real-time server: WebSocket, Protobuf, API handlers
│   └── fast-time/     batch accelerated: no live clients
├── libs/
│   ├── common/        ymir_common — math utilities, physical constants, shared types
│   ├── physics/       ymir_physics — bodies, force modules, CVODE integrator
│   ├── simulation/    ymir_simulation — tick orchestration, event system
│   ├── world/         ymir_world — wave engine, environment, terrain state
│   ├── vessel/        ymir_vessel — vessel config, control modes, actuator state
│   └── persistence/   ymir_persistence — JSON scenario reader (SQLite planned)
├── tests/             mirrors libs/ structure
└── docs/
    ├── architecture/  this wiki
    ├── guides/        building.md, contributing.md
    └── adr/           Architecture Decision Records
```

### Bounded Contexts

Each bounded context is one CMake static library. The linker enforces dependency rules — no target may link to a context it should not depend on (ADR-003). Include prefix, directory name, and CMake target name are all identical by design (ADR-002).

| Context | CMake target | Include prefix | Directory | Dependencies |
|---------|-------------|----------------|-----------|-------------|
| Common | `ymir_common` | `ymir/common/` | `libs/common/` | none |
| Physics | `ymir_physics` | `ymir/physics/` | `libs/physics/` | `ymir_common`, `SUNDIALS::cvode` |
| Simulation | `ymir_simulation` | `ymir/simulation/` | `libs/simulation/` | `ymir_physics`, `ymir_common` |
| World | `ymir_world` | `ymir/world/` | `libs/world/` | `ymir_physics`, `ymir_common` |
| Vessel | `ymir_vessel` | `ymir/vessel/` | `libs/vessel/` | `ymir_physics`, `ymir_common` |
| Persistence | `ymir_persistence` | `ymir/persistence/` | `libs/persistence/` | `ymir_physics`, `ymir_world`, `ymir_vessel`, `nlohmann_json` |

### Architecture rules

- No context may `#include` a header from a context it does not depend on
- `libs/physics/` must have zero knowledge of I/O, file formats, or rendering
- All external dependencies (JSON, CSV, SUNDIALS, CGAL) live in `libs/persistence/` or adapter apps, not in `libs/physics/` or `libs/common/`
- The C API in `include/` is the only public surface — everything else is internal

### C API boundary

- No STL types cross the DLL boundary (`std::vector`, `std::string`, etc.)
- No C++ exceptions cross the DLL boundary — functions return error codes
- All structs at the boundary must be plain C (`sizeof` must be deterministic)
- Calling convention: `extern "C"`, no `__cdecl` attribute needed (use `CallingConvention.Cdecl` on .NET side)

### Physics core

- No heap allocation inside the integration loop (pre-allocate in constructors)
- No global mutable state — every simulation is an isolated object
- Force modules: pure functions of state — receive `const BodyState&`, return `Forces`
- CVODE memory owned by `BodyDynamics`, cleaned up in destructor

---

## Include conventions

Use the bounded-context include prefix. Examples:

```cpp
// Common math and types
#include <ymir/common/math/LinearAlgebra.h>
#include <ymir/common/math/AngleUtils.h>
#include <ymir/common/Types.h>
#include <ymir/common/PhysicalConstants.h>

// Physics — body state, force model, integrator
#include <ymir/physics/BodyState.h>
#include <ymir/physics/ForceModel.h>
#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/physics/forces/CurrentForces.h>
#include <ymir/physics/NavalForceModel.h>

// Simulation
#include <ymir/simulation/Simulation.h>

// World
#include <ymir/world/World.h>
#include <ymir/world/Environment.h>

// Vessel
#include <ymir/vessel/VesselConfig.h>

// Persistence
#include <ymir/persistence/json/ScenarioReader.h>
```

---

## Coding standards

- C++17 — use `std::optional`, structured bindings, `if constexpr` where appropriate
- SOLID — single responsibility per class, no god objects
- No raw owning pointers — use `std::unique_ptr` / `std::shared_ptr`
- No `using namespace std` in headers
- Doxygen on every public symbol (`/** */` style)
- No magic numbers — named constants or `constexpr`

---

## What to avoid (lessons from legacy)

| Problem | Rule |
|---------|------|
| Global state in wave engine (`g_waveData`, `g_components`) | All state owned by instance objects |
| `rand()` without seed — deterministic "random" phases | Use `std::mt19937` with explicit seed; expose seed as parameter |
| `wave_engine_prune` corrupting indices on 2nd call | Rebuild index after any mutation of the component list |
| Inconsistent angle normalization across modules | One `AngleUtils` namespace, used everywhere |
| Off-diagonal hydrostatic stiffness silently ignored | Either implement fully or document explicitly in code |
| Small-angle approximation for roll/pitch undocumented | Document assumptions with a `// ASSUMPTION:` comment |
| Mixing coordinate origins in restoring force calc | Each formula must name its reference frame in a comment |
| CSV output coupled to physics loop | `OutputWriter` is an adapter — physics loop calls an observer interface |

---

## Testing

### Regra global — toda feature deve ser testada

**Cobertura mínima obrigatória: 80%** em linhas, funções, branches e statements.  
Nenhuma feature é considerada completa sem testes que satisfaçam este limiar.

### C++ (libs/physics, libs/simulation, libs/vessel, …)

- Every force module has a unit test before it is integrated into `RHS`
- Unit tests use analytical solutions or known limits (e.g., zero current → zero current force)
- Integration tests run a full simulation and assert on conserved quantities or known maneuver behavior
- Test files mirror source structure: `tests/physics/forces/TestCurrentForces.cpp`
- Test executables are defined in `tests/CMakeLists.txt` and linked via `Ymir::*` targets
- Coverage measured via `-DYMIR_ENABLE_COVERAGE=ON` (gcov/llvm-cov)

### Frontend (apps/web)

Dois níveis de teste, ambos obrigatórios para features de UI:

| Nível | Ferramenta | Localização | O que testa |
|-------|-----------|-------------|-------------|
| Unit / Component | **Vitest** + Testing Library | `src/**/*.test.{ts,tsx}` | Lógica pura, hooks, stores, componentes isolados |
| E2E / Flow | **Playwright** | `e2e/**/*.spec.ts` | Fluxos completos de usuário no browser |

**Comandos:**

```bash
# Unit tests + cobertura (enforces 80% threshold — falha abaixo)
pnpm --filter @ymir/web test

# E2E (requer servidor rodando ou usa webServer do playwright.config.ts)
pnpm --filter @ymir/web test:e2e

# Modo watch (desenvolvimento)
pnpm --filter @ymir/web test:watch
```

**Regras de cobertura:**

- Thresholds configurados em `apps/web/vitest.config.ts` — `lines: 80, functions: 80, branches: 80`
- `pnpm test` falha se qualquer threshold não for atingido (bloqueante em CI)
- Excluções válidas: `src/main.tsx`, arquivos `*.d.ts`, barris `index.ts` sem lógica
- Mocks de módulos externos (Leaflet, WASM) são aceitos e não contam contra cobertura

**Playwright:**

- Configuração em `apps/web/playwright.config.ts`
- Browser: Chromium (headless em CI, headed em dev)
- `webServer` inicia `vite dev` automaticamente antes dos testes
- Toda feature nova deve ter ao menos um teste E2E cobrindo o happy path

### Backend (apps/api)

- Testes unitários de serviços e repositórios: a definir quando volume justificar framework
- Enquanto não configurado: testar manualmente via `curl` e documentar no `SUMMARY.md` da task

---

## Documentation

**`docs/` is the single source of truth for all project knowledge. No other folder.**

| Content type | Location |
| --- | --- |
| Architecture Decision Records | `docs/adr/<phase>/adr-NNN.md` |
| Architecture wiki | `docs/architecture/` |
| PRDs (product requirements, roadmap, modules) | `docs/planning/prds/` |
| Feature planning (tech spec, task breakdown) | `docs/planning/<feature>/` |
| Codebase specs (architecture, conventions, stack) | `docs/specs/codebase/` |
| Feature specs | `docs/specs/features/<feature>/` |
| Project specs (roadmap, state) | `docs/specs/project/` |
| Web API & backend conventions | `docs/conventions/` |
| Public API docs (Doxygen output) | `docs/api/` |
| Legacy reference implementation docs | `docs/legacy/` |
| Design discussions and open questions | `docs/discussions/` |

Rules:

- Every significant design decision → ADR in `docs/adr/`
- Every new feature → PRD + tech spec in `docs/planning/<feature>/` before any code
- Do **not** create hidden documentation folders (`.compozy/`, `.specs/`, or similar)
- Do not store design decisions in conversation memory or agent scratch files — commit them to `docs/`
- No inline `// TODO` without a linked issue or a `// REASON:` explanation

---

## AI-assisted development

### Context hygiene (lazy loading)

AI agents have limited context windows. Load only what the current task requires.

- **Start narrow**: read `AGENTS.md` + the specific bounded-context headers you will touch — nothing else
- **Load on demand**: only `#include` or read a file when a task concretely requires it; do not preload the full tree
- **One context per concern**: do not mix architecture review, test writing, and implementation in a single agent turn — split them
- **Anchor before expanding**: identify the exact files and symbols to change before opening related files for context
- **Avoid re-reading**: if a file was already read in the current session, do not read it again unless its content changed

### Documentation discipline

- Every non-trivial decision made during an AI session → ADR in `docs/adr/` before closing the session
- PRD and tech spec must exist in `docs/planning/<feature>/` **before** any code is written
- Do not rely on conversation history as a design record — it is ephemeral
- Agent scratchpad / memory files are temporary; promote durable conclusions to `docs/`

### Scope control

- One PR per logical change — do not bundle unrelated fixes with a feature
- If a task grows beyond the original scope, stop, document the new scope in `docs/planning/`, and restart
- Never silently refactor code outside the stated task scope

### Verification before commit

- Run the affected test suite (`ctest -R <module>`) before marking a task complete
- Check for regressions in dependent bounded contexts (see dependency table above)
- Confirm Doxygen compiles without errors on touched public headers

### Naming consistency

- File names, CMake targets, include prefixes, and namespace names must all match the bounded-context name — no one-offs
- When an ADR introduces a rename, update all four in the same commit

---

## Versioning

Ymir follows **Semantic Versioning** ([semver.org](https://semver.org)):

```text
MAJOR.MINOR.PATCH
```

| Bump    | When                                                          |
| ------- | ------------------------------------------------------------- |
| `MAJOR` | Breaking change to the public C API or physics model contract |
| `MINOR` | New capability added in a backward-compatible way             |
| `PATCH` | Bug fix, doc update, internal refactor — no API change        |

Version is declared in `CMakeLists.txt` (`project(Ymir VERSION x.y.z)`) and exposed in the C API header.

---

## Web Stack Conventions

The monorepo includes a Node.js API and a React frontend. Before writing any code in `apps/` or `packages/`, read the relevant convention file:

- **Backend** (`apps/api`): [docs/conventions/backend.md](docs/conventions/backend.md)
- **Frontend** (`apps/web`): [docs/conventions/frontend.md](docs/conventions/frontend.md)

Key rules that apply to both:

- TypeBox is the **only** validation library — no Zod, Yup, or manual checks
- Every request schema must be declared on the Fastify route (body, params, querystring)
- Layered architecture: Route → Service → Repository → DB (no layer skipping)
- File size limit: ~200 lines — split if exceeded
- `import type` for type-only imports
- No `any` without a documented reason
- No `console.log` in production code

---

## Commit discipline

Ymir follows **Conventional Commits** ([conventionalcommits.org](https://www.conventionalcommits.org)):

```text
<type>(<scope>): <short description>

[optional body]

[optional footer: BREAKING CHANGE: ...]
```

### Types

| Type        | Use                                         |
| ----------- | ------------------------------------------- |
| `feat`      | New feature (bumps MINOR)                   |
| `fix`       | Bug fix (bumps PATCH)                       |
| `docs`      | Documentation only                          |
| `test`      | Adding or fixing tests                      |
| `refactor`  | Code change that is neither fix nor feat    |
| `perf`      | Performance improvement                     |
| `build`     | Build system, CMake, dependencies           |
| `ci`        | CI/CD pipeline changes                      |
| `chore`     | Maintenance — no production code change     |

### Scopes

| Scope        | What it covers                                  |
| ------------ | ----------------------------------------------- |
| `common`     | Shared math, types, physical constants          |
| `physics`    | Rigid-body dynamics, force modules, integrator  |
| `simulation` | Tick orchestration, event system                |
| `world`      | Wave engine, environment, terrain state         |
| `vessel`     | Vessel configuration, control modes             |
| `persistence`| I/O adapters (JSON, SQLite)                     |
| `api`        | apps/api — Fastify backend                      |
| `web`        | apps/web — React frontend                       |
| `types`      | packages/types — shared TypeBox schemas         |
| `wasm`       | Emscripten build, YmirBindings.cpp              |
| `tests`      | Test infrastructure                             |
| `build`      | CMake, Docker, toolchain                        |

### Rules

- One logical change per commit
- Breaking C API or ABI changes require `BREAKING CHANGE:` footer
- No commented-out code in commits
- Tests must pass before committing to `main`

### Examples

```text
feat(physics): add current force model for cross-flow drag

fix(physics): correct inverted sign in restoring force heave term

refactor(api): replace raw pointer with unique_ptr in BodyDynamics

BREAKING CHANGE: realtime_create() now returns a typed handle, not void*
```
