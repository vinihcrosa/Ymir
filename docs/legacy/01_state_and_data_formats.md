# Estado do Sistema e Formatos de Dados

## Vetor de estado (CVODE)

O integrador CVODE trabalha com um vetor de 12 doubles chamado `y`:

```
y[0]  = x         (posição Leste, m)
y[1]  = y         (posição Norte, m)
y[2]  = z         (heave, m — positivo pra cima)
y[3]  = roll      (rad)
y[4]  = pitch     (rad)
y[5]  = yaw/ψ     (rad — 0=Leste, cresce anti-horário)
y[6]  = u         (surge velocity, m/s — frame do navio, proa=+x)
y[7]  = v         (sway velocity, m/s — frame do navio, bombordo=+y)
y[8]  = w         (heave velocity, m/s)
y[9]  = p         (roll rate, rad/s)
y[10] = q         (pitch rate, rad/s)
y[11] = r = ψ̇    (yaw rate, rad/s)
```

Em `Vessel`, esses arrays são separados:
```cpp
vector<double> q(6);      // y[0:6]  — posição/ângulo
vector<double> q_dot(6);  // y[6:12] — velocidade no frame do navio
vector<double> q_ddot(6); // acelerações (não no vetor CVODE, computadas pelo RHS)
vector<double> q_avg(6);  // EMA de q com τ=16.5s (usado para ondas)
```

---

## Estrutura de entrada — JSON

### Diretório de entrada

```
<caseDir>/
├── preProcessing/
│   ├── controlDict.json          ← configuração principal
│   ├── bodies/
│   │   └── <nome>.json           ← propriedades de cada navio
│   └── maneuver/
│       └── maneuverParameters.json  (se módulo = maneuver)
└── postProcessing/               ← criado automaticamente na saída
```

### `controlDict.json` — estrutura

```json
{
  "simulation": {
    "module": "maneuver",        // "drift" | "maneuver" | "berthManeuver" | "simulation"
    "dt": 0.1,                   // passo de tempo (s)
    "Tfinal": 600.0,             // tempo final (s); 0 = roda até manobra terminar
    "printSteps": true,          // imprime progresso no stdout
    "writeInterval": 5,          // grava output a cada N passos
    "solver": {
      "reltol": 1e-8,
      "abstol": 1e-10,
      "maxNumSteps": 10000,
      "maxStep": 0.0             // 0 = CVODE decide o passo máximo
    }
  },
  "environment": {
    "seabed_depth": 20.0,        // profundidade da lâmina d'água (m)
    "wind_speed": 10.0,          // m/s
    "wind_direction": 90.0,      // graus náuticos (0=N, 90=E)
    "current_speed": 0.5,        // m/s
    "current_direction": 180.0,  // graus náuticos
    "tide_startTime": 0.0,
    "waves": [
      {
        "height": 2.0,           // Hs (m)
        "period": 10.0,          // Tp (s)
        "direction": 45.0,       // graus náuticos
        "spectrum": "JONSWAP",   // "JONSWAP" | "PIERSON" | "REGULAR"
        "gamma": 3.3,            // fator de pico JONSWAP (0 = padrão)
        "alfa": 0.0
      }
    ]
  },
  "bodies": [
    {
      "id": 0,
      "name": "vessel_name",     // deve bater com arquivo em preProcessing/bodies/
      "module": "maneuver",
      "fsiModels": {
        "current": "OBOKATA",    // "OBOKATA" | "REGULAR"
        "wind": "REGULAR",       // "REGULAR" | "ACSINKAGE"
        "wave": "WAMIT"
      },
      "initialConditions": {
        "position": [x, y, z, roll_deg, pitch_deg, heading_deg],
        "velocity": [u, v, w, p, q, r]
      },
      "thrusterIDs": [0],        // IDs de hélices sob controle do autopiloto
      "rudderIDs": [0],
      "azimuthIDs": []
    }
  ]
}
```

### `<nome>.json` — estrutura do navio (VesselProperties)

```json
{
  "type": "VESSEL",              // "VESSEL" | "PLATFORM"
  "dimensions": {
    "length_BP": 200.0,          // m
    "beam": 30.0,                // m
    "draft": 10.0,               // m
    "height": 25.0,              // m (total, incluindo borda livre)
    "blockCoefficient": 0.75,
    "volumetricWeight": 5e8,     // N (= deslocamento × g, em Newtons)
    "cg": [cx, cy, cz],         // centro de gravidade (m) relativo à origem WAMIT
    "cf": [cx, cy, cz],         // centro de flutuação (m)
    "massMatrix": [              // 6×6, em TONELADAS (convertido para kg internamente)
      [m, 0, 0, 0, 0, 0],
      ...
    ],
    "addedMass": [               // 6×6, em TONELADAS
      ...
    ],
    "hydro_rest": [              // 6×6, rigidez hidrostática (N/m ou N·m/rad)
      ...                        // Na prática: só diagonal usada (off-diag comentado)
    ]
  },
  "damping": {
    "linear": [[...]],           // 6×6 (N·s/m ou N·m·s/rad)
    "potential": [[...]],        // 6×6 — amortecimento de radiação
    "quadratic": [[...]],        // 6×6 (N·s²/m² ou N·m·s²/rad²)
    "linearDampingCoeff": 0.1   // coef. de modulação exponencial do linear
  },
  "current": {
    "model": "OBOKATA",
    "frontalArea": 300.0,        // m² (área frontal submersa)
    "lateralArea": 2000.0,       // m² (área lateral submersa)
    "midshipDistance": 0.0,      // distância do meia-nau à origem WAMIT (m)
    "frontalHeight": -5.0,       // altura do centroide da área frontal (m)
    "lateralHeight": -5.0,
    "coefficients": [            // tabela: [ângulo_inc_deg, Cx, Cy, Cz]
      [0, -0.05, 0, 0],
      [45, -0.03, 0.8, 0.1],
      ...
      [360, -0.05, 0, 0]
    ]
  },
  "wind": {
    "frontalArea": 600.0,        // m² (área frontal acima d'água)
    "lateralArea": 3000.0,
    "midshipDistance": 0.0,
    "frontalHeight": 10.0,
    "lateralHeight": 8.0,
    "coefficients": [            // tabela: [ângulo_inc_deg, Cx, Cy, Cz]
      [0, -0.8, 0, 0],
      ...
    ]
  },
  "waves": {
    "originPosition": [0, 0, -10.0],  // origem WAMIT em [x, y, z] (m)
    "angle": [0, 30, 60, ..., 360],   // ângulos de incidência tabelados (deg)
    "omega": [0.1, 0.2, ..., 2.0],    // frequências angulares (rad/s)
    "waveForces": {                    // força de excitação de 1ª ordem
      "amplitude": [[[...]]],          // [6_dof][N_omega][N_angle] (N ou N·m por m de onda)
      "phase": [[[...]]]               // [6_dof][N_omega][N_angle] (graus)
    },
    "rao": {                           // Response Amplitude Operator
      "amplitude": [[[...]]],          // [6_dof][N_omega][N_angle] (m/m ou rad/m)
      "phase": [[[...]]]               // (graus)
    },
    "meanDrift": {                     // força de deriva média (2ª ordem)
      "amplitude": [[[...]]],          // [6_dof][N_omega][N_angle] (N/m² ou N·m/m²)
      "phase": [[[...]]]               // (graus)
    }
  },
  "thrusters": [
    {
      "id": 0,
      "position": [x, y, z, azimuth_deg],  // posição no frame do navio
      "diameter": 5.0,                      // m
      "rotationSpeed": 120.0,               // RPM nominal
      "rotationSpeedMax": 150.0,
      "pitch_ratio": 1.0,
      "rotationTime": 50.0,                 // constante de tempo 1ª ordem (s)
      "azimuth_speed": 5.0,                 // graus/s
      "maximumPowerW": 5e6,                 // W
      "hullEfficiency": 0.9,
      "asternEfficiency": 0.5,
      "paddleEffect": [c0, c1, c2],         // coefs. efeito paddle
      "transversalSpeed": [vLim, factor],
      "openWaterCurve": [                   // tabela: [J, Kq, Kt]
        [0.0, 0.05, 0.40],
        [0.2, 0.045, 0.35],
        ...
      ]
    }
  ],
  "rudders": [
    {
      "id": 0,
      "associatedThruster": 0,             // -1 se sem hélice associada
      "position": [x, y, z, 0],
      "area": 20.0,                        // m²
      "angleMaximum": 35.0,               // graus
      "speed": 3.0,                        // graus/s (rate limiter)
      "slipStreamFactor": 0.7,            // fração do slipstream que chega ao leme
      "coefficients": [                    // tabela: [ângulo_deg, Cl, Cd]
        [0, 0, 0.01],
        [10, 0.8, 0.02],
        ...
      ]
    }
  ],
  "tugs": [
    {
      "id": 0,
      "bollard_pull": 500000.0,           // N
      "s_max": 4.0,                       // m/s (velocidade máxima do rebocador)
      "conventional": true,
      "thrust_time": 10.0,               // constante de tempo de empuxo (s)
      "dir_speed_max": 15.0,             // graus/min
      "fairlead": [x, y],               // posição do ponto de amarração no navio (m)
      "fpush": {
        "angles": [...],                 // deg
        "force": [...]                   // normalizado [0..1]
      },
      "fpull": {
        "angles": [...],
        "speeds": [...],                 // nós
        "force": [[...]]                 // [N_angles][N_speeds] normalizado
      }
    }
  ]
}
```

---

## Propriedades derivadas em `Vessel` (calculadas no construtor)

```cpp
// Matrizes de massa
massMatrix  = tons2kg_6x6(props.dim.massMatrix)   // toneladas → kg
addedMass   = tons2kg_6x6(props.dim.addedMass)
totalMass   = massMatrix + addedMass               // 6×6
invTotalMass = invert_matrix_6x6(totalMass)        // calculada 1 vez, usada todo passo

// Dimensões básicas
length_BP, beam, draft, height, blockCoefficient, volumetricWeight

// Tabelas de coeficientes (lidas do JSON)
Cdc_incAngle, Cdc_x, Cdc_y, Cdc_z  // corrente
Cdwd_incAngle, Cdwd_x, Cdwd_y, Cdwd_z  // vento

// Tabelas de ondas — indexadas por [dof][omega_idx][angle_idx]
wvForcesAmplitude[6][N_omega][N_angle]
wvForcesPhase[6][N_omega][N_angle]
raoAmplitude[6][N_omega][N_angle]
raoPhase[6][N_omega][N_angle]
mdForceAmplitude[6][N_omega][N_angle]
mdForcePhase[6][N_omega][N_angle]

// Indexadores das tabelas de ondas
wv_angle[N_angle]   // ângulos de incidência (deg)
wv_omega[N_omega]   // frequências (rad/s)

// Seções para integração de corrente (Obokata)
n_sections = 50
sectionLocalPosition[51]  // posições longitudinais das seções (m, relativas ao CG)
```

---

## Variáveis de estado atualizadas a cada passo (`updateStates`)

```cpp
speedToWater[3]   // velocidade relativa à água no frame do navio [u_rel, v_rel, w_rel]
speedToWind[3]    // velocidade relativa ao vento no frame do navio
q_avg[6]          // EMA da posição: α = 1 - exp(-dt/τ), τ=16.5s
x, y              // posição inercial (espelho de q[0], q[1])
u, v              // velocidades inerciais (rotacionadas de q_dot pelo yaw)
sog               // speed over ground = sqrt(u²+v²)
cog               // course over ground (rad, convenção náutica)
driftAngle        // ângulo de deriva = COG - heading
currTime, deltaTime
```

---

## Saída (CSV em `postProcessing/`)

Um arquivo por navio: `postProcessing/body<id>.csv`

Colunas (todas em SI ou graus conforme descrito):

| Grupo | Colunas |
|-------|---------|
| Tempo | `time` |
| Posição | `surge,sway,heave,roll,pitch,yaw` |
| Velocidade | `vel_x,vel_y,vel_z,vel_xx,vel_yy,vel_zz` |
| Aceleração | `acc_x,...,acc_zz` |
| Forças totais | `Fx,Fy,Fz,Mx,My,Mz` |
| Por módulo | `Fi_x,...`, `Fr_x,...`, `Fd_x,...`, `Fsq_z` |
| | `Fc_x,...`, `Fwd_x,...` |
| | `Fwv_ex_x,...`, `Fwv_md_x,...`, `Fwv_sd_x,...`, `Fwv_dd_x,...` |
| | `Fth_x,...`, `Frd_x,...`, `Ftg_x,...` |
| Navegação | `x,y,sog,heading,cog,drift` |
| Controle | `rudder_angle_demanded_i`, `rudder_angle_effective_i` |
| | `thruster_rpm_demanded_i`, `thruster_rpm_effective_i` |
| Por hélice | `Fth_i_x,...` (6 DOF por hélice) |
| Por leme | `Frd_i_x,...` (6 DOF por leme) |
| Por rebocador | `tug_i_pos_x`, `tug_i_pos_y`, `tug_i_active`, `tug_i_force` |
| Manobra | `maneuver_stage` (índice do waypoint atual) |
