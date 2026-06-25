# Task Memory: task_02.md

Keep only task-local execution context here. Do not duplicate facts that are obvious from the repository, task file, PRD documents, or git history.

## Objective Snapshot

Done. Moved 3 math headers + PhysicalConstants.h into `libs/common/`, updated all 26 includes (14 math, 12 const), build+72 tests green.

## Important Decisions

- Added `$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libs/common/include>` to `ymir_core` PUBLIC include dirs. Needed so moved headers resolve without a `libs/common` target (forbidden until task_08). PUBLIC linkage propagates to `ymir_naval` (links core) and all test exes. This temp wiring gets removed when task_08 adds the real `ymir_common` target.
- Moved math headers still `#include <ymir/core/Types.h>` internally — left as-is (Types.h moves in task_03; not in this task's grep scope).

## Learnings

- `for f in $files` with multiline var failed (sed got whole blob as one arg). Use `grep -rl ... | while IFS= read -r f`.
- task_01 already created the 6 `libs/<ctx>/include/ymir/<ctx>/` skeleton, but NOT the `math/` subdir under common — had to `mkdir -p` before `git mv`.

## Files / Surfaces

- Moved: `libs/common/include/ymir/common/math/{LinearAlgebra,AngleUtils,Interpolation}.h`, `libs/common/include/ymir/common/PhysicalConstants.h`
- Edited includes: core/src/RigidBody6DOF.cpp; naval/src + naval/src/forces/* + naval/src/wave/*; tests/core/{TestMath,TestBody}.cpp; tests/naval/{TestSquat,TestRestoring,TestNavalInfra}Forces/Infra.cpp
- Edited CMake: `core/CMakeLists.txt` (added libs/common/include)

## Errors / Corrections

## Ready for Next Run

task_03 (move core body/force/integrator → libs/physics) will update Types.h path and can drop the temp include-dir hack once libs/physics + common targets land.
