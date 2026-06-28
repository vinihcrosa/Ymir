# Ymir — Convenções

## C++ Naming

| Elemento | Convenção | Exemplo |
|----------|-----------|---------|
| Classes | PascalCase | `RigidBody6DOF`, `NavalForceModel` |
| Funções/métodos | camelCase | `computeForce()`, `integrate()` |
| Membros privados | `m_` prefix + camelCase | `m_state`, `m_config` |
| Constantes | UPPER_SNAKE_CASE | `GRAVITY_ACCELERATION` |
| Headers | PascalCase + `.h` | `BodyState.h`, `ForceModel.h` |
| Implementações | PascalCase + `.cpp` | `RigidBody6DOF.cpp` |

## C++ Include Paths

Prefixo obrigatório matching o contexto:
```cpp
#include "ymir/physics/BodyState.h"        // não "BodyState.h"
#include "ymir/common/Types.h"
#include "ymir/vessel/Thruster.h"
```

## C++ Padrões de Código

- C++17 apenas — sem features C++20
- `std::unique_ptr` / `std::shared_ptr` — sem raw owning pointers
- Constantes nomeadas — sem magic numbers
- Sem I/O em `physics/` e `simulation/`
- `[[nodiscard]]` em funções que retornam valor de erro
- Doxygen em interfaces públicas

## Commits

Conventional Commits com scopes definidos:
```
feat(physics): add Dormand-Prince RK45 integrator
fix(vessel): correct thruster force direction
refactor(simulation): decouple NavalDomain from persistence
test(world): add wave spectrum validation
build(core): add Emscripten CMake toolchain
chore(monorepo): setup pnpm workspaces + Turborepo
```

**Scopes válidos:** `physics`, `world`, `vessel`, `simulation`, `persistence`, `api`, `web`, `types`, `tests`, `build`, `core`, `monorepo`

## Testes C++

- Framework: Catch2 v3
- Um arquivo de teste por módulo de força
- Nomenclatura: `Test{ComponentName}.cpp`
- Localização: `tests/` espelhando `libs/`
- Target de cobertura: ≥ 80% de linhas

## TypeScript (pós-migração)

- `strict: true` em todos os `tsconfig.json`
- TypeBox schemas como source of truth de tipos
- `import type` para imports type-only
- Sem `any` — use `unknown` + narrowing
- Fastify routes usam `RouteShorthandOptions` com TypeBox schema

## Estrutura de Arquivos TypeScript

```
apps/api/src/
├── routes/          # um arquivo por domínio
├── services/        # lógica de negócio
├── db/              # Drizzle schema + migrations
└── plugins/         # Fastify plugins

apps/web/src/
├── components/      # React components
├── stores/          # Zustand stores
├── workers/         # Web Worker scripts
└── lib/             # utilities, wasm loader
```
