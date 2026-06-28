# TMS Dynamics Engine — Análise Técnica

Documentação técnica gerada a partir de leitura direta do código fonte (branch `dev`, 2026-06-21).
Não se baseia na documentação oficial — reflete o que o código realmente faz.

## Índice

| Arquivo | Conteúdo |
|---------|----------|
| [00_overview.md](00_overview.md) | Arquitetura geral, estrutura de pastas, hierarquia de objetos, fluxo de dados por passo, convenções de referencial |
| [01_state_and_data_formats.md](01_state_and_data_formats.md) | Vetor de estado 12-DOF, formato completo dos JSONs de entrada, propriedades derivadas em `Vessel`, formato do CSV de saída |
| [02_integration_loop.md](02_integration_loop.md) | Equação central, configuração do CVODE, `rhs_sundials`, `RHS::operator()`, transformações de referencial, `updateStates` |
| [03_force_models.md](03_force_models.md) | Todos os 11 modelos de força: Inertial, Restoring, Damping, Squat, Current (Obokata + REGULAR + Van der Pol), Wind, Thrust (Kt/Kq), Rudder (foil), Tug |
| [04_wave_engine.md](04_wave_engine.md) | Espectro JONSWAP/Pierson/Regular, spreading direcional Mitsuyasu, `WaveComponent` (excitação 1ª ordem, mean drift, slow drift, drift damping, RAO), `WaveAPI` C |
| [05_maneuver_system.md](05_maneuver_system.md) | Módulos drift/maneuver/berthManeuver, LOS + PID, FSM de atracação (navigating/sideway/turnROTTUG), atuadores |
| [06_math_utilities.md](06_math_utilities.md) | `DyM::Math`: interpolação, álgebra linear, RK4, transformações, wrapping de ângulos |
| [07_public_apis.md](07_public_apis.md) | `WaveAPI` e `RealtimeDyMAPI` — C interface para embedding externo, notas de integração C#/Python |
| [08_known_issues.md](08_known_issues.md) | Bugs confirmados, TODOs no código, limitações de modelo, guia para extensão |

## Resumo da equação de movimento

```
(M + A) · q̈ = Fi + Fr + Fd + Fsq + Fc + Fwd + Fwv_ex + Fwv_md + Fwv_sd + Fwv_dd + Fth + Frd + Ftg

q̈ = invTotalMass · Ft

CVODE/BDF integra y[12] = {q[6], q_dot[6]} com tolerâncias reltol=1e-8, abstol=1e-10
```

## Convenções rápidas

- Frame do navio: X=proa, Y=bombordo, Z=cima
- Yaw: 0=Leste, cresce anti-horário. Entrada: `ψ = deg2rad(450 - heading_náutico)`
- Unidades: N, N·m, m, s, rad. JSON em **toneladas** (convertido para kg no construtor)
- Origem WAMIT: ponto de referência dos dados hidrodinâmicos (não é necessariamente o CG)
