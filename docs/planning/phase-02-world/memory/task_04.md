# Task Memory: task_04.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Implement `CouplingForceModel` — header-only `NavalForceModel` that reads resolved coupling force from `CouplingRegistry` for a consumer body. Bridge delivering Jacobi coupling forces into CVODE integration.

Status: **COMPLETE**

## Important Decisions

- Class placed in `namespace ymir` (not `ymir::naval`) per TechSpec and task spec — consistent with CouplingRegistry.
- Base class referenced with full qualification: `ymir::naval::NavalForceModel`.
- `computeNaval` takes `const ymir::naval::NavalContext&` (full qualification required from `ymir` namespace).
- Tests call `bindContext(&ctx)` with a default-constructed `NavalContext` before `compute()` — required because `NavalForceModel::compute()` asserts ctx != nullptr.

## Files / Surfaces

- Created: `libs/simulation/include/ymir/simulation/CouplingForceModel.h`
- Created: `tests/simulation/TestCouplingForceModel.cpp`
- Modified: `tests/CMakeLists.txt` — added `TestCouplingForceModel.cpp` to `ymir_simulation_tests`

## Verification Evidence

- `cmake --build build` exit 0
- `ctest`: 179/179 passed, 0 failed

## Ready for Next Run

task_05 (NavalDomain) can include `CouplingForceModel.h` and call `addNavalForceModel()` to wire coupling models to consumer bodies.
