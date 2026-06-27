# Repo Reorganization — Task List

## Tasks

| # | Title | Status | Complexity | Dependencies |
|---|-------|--------|------------|--------------|
| 01 | Scaffold `libs/`, `apps/`, `services/` directory structure | completed | low | — |
| 02 | Move `libs/common/` — math utilities + PhysicalConstants | completed | medium | task_01 |
| 03 | Move `libs/physics/` — bodies, integrator, all force implementations | completed | high | task_01, task_02 |
| 04 | Move `libs/simulation/` — Simulation + NavalSimulation | completed | low | task_01, task_03 |
| 05 | Move `libs/vessel/` — VesselConfig + NavalContext | completed | low | task_01, task_03 |
| 06 | Move `libs/world/` — wave engine + NavalEnvironment | completed | medium | task_01, task_03, task_05 |
| 07 | Move `libs/persistence/` — JSON scenario reader | completed | medium | task_01, task_03, task_05, task_06 |
| 08 | Write CMakeLists.txt for 6 libs + update root CMakeLists.txt | completed | high | task_02, task_03, task_04, task_05, task_06, task_07 |
| 09 | Reorganize tests — move files + rewrite tests/CMakeLists.txt | completed | high | task_08 |
| 10 | Delete legacy dirs, update `.gitignore`, verify build | completed | medium | task_09 |
| 11 | Create `docs/architecture/` wiki | completed | medium | — |
| 12 | Update Doxyfile + wire docs CMake target | done | low | task_08 |
| 13 | Rewrite `AGENTS.md` | completed | medium | task_11 |
| 14 | Rewrite `README.md` | done | low | task_11, task_13 |
