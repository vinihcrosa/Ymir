# Task Memory: task_07.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Implementar BerthManeuverSystem (FSM 3-estados) + TugParametricForces (NavalForceModel adapter). Completo.

## Important Decisions

- Internal `ManeuverController` criado com `captureRadius_m = 0.0` para nunca avançar waypoints automaticamente — BSM controla todas as transições.
- BSM não expõe método de troca de waypoint ativo; `activeWpt_` fixo em 0 para Phase 1 (single berth waypoint assumption).
- `checkTransitions()` executado ANTES da lógica de fase no mesmo tick — fase nova já roda no tick de transição.
- Lateral error calculado como projeção perpendicular ao `headingTarget_rad`: `-dx*sin(headingTarget) + dy*cos(headingTarget)`.
- Tug forces em Navigating: soma vetorial body-frame por bearing_deg; em Sideway/TurnROTTUG: força sway (index 1) via PD + clamp a pushForce_N.
- `TugParametricForces` armazena `const BerthManeuverSystem*` (não-owner) — BSM deve outlive o TugParametricForces.

## Learnings

- `NavalForceModel` é non-copyable E non-movable — sempre criar via `std::make_unique`.
- `buildNavMc()` helper estático é invocado no initializer list APÓS `cfg_` ser inicializado, o que funciona pois membros são inicializados na ordem de declaração da classe.
- Catch2 v3 no macOS não aceita `-v` nem `--filter`; usar test name substring ou rodar tudo.

## Files / Surfaces

- `libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h` (novo)
- `libs/vessel/src/controllers/BerthManeuverSystem.cpp` (novo)
- `libs/vessel/include/ymir/vessel/controllers/TugParametricForces.h` (novo)
- `libs/vessel/src/controllers/TugParametricForces.cpp` (novo)
- `libs/vessel/tests/TestBerthManeuverSystem.cpp` (novo, 11 test cases)
- `libs/vessel/CMakeLists.txt` (atualizado: +2 fontes)
- `libs/vessel/tests/CMakeLists.txt` (atualizado: +TestBerthManeuverSystem.cpp)

## Errors / Corrections

- Nenhum erro de compilação ou teste.

## Ready for Next Run

Task 07 completa. `BerthManeuverSystem` e `TugParametricForces` prontos para uso em `DynamicVessel` (task_08).
