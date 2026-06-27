---
status: completed
title: CouplingRegistry
type: backend
complexity: medium
dependencies: []
---

# Task 3: CouplingRegistry

## Overview

Implement `CouplingRegistry` in `libs/simulation/` — the component that mediates
weak Jacobi force coupling between simulation bodies. It stores (producer, consumer)
force links, accepts force writes from producers during domain steps, resolves links
after all domain steps complete, and exposes resolved forces to consumer bodies.
Update `libs/simulation/CMakeLists.txt` to compile the new source and formally add
`ymir_world` as a link target.

<critical>
- ALWAYS READ the PRD and TechSpec before starting
- REFERENCE TECHSPEC for implementation details — do not duplicate here
- FOCUS ON "WHAT" — describe what needs to be accomplished, not how
- MINIMIZE CODE — show code only to illustrate current structure or problem areas
- TESTS REQUIRED — every task MUST include tests in deliverables
</critical>

<requirements>
- `CouplingRegistry` MUST be in `libs/simulation/include/ymir/simulation/CouplingRegistry.h` in namespace `ymir`
- `CouplingRegistry` MUST provide: `addLink(int producerBodyId, int consumerBodyId)`, `writeForce(int producerBodyId, const Forces& f)`, `resolve()`, `consumedForce(int consumerBodyId) const`, `reset()`
- `consumedForce()` MUST return `Forces::zero()` when no link exists for `consumerBodyId` OR when the link exists but `ready == false`
- `reset()` MUST clear `ready` flags on all links WITHOUT clearing force values (the last resolved force remains readable until overwritten)
- `resolve()` MUST copy each link's `force` field into the consumer's slot only when `ready == true`; if a producer did not write in the current tick, the previous tick's resolved force remains in the consumer slot
- `addLink()` with a duplicate `(producerBodyId, consumerBodyId)` pair MUST assert (debug) — no silent duplicates
- `writeForce()` with unknown `producerBodyId` MUST assert (debug) — links must be registered before writing
- `libs/simulation/CMakeLists.txt` MUST add `ymir_world` to `target_link_libraries` and add `CouplingRegistry.cpp` to sources
- Zero heap allocation per `writeForce`, `resolve`, `consumedForce`, `reset` call — all links pre-allocated via `addLink`
</requirements>

## Subtasks

- [ ] 3.1 Create `libs/simulation/include/ymir/simulation/CouplingRegistry.h` with class declaration and `Link` private struct
- [ ] 3.2 Create `libs/simulation/src/CouplingRegistry.cpp` with all method implementations
- [ ] 3.3 Update `libs/simulation/CMakeLists.txt`: add `ymir_world` to `target_link_libraries`, add `CouplingRegistry.cpp` to sources
- [ ] 3.4 Write unit tests in `tests/simulation/TestCouplingRegistry.cpp`
- [ ] 3.5 Verify all existing simulation tests still pass after CMakeLists change

## Implementation Details

See TechSpec "Core Interfaces — CouplingRegistry" section for the exact method signatures
and the `Link` private struct layout (`producerBodyId`, `consumerBodyId`, `Forces force`,
`bool ready`).

The registry uses `std::vector<Link>` internally. With N tugs ≤ 10 in any realistic
scenario, linear scan in `consumedForce()` and `writeForce()` is O(N) — acceptable.

`resolve()` iterates links where `ready == true` and copies `link.force` to a
`std::map<int, Forces> resolved_` (keyed by consumerBodyId). `consumedForce(id)`
looks up `resolved_`. This two-container approach avoids ambiguity when multiple
producers write to the same consumer (forces accumulate via `+=` in `resolve()`).

`reset()` sets all `link.ready = false`. The `resolved_` map is NOT cleared in reset —
it holds the last resolved state until overwritten in the next `resolve()`.

### Relevant Files

- `libs/physics/include/ymir/physics/Forces.h` — `Forces::zero()`, `Forces` type, `+=` operator
- `libs/simulation/CMakeLists.txt` — must add ymir_world dep and new source
- `tests/simulation/CMakeLists.txt` — must add TestCouplingRegistry.cpp

### Dependent Files

- `libs/simulation/include/ymir/simulation/CouplingForceModel.h` (task_04) — reads `consumedForce()`
- `libs/simulation/include/ymir/simulation/NavalDomain.h` (task_05) — holds `CouplingRegistry*`
- `libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h` (task_06) — writes via `writeForce()`
- `libs/world/include/ymir/world/World.h` (task_07) — owns CouplingRegistry, calls reset/resolve

### Related ADRs

- [ADR-004: Acoplamento Jacobi com CouplingRegistry](adrs/adr-004.md) — Jacobi semantics define reset/resolve ordering

## Deliverables

- `libs/simulation/include/ymir/simulation/CouplingRegistry.h`
- `libs/simulation/src/CouplingRegistry.cpp`
- `libs/simulation/CMakeLists.txt` updated
- `tests/simulation/TestCouplingRegistry.cpp` with unit tests **(REQUIRED)**

## Tests

- Unit tests:
  - [ ] `addLink(1, 0) + writeForce(1, F) + resolve() + consumedForce(0)` returns `F`
  - [ ] `consumedForce(0)` before any `resolve()` returns `Forces::zero()`
  - [ ] `reset()` sets ready to false: after reset, `consumedForce(0)` returns the last resolved force (not zero) but a second `resolve()` without a `writeForce()` leaves the consumer with the previous resolved value
  - [ ] Two producers (id=1, id=2) both writing to consumer (id=0): `consumedForce(0)` returns sum of both forces after `resolve()`
  - [ ] `writeForce()` with unknown producer id triggers assertion
  - [ ] `addLink()` with duplicate pair triggers assertion
  - [ ] `consumedForce()` for consumer with no link returns `Forces::zero()`
  - [ ] Producer writes zero force: `consumedForce()` returns `Forces::zero()` (not leftover from previous tick)
- Integration tests:
  - [ ] (Covered by task_07 integration test — no standalone integration test needed)
- Test coverage target: >=80%
- All tests must pass

## Success Criteria

- All tests passing
- Test coverage >=80%
- `std::vector<Link>` size does not change after initial `addLink()` calls — no per-tick allocation
- Existing `ymir_simulation` tests continue to pass after CMakeLists update
