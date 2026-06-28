# Workflow Memory

Keep only durable, cross-task context here. Do not duplicate facts that are obvious from the repository, PRD documents, or git history.

## Current State

task_01 through task_07 complete. `Environment`, `IDomain`, `BodyPosition`, `CouplingRegistry`, and `World` live in `ymir_world`. `CouplingForceModel` + `NavalDomain` live in `ymir_simulation`. `NavalSimulation` is deprecated. All 203 tests pass (ctest full suite); coverage line rate is 84.80%.

## Shared Decisions

- **Deprecated aliases**: Put in BOTH `ymir` and `ymir::naval` namespaces so all existing callers resolve without modification. See NavalEnvironment.h.
- **`-Werror` blocks deprecated-alias usage**: Any target that uses a deprecated alias will fail to compile. Existing code must migrate to the non-deprecated type, not just use the alias. Use `#pragma clang diagnostic` only in tests that explicitly verify the alias compiles.
- **`ymir_simulation` includes `libs/world` headers but does not link `ymir_world`**: Inline getters work; symbol-generating setters would fail at link time. Do not add setter calls to `NavalSimulation.cpp` without linking `ymir_world` first.
- **CouplingRegistry home**: `CouplingRegistry` moved from `ymir_simulation` to `ymir_world` in task_07 so `World` can own the registry without a circular CMake dependency.

## Shared Learnings

- `ymir_simulation/CMakeLists.txt` has `libs/world/include` in include path already — world headers are available without adding a link dep.

## Open Risks

- ~~`ymir_simulation` does not link `ymir_world`~~ — **resolved in task_03**: `ymir_world` added to `ymir_simulation` link libraries. task_05 (NavalDomain) does NOT need to add it separately.

## Shared Learnings (cross-task)

- **Abstract base default constructor**: Deleting a copy constructor via `= delete` suppresses the implicit default constructor. Always add `IDomain() = default` (or any abstract base) explicitly, or derived classes cannot be default-constructed.

## Handoffs

task_01 → task_02: complete.
task_02 → task_03/task_05: `ymir::IDomain` + `ymir::BodyPosition` available in `libs/world/include/ymir/world/IDomain.h`. `CouplingRegistry` is forward-declared there — task_03 must define it, and task_05 (NavalDomain) can then include both.
task_03/task_07 → later tasks: `CouplingRegistry` is defined in `libs/world/include/ymir/world/CouplingRegistry.h`. `ymir_simulation` links `ymir_world`; use `<ymir/world/CouplingRegistry.h>` for new includes.
task_04 → task_05: `CouplingForceModel` in `libs/simulation/include/ymir/simulation/CouplingForceModel.h`, namespace `ymir`, inherits `ymir::naval::NavalForceModel`. task_05 (`NavalDomain`) registers it via `addNavalForceModel()` on consumer bodies. Requires `bindContext()` before `compute()` — handled automatically by `NavalDomain::initialize()`.
task_05 → task_07: `NavalDomain` in `libs/simulation/include/ymir/simulation/NavalDomain.h`, namespace `ymir`, implements `IDomain`. `World::addDomain()` must call `domain.onAddedToWorld(env_, coupling_)`. `NavalDomain` is non-copyable and non-movable — World must store domains by pointer/unique_ptr. `NavalDomain::initialize()` must be called after `onAddedToWorld` (safe to call before too, but context won't be set until env_ is non-null).
task_06 → task_07: `BerthManeuverSystem::setCouplingRegistry(CouplingRegistry*, int shipBodyId, const std::vector<int>& tugBodyIds)` is implemented. In registry mode, BSM writes each tug force with `writeForce(tugBodyIds[i], force)` and leaves `tugForces()` zero; without a registry it preserves legacy parametric accumulation. `TugParametricForces` is deprecated for Phase 3 removal.
task_07 → Phase 3: `WorldSnapshot` is nested by domain and stores full `BodyState` per body. `World::step(dt)` is the thin orchestrator sequence: advance time, reset coupling, step domains, resolve coupling.
