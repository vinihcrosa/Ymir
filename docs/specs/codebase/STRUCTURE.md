# Ymir — Estrutura Atual

## Diretório Raiz (atual)

```
/Users/viniciusrosa/Documents/GitHub/Ymir/
├── libs/                    # 6 bounded contexts
│   ├── common/              # ymir_common (INTERFACE, headers only)
│   ├── physics/             # ymir_physics (STATIC, 14 .cpp + 23 headers)
│   ├── simulation/          # ymir_simulation (STATIC, 3 .cpp + 4 headers)
│   ├── world/               # ymir_world (STATIC, 6 .cpp + 9 headers)
│   ├── vessel/              # ymir_vessel (STATIC, 7 .cpp + 10 headers)
│   └── persistence/         # ymir_persistence (STATIC, 1 .cpp + 3 headers)
├── apps/                    # Stubs (sem implementação)
│   ├── server/              # ymir_server_app (INTERFACE)
│   └── fast-time/           # ymir_fast_time_app (INTERFACE)
├── tests/                   # 35 arquivos de teste, espelha libs/
├── docs/                    # Architecture wiki + ADRs
├── cmake/                   # Dependencies.cmake, CompilerWarnings.cmake
├── .docs/                   # Legacy reference docs (não copiar)
├── .prds/                   # Roadmap original (referência)
├── docker/                  # Dockerfiles linux-x86_64
├── CMakeLists.txt           # Root CMake
├── Makefile                 # Recipes Docker
└── docker-compose.yml
```

## Estrutura Planejada (pós-migração)

```
/Users/viniciusrosa/Documents/GitHub/Ymir/
├── core/                    # C++ (movido de raiz)
│   ├── libs/                # 6 bounded contexts (inalterados)
│   ├── tests/               # Testes C++ (inalterados)
│   ├── cmake/               # CMake modules
│   ├── docs/
│   ├── CMakeLists.txt       # Root CMake do core
│   └── Makefile             # Build recipes (nativo + WASM)
├── apps/
│   ├── api/                 # Node.js + Fastify
│   │   ├── src/
│   │   │   ├── routes/
│   │   │   ├── services/
│   │   │   ├── db/          # Drizzle schema + migrations
│   │   │   └── plugins/
│   │   ├── package.json
│   │   └── tsconfig.json
│   └── web/                 # React + Vite
│       ├── src/
│       │   ├── components/
│       │   ├── stores/
│       │   ├── workers/     # simulation.worker.ts
│       │   └── lib/
│       ├── public/
│       │   └── wasm/        # ← artefatos WASM copiados aqui
│       ├── package.json
│       └── vite.config.ts
├── packages/
│   └── types/               # TypeBox schemas compartilhados
│       ├── src/
│       │   ├── vessel.ts
│       │   ├── scenario.ts
│       │   ├── simulation.ts
│       │   └── index.ts
│       └── package.json
├── .specs/                  # Este diretório
├── .prds/                   # Roadmap original (referência)
├── turbo.json
├── pnpm-workspace.yaml
└── package.json             # Root workspace
```

## Contagem de Arquivos Atual

| Categoria | Quantidade |
|-----------|-----------|
| Headers C++ | 39 |
| Implementações C++ | 25 |
| Arquivos de teste | 35 |
| CMakeLists.txt | 9 |
| Total fontes | ~108 |
