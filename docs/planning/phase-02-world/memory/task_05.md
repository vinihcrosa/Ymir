# Task Memory: task_05.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Implement `NavalDomain` as copy-refactor of `NavalSimulation` implementing `IDomain`. Deprecate `NavalSimulation`. All tests pass (191/191).

## Important Decisions

- `initialize()` guards `buildContext()` call with `if (env_ != nullptr)` — allows use without World per TechSpec, avoiding assert in non-World tests.
- `reset()` similarly guards `buildContext()` call.
- `buildContext()` has no assert; protection is in `step()` which asserts `env_ != nullptr` before calling buildContext.
- Existing tests (TestNavalInfra, TestNavalIntegration, TestVesselIntegration) got `#pragma clang diagnostic push/pop` around the NavalSimulation include. The pragma only suppresses the include-site warning; usage-site warnings remain but are not -Werror, so build succeeds.
- `NavalDomain` lives in `namespace ymir` (not `ymir::naval`). Vessel types (`DynamicVessel`, `ThrustForces`, `RudderForces`) are in `namespace ymir::naval` — must use fully qualified names in header and impl.

## Learnings

- `[[deprecated]]` attribute on a class generates `-Wdeprecated-declarations` at usage sites. The project's `target_set_warnings` does NOT treat this as `-Werror`, so deprecated class usage compiles with warnings only.
- `#pragma clang diagnostic push/pop` around an `#include` only suppresses include-directive warnings, not usage-site warnings later in the same file.

## Files / Surfaces

- `libs/simulation/include/ymir/simulation/NavalDomain.h` — created
- `libs/simulation/src/NavalDomain.cpp` — created
- `libs/simulation/include/ymir/simulation/NavalSimulation.h` — added `[[deprecated]]`
- `libs/simulation/CMakeLists.txt` — added `NavalDomain.cpp`
- `tests/simulation/TestNavalDomain.cpp` — created (12 tests: unit + regression)
- `tests/CMakeLists.txt` — added TestNavalDomain.cpp to ymir_simulation_tests
- `tests/simulation/TestNavalInfra.cpp` — pragma around NavalSimulation include
- `tests/simulation/TestNavalIntegration.cpp` — pragma around NavalSimulation include
- `tests/simulation/TestVesselIntegration.cpp` — pragma around NavalSimulation include

## Errors / Corrections

- Initial header used unqualified `DynamicVessel`, `ThrustForces`, `RudderForces` — fixed to `naval::DynamicVessel`, `naval::ThrustForces`, `naval::RudderForces`.

## Ready for Next Run

task_05 complete. 191/191 tests pass. `NavalDomain` ready for `World` integration in task_07.
