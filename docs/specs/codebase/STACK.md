# Ymir — Stack Atual

## Core (C++)

| Componente | Tecnologia | Versão | Notas |
|-----------|-----------|--------|-------|
| Linguagem | C++ | 17 | `-std=c++17` obrigatório |
| Build system | CMake + Ninja | CMake 3.16+ | |
| Containerização | Docker | Linux x86_64 | Image em `docker/linux-x86_64/` |
| ODE Solver | SUNDIALS CVODE | 6.0+ | **Será removido** — substituído por RK45 |
| JSON | nlohmann_json | 3.11.3 | FetchContent |
| Test framework | Catch2 | v3.5.4 | FetchContent |
| Geometry (futuro) | CGAL | system | Opcional, não usado ainda |

## Compilador

- MSVC: `/W4 /WX /permissive-`
- Clang/GCC: `-Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter`
- Coverage: LLVM (`llvm-cov`) com target ≥ 80%

## Build modes

```bash
# Docker (recomendado)
make build     # compila no container
make test      # roda testes
make shell     # shell interativo

# Nativo
cmake -B build -S . -GNinja -DCMAKE_BUILD_TYPE=Release -DYMIR_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Dependências do sistema (nativo)

- SUNDIALS 6.0+ (`libsundials-dev`)
- CGAL (opcional)
- CMake 3.16+
- Ninja

## Stack Planejada (pós-migração)

| Componente | Tecnologia | Versão |
|-----------|-----------|--------|
| WASM Compiler | Emscripten | latest stable |
| WASM Bindings | Embind | (bundled com Emscripten) |
| Monorepo | pnpm + Turborepo | pnpm 9+, turbo 2+ |
| API Runtime | Node.js | 20 LTS |
| API Framework | Fastify | 4.x |
| API Schema | TypeBox | 0.33+ |
| Database | SQLite + Drizzle ORM | — |
| WebSocket | @fastify/websocket | — |
| Frontend | React + Vite | React 18, Vite 5 |
| Frontend State | Zustand | 4.x |
| Shared Types | TypeBox (packages/types) | — |
