# Task Memory: task_05.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot
- Move vessel-owned `VesselConfig.h` and `NavalContext.h` from the legacy naval include tree into `libs/vessel/include/ymir/vessel/`, then update direct include paths without moving world/wave/environment files or creating the real vessel CMake target.

## Important Decisions
- Treat task_05, TechSpec, and ADR-002 as authoritative for `NavalContext.h` destination (`libs/vessel/`). ADR-001 contains a stale line placing `NavalContext.h` under `libs/world/`.

## Learnings
- Baseline before edits: stale `ymir/naval/NavalContext.h` includes existed in naval force tests, wave test, `libs/simulation/include/ymir/simulation/NavalSimulation.h`, and `libs/physics/include/ymir/physics/NavalForceModel.h`; stale `ymir/naval/VesselConfig.h` includes existed in four physics force headers.
- Unfiltered `grep -r "ymir/naval/VesselConfig\|ymir/naval/NavalContext" .` is noisy because it matches task/ADR/spec documents and generated build dependency files. Source-scoped greps over `*.h`/`*.cpp` are the useful validation signal for this task.
- Coverage remains unavailable as a numeric gate: no project coverage target/config is present, `lcov` and `llvm-cov` are not installed locally, and `/usr/bin/gcov` alone is not enough without coverage instrumentation.

## Files / Surfaces
- Expected touch points: `naval/include/ymir/naval/{VesselConfig.h,NavalContext.h}`, `libs/vessel/include/ymir/vessel/`, tests including `NavalContext.h`, physics force headers including `VesselConfig.h`, and temporary old-target include wiring if build resolution requires it.
- Moved headers to `libs/vessel/include/ymir/vessel/{VesselConfig.h,NavalContext.h}` and updated direct consumers in physics force headers, simulation header, and naval tests.
- Added `libs/vessel/include` to `ymir_naval` PUBLIC include directories as temporary compatibility wiring until task_08 creates real context targets.

## Errors / Corrections
- Build required no code changes beyond temporary `ymir_naval` include exposure; `cmake --build build` succeeded after the move.

## Ready for Next Run
- Validation completed: source-scoped stale include greps returned no matches, `cmake --build build` exited 0, `ctest --test-dir build --output-on-failure` passed 72/72, and `git diff --check` exited 0.
