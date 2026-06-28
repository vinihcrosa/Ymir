# Ymir — Testing

## C++ (atual)

**Framework:** Catch2 v3.5.4 (FetchContent)

**7 test executables:**

| Target | Cobre | Arquivos |
|--------|-------|----------|
| `ymir_smoke_tests` | Sanidade geral | `SmokeTest.cpp` |
| `ymir_common_tests` | Math utilities | `TestMath.cpp` |
| `ymir_physics_tests` | Body, ForceModel, Forces | 4 arquivos |
| `ymir_integrator_tests` | CVODE integration loop | `TestIntegrator.cpp` |
| `ymir_simulation_tests` | NavalInfra, coupling, domains | 7 arquivos |
| `ymir_world_tests` | Environment, waves, coupling registry | 6 arquivos |
| `ymir_persistence_tests` | JSON scenario reader | `TestScenarioReader.cpp` |

**Coverage target:** ≥ 80% line coverage (LLVM `llvm-cov`)

**Rodar testes:**
```bash
# Docker
make test

# Nativo
ctest --test-dir build --output-on-failure

# Com coverage
cmake -B build -DYMIR_ENABLE_COVERAGE=ON
cmake --build build --target ymir_coverage
```

**Filtros de coverage:** ignora `build*`, `_deps/`, `tests/`

## Gate checks (C++)

Qualquer task C++ passa quando:
1. `cmake --build build --parallel` sem erros
2. `ctest --test-dir build --output-on-failure` — todos passam
3. Coverage ≥ 80% (verificado em tasks que adicionam código)

## Testes Planejados (pós-migração)

### Integrador RK45 (substitui TestIntegrator.cpp)
- Teste de precisão: problema ODE com solução analítica conhecida (ex: oscilador harmônico)
- Teste de step adaptation: erro local dentro de tolerância `rtol=1e-6, atol=1e-8`
- Teste de regressão: mesmos casos de integração que existiam com CVODE

### WASM smoke test (browser)
- Playwright: carrega página, inicia simulação, verifica que estado avança por 10 steps
- Não substitui testes C++ — testa apenas que o binding funciona no browser

### API (Fastify)
- Framework: Vitest + `fastify.inject()` para rotas (sem HTTP real)
- Cada rota tem teste de: happy path, schema inválido, not found

### Frontend (React)
- Framework: Vitest + React Testing Library
- Componentes críticos testados em isolamento
- Worker não testado unitariamente — coberto pelo teste Playwright
