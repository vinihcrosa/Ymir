# Task Memory: task_09.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Refatorar NavalSimulation de single-body para N corpos. Breaking change: construtor único `NavalSimulation()`, método `addBody(id, body)`, `addNavalForceModel(int bodyId, ...)`, `registerVessel(int bodyId, ...)`, `state(int bodyId)` (throws out_of_range). Migrar testes existentes. Criar TestVesselIntegration.cpp.

## Status

COMPLETE. All 149 tests pass (0 failures).

## Important Decisions

- `buildContext` assinatura mudou para `buildContext(int id, const BodyEntry& entry, double dt) const` — ID passado explicitamente porque `BodyEntry` não armazena `int id`; `sim_.state(id)` precisa do ID.
- `step(dt)` itera `entries_` duas vezes: primeiro buildContext para todos, depois vessel tick para todos, depois `sim_.step(dt)`, depois EMA update. Separação intencional para consistência Jacobi.
- `addNavalForceModel` usa `.at(bodyId)` (throws se body não existe) para segurança.
- Teste de BSM tugForces: usa BSM standalone (fora da variant do vessel) para manter pointer estável. Vessel recebe cópia do BSM via `setController(*bsm)`. TugParametricForces aponta para o BSM standalone e é atualizado manualmente antes do assert.
- Teste ManeuverController end-to-end: ship com mass=1e5 kg, speedKp=300, 200 steps × 0.5s. Verifica `state(0).q()[0] > 0`.

## Files / Surfaces

- `libs/simulation/include/ymir/simulation/NavalSimulation.h` — reescrito (BodyEntry struct + nova API)
- `libs/simulation/src/NavalSimulation.cpp` — reescrito
- `libs/simulation/CMakeLists.txt` — adicionado `ymir_vessel` em target_link_libraries
- `tests/simulation/TestNavalInfra.cpp` — migrado
- `tests/simulation/TestNavalIntegration.cpp` — migrado (`state()` → `state(0)`, construtor → `addBody`)
- `tests/simulation/TestVesselIntegration.cpp` — criado (20 testes, todos passando)
- `tests/CMakeLists.txt` — adicionado `TestVesselIntegration.cpp`

## Learnings

- BSM dentro de `std::variant` (DynamicVessel) não expõe pointer externo → para testar tugForces via TugParametricForces, usar BSM standalone fora do vessel.
- `BodyEntry::q_avg` é `ymir::Vector6{}` (zero-init by default) — initialize() corrige isso.
- `sim_.state(id)` pode ser chamado a qualquer momento após `addBody` sem `initialize()`.
