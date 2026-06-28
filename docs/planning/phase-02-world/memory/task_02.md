# Task Memory: task_02.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Create `IDomain` abstract interface + `BodyPosition` POD struct in `libs/world/include/ymir/world/IDomain.h`.

## Important Decisions

- Added `IDomain() = default;` explicitly: declaring `IDomain(const IDomain&) = delete` suppresses the implicit default constructor, causing derived-class default construction to fail. Must be explicit.
- `CouplingRegistry` forward-declared only (per task spec) — full definition comes in task_03.
- Included `Environment.h` (not just forward-declared) — `onAddedToWorld` takes `Environment&`; TechSpec success criteria explicitly allows it.
- `BodyState.h` included because `bodyState(int id) const` returns `BodyState` by value — full type required.

## Learnings

- Deleting a copy constructor via `= delete` is a user-declared constructor → suppresses the implicit default constructor. Always add `= default` explicitly on abstract bases when derived classes need default construction.

## Files / Surfaces

- **Created**: `libs/world/include/ymir/world/IDomain.h`
- **Created**: `tests/world/TestIDomain.cpp`
- **Modified**: `tests/CMakeLists.txt` — added `world/TestIDomain.cpp` to `ymir_world_tests`

## Errors / Corrections

- Initial build failed: derived StubDomain default construction failed because IDomain's implicit default constructor was suppressed by the deleted copy constructor declaration. Fixed by adding `IDomain() = default;`.

## Ready for Next Run

task_02 complete. `IDomain` + `BodyPosition` live in `ymir_world`. 165/165 tests pass.
