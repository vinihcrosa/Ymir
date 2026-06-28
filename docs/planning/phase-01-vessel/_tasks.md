# Camada de Embarcação — Fase 1 — Task List

## Tasks

| # | Title | Status | Complexity | Dependencies |
|---|-------|--------|------------|--------------|
| 01 | Refactor ThrustForces, RudderForces e VesselConfig para estado de atuador externo | completed | medium | — |
| 02 | Implementar entidade Thruster | completed | medium | task_01 |
| 03 | Implementar entidade Rudder | completed | medium | task_01 |
| 04 | Criar VesselState | completed | low | — |
| 05 | Implementar PrescribedController | completed | medium | task_02, task_03 |
| 06 | Implementar ManeuverController | completed | medium | task_02, task_03 |
| 07 | Implementar BerthManeuverSystem | completed | high | task_06 |
| 08 | Criar DynamicVessel aggregate root | completed | high | task_02, task_03, task_04, task_05, task_06, task_07 |
| 09 | Refatorar NavalSimulation para N corpos | completed | high | task_08 |
