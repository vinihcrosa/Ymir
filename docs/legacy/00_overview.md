# TMS Dynamics Engine — Visão Geral e Arquitetura

## O que é

Motor de simulação dinâmica naval 6-DOF. Simula navios e plataformas flutuantes sob
corrente, vento e ondas. Suporta propulsão, leme, rebocadores e manobras autônomas.

Dois modos de uso:
- **Fast-time**: simulação pura (mais rápida que tempo real).
- **Real-time**: expõe C API para integração com simuladores externos (Unity, etc).

---

## Estrutura de pastas

```
dynamics-github/
├── src/                    # Biblioteca principal (TMSCore)
│   ├── basic/              # DyM.h — utilitários matemáticos, namespace DyM::Math
│   ├── body/               # Vessel, Propeller, Rudder, Tug — estado + propriedades
│   ├── dynamics/           # BodyDyM — integra Vessel + forças + CVODE
│   ├── solver/             # RHS.cpp, SundialsInterface.cpp — equação de movimento
│   ├── inertialForces/     # Acoplamento Coriolis/giroscópico
│   ├── restoringForces/    # Mola hidrostática
│   ├── dampingForces/      # Amortecimento: radiation + linear + quadrático
│   ├── squatForces/        # Efeito squat (águas rasas) — ICORELS
│   ├── currentForces/      # Arrasto de corrente — Obokata ou REGULAR+VDP
│   ├── windForces/         # Arrasto de vento — tabelas Cd por ângulo
│   ├── waveForces/         # Excitação, drift, RAO — WAMIT tables
│   ├── thrustForces/       # Hélice — curvas abertas Kt/Kq vs J
│   ├── rudderForces/       # Leme — Cl/Cd vs ângulo de ataque
│   ├── tugForces/          # Rebocadores — push/pull com filtros de 1ª ordem
│   ├── actuators/          # Actuator, FilterActuator, RateLimitedActuator
│   ├── bridge/             # ManeuverSystem, BerthManeuverSystem — FSM de manobra
│   ├── controller/         # PID, ManeuverController, BerthManeuverController
│   ├── weather/            # Weather, SurfaceWaves — espectro e spreading
│   └── IO/                 # IO_Input, IO_OutputWriter, VesselProperties
├── applications/
│   ├── fasttime/           # Executáveis: fasttimeDyM, fasttimeBerthDyM, etc
│   ├── realtime/           # realtimeDyMLib — C API para embedding
│   ├── staticForces/       # staticForcesDyM — balanço estático sem integração
│   └── wavesEngineLib/     # waveEngineDyMLib — C API só para espectro de ondas
├── include/
│   ├── WaveAPI.h           # C API pública do wavesEngine
│   └── RealtimeDyMAPI.h    # C API pública do realtime
├── tests/                  # Catch2: BargeTests, PlatformTests
└── templates/              # Exemplos de uso das APIs
```

---

## Hierarquia de objetos em runtime

```
main()
└── IO_Input                    ← lê JSON, popula structs tipados
└── Weather                     ← condições ambientais globais
└── BodyDyM (1 por navio)
    ├── Vessel                  ← estado 6-DOF + todas propriedades do navio
    ├── Force modules (11)      ← modelos de força, cada um com ref const Vessel
    │   ├── InertialForces
    │   ├── RestoringForces
    │   ├── DampingForces
    │   ├── SquatForces
    │   ├── CurrentForces
    │   ├── WindForces
    │   ├── WaveForces          ← agrega N WaveComponent (um por componente espectral)
    │   ├── ThrustForces[]      ← um por hélice
    │   ├── RudderForces[]      ← um por leme
    │   └── TugForces[]         ← um por rebocador
    ├── RHS                     ← monta Ft, calcula q_ddot = M⁻¹ · Ft
    └── CVODE (memória)         ← integrador ODE da SUNDIALS
```

---

## Fluxo de dados por passo de tempo

```
CVODE chama rhs_sundials(t, y[12], ydot[12])
  │
  ├─ Extrai y → q[6] (posição), q_dot[6] (velocidade no frame do navio)
  │
  ├─ RHS::operator()(q, q_dot)
  │   ├─ Vessel::updateStates(weather, t, dt)
  │   │   ├─ Rotaciona corrente/vento para frame do navio
  │   │   ├─ Calcula speedToWater, speedToWind
  │   │   ├─ Atualiza hélices (dinâmica de 1ª ordem)
  │   │   ├─ Atualiza lemes (rate limiter)
  │   │   └─ EMA da posição q_avg (τ=16.5s)
  │   │
  │   ├─ RHS::compute_loads()        ← soma todos os modelos de força → Ft[6]
  │   └─ RHS::compute_acceleration() ← q_ddot = invTotalMass · Ft
  │
  ├─ RHS::update_position_from_rao() ← aplica delta de posição RAO no frame inercial
  │
  └─ Transforma para frame inercial:
      ydot[0] = cos(ψ)·u - sin(ψ)·v
      ydot[1] = sin(ψ)·u + cos(ψ)·v
      ydot[2] = w
      ydot[3:6] = q_dot[3:6]      (roll/pitch/yaw rates — small angle)
      ydot[6:12] = q_ddot[0:6]

CVODE atualiza y com integral de ydot (BDF, adaptativo)
```

---

## Módulos de força — ordem de chamada em `compute_loads()`

| # | Módulo | Arquivo | Variável em Vessel |
|---|--------|---------|-------------------|
| 1 | InertialForces | `Fi.inertial()` | `v.Fi` |
| 2 | RestoringForces | `Fr.hydrostatic()` | `v.Fr` |
| 3 | DampingForces | `Fd.damping()` | `v.Fd` |
| 4 | SquatForces | `Fsq.squat()` | `v.Fsq` |
| 5 | CurrentForces | `Fc.current()` | `v.Fc` |
| 6 | WindForces | `Fwd.wind()` | `v.Fwd` |
| 7 | WaveForces (excitação) | `Fwv.wave(t)` | `v.Fwv_ex` |
| 8 | WaveForces (mean drift) | idem | `v.Fwv_md` |
| 9 | WaveForces (slow drift) | idem | `v.Fwv_sd` |
| 10 | WaveForces (drift damping) | idem | `v.Fwv_dd` |
| 11 | ThrustForces[] | `Fth[i].thrust()` | `v.Fth` |
| 12 | RudderForces[] | `Frd[i].rudder()` | `v.Frd` |
| 13 | TugForces[] | `Ftg[i].tug()` | `v.Ftg` |

`Ft = Fi + Fr + Fd + Fsq + Fc + Fwd + Fwv_ex + Fwv_md + Fwv_sd + Fwv_dd + Fth + Frd + Ftg`

---

## Convenções de referência

- **Frame inercial**: X = Leste, Y = Norte, Z = cima (NED invertido em Z). Posição absoluta em metros.
- **Frame do navio (body)**: X = proa, Y = bombordo, Z = cima. Velocidades `u/v/w/p/q/r` neste frame.
- **Yaw (ψ)**: armazenado em radianos, 0 = Leste, cresce anti-horário. Conversão de entrada: `ψ = deg2rad(450 - heading_deg)` onde heading é convençao náutica (0=N, 90=E).
- **Ângulo de incidência** de vento/corrente: calculado por `atan2(vy_body, vx_body)`, em graus.
- **Origem WAMIT** (`wavesOriginPosition`): ponto de referência dos dados hidrodinâmicos (geralmente no CG, na linha d'água). Braços de momento calculados em relação a esse ponto.
- **Unidades**: SI — N, N·m, m, s, rad. Massas no JSON em **toneladas**, convertidas para kg no construtor de `Vessel`.

---

## Dependências externas

| Biblioteca | Uso |
|-----------|-----|
| SUNDIALS CVODE v5.8.0 | Integrador ODE (BDF, stiff-capable) |
| Catch2 | Testes unitários |
| nlohmann/json | Parser JSON (header-only, incluído) |
| CMake ≥ 3.10 | Build system |

C++17 obrigatório (`std::optional`, `std::filesystem`, structured bindings).
