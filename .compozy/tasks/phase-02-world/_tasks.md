# Phase 2 — World + Multi-Domain Naval Architecture — Task List

## Tasks

| # | Title | Status | Complexity | Dependencies |
|---|-------|--------|------------|--------------|
| 01 | Environment class + NavalEnvironment deprecated | done | low | — |
| 02 | IDomain interface + BodyPosition struct | done | low | task_01 |
| 03 | CouplingRegistry | done | medium | — |
| 04 | CouplingForceModel | done | low | task_03 |
| 05 | NavalDomain + NavalSimulation deprecation | pending | high | task_01, task_02, task_03, task_04 |
| 06 | BerthManeuverSystem coupling update + TugParametricForces deprecated | done | medium | task_03 |
| 07 | WorldSnapshot + World orchestrator + integration tests | done | medium | task_01, task_02, task_03, task_05, task_06 |
