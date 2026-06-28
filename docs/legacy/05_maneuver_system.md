# Sistema de Manobra — Controle e Autopiloto

## Módulos disponíveis

Selecionado por `controlDict.json → simulation.module`:

| Módulo | Comportamento |
|--------|---------------|
| `"drift"` | Sem propulsão. Navio deriva sob vento/corrente/ondas. |
| `"simulation"` | Forças externas pré-definidas. Sem autopiloto. |
| `"maneuver"` | Autopiloto segue waypoints com PID de heading/velocidade. |
| `"berthManeuver"` | FSM de atracação com apoio de rebocadores. |

---

## `ManeuverSystem` — Manobra genérica

**Arquivo**: `src/bridge/ManeuverSystem.cpp`, `src/controller/ManeuverController.cpp`

### Estrutura de waypoints (`maneuverParameters.json`)

```json
{
  "waypoints": [
    {
      "x": 100.0,
      "y": 200.0,
      "heading": 90.0,
      "speed": 5.0,
      "captureRadius": 20.0,
      "controllerType": "navigating"
    },
    ...
  ]
}
```

### ManeuverController — LOS + PID

Algoritmo LOS (Line-of-Sight) para seguir waypoints:

```
1. Calcula bearing até o waypoint atual
   LOS_bearing = atan2(wp.x - x, wp.y - y)   (convenção náutica)

2. Erro de heading
   heading_err = wrap_180(LOS_bearing - heading)

3. PID de heading → comando de leme
   rudder_cmd = Kp·err + Ki·∫err·dt + Kd·(err - err_prev)/dt
   rudder_cmd = clamp(rudder_cmd, -max_rudder, +max_rudder)

4. Controle de velocidade → RPM
   speed_err = v_demanded - SOG
   rpm_cmd = Kp_v·speed_err + ...

5. Transição de waypoint
   if dist(vessel, wp) < captureRadius: wp_idx++
```

---

## `BerthManeuverSystem` — Atracação com rebocadores

**Arquivo**: `src/bridge/BerthManeuverSystem.cpp`  
**Controlador**: `src/controller/BerthManeuverController.cpp`

### FSM — três modos por waypoint

Cada waypoint em `maneuverParameters.json` tem `"controllerType"` que seleciona o modo:

#### Modo `"navigating"` — Navegação em rota

Algoritmo LOS com controle de velocidade. Rebocadores em modo ESCORTING (sem força).

```
// Heading demand = LOS bearing para o waypoint
heading_demand = losBearing(vessel_pos, waypoint)
heading_err    = wrap180(heading_demand - measured_heading)

// PID de heading → leme
rudder = Kp_h · heading_err + Ki_h · ∫heading_err · dt

// Controle de velocidade → RPM acumulado (F_surge_)
speed_err = v_demanded - SOG
F_surge_ += Kp_v · speed_err   (acumulo de força → converte para RPM)
rpm_cmd = F_surge_ / thrust_por_rpm   (simplificado)
```

#### Modo `"sideway"` — Translação lateral (aproximação à berma)

Heading fixo (sem mudança de proa). Rebocadores empurram lateralmente em direção ao waypoint.

```
// Mantém heading do waypoint anterior
heading_err = wrap180(wp.heading - measured_heading)
rudder = Kp_h · heading_err + Ki_h · ∫heading_err + Kd_h · (err - err_prev)/dt

// Erro lateral ao waypoint
lat_err = dist_transversal(vessel_pos, waypoint)

// Força de rebocador acumulada (F_sway_)
F_sway_ += Kp_lat · lat_err + Kd_lat · (lat_err - lat_err_prev)/dt
// Converte F_sway_ em thrust_demanded para cada rebocador
tug[i].thrust_demanded = F_sway_ / n_tugs / bollardPull[i]

// RPM reduzido (quasi-zero — só mantém posição longitudinal)
```

#### Modo `"turnROTTUG"` — Giro com rebocadores

Controle de ROT (Rate of Turn) via PID + empurrão lateral de rebocadores para posicionamento sway.

```
// ROT medida (filtrada por low-pass)
rot_smoothed = α · rot_smoothed + (1-α) · q_dot[5]  // α depende de τ_filter e dt

// PID de ROT → leme
rot_demanded = wp.rot_demanded   (graus/s)
rot_err = rot_demanded - rot_smoothed
rudder = Kp_rot · rot_err + Kd_rot · (rot_err - rot_err_prev)/dt

// Sway: empurrão de rebocador por erro lateral
lat_err = dist_transversal(vessel_pos, waypoint)
F_sway_ += Kp_lat · lat_err
tug[i].thrust_demanded = ...
```

### Transição de waypoints

```
dist = sqrt((wp.x - x)² + (wp.y - y)²)
if dist < wp.captureRadius:
    wp_idx++
    // Reseta integradores de PID
    heading_err_integ = 0
    tug_err_integ     = 0
    // Reinicializa F_surge_ se novo wp for "sideway"
```

### Controle de rebocadores por modo

| Modo | Tug mode | O que controla |
|------|----------|---------------|
| navigating | ESCORTING | Nada (só escolta) |
| sideway | PUSH | `thrust_demanded` proporcional a erro lateral |
| turnROTTUG | PUSH | `thrust_demanded` proporcional a erro lateral |

---

## Actuators — Dinâmica de atuadores

**Arquivo**: `src/actuators/`

Três classes, todas herdam de `Actuator`:

### `FilterActuator` — 1ª ordem (low-pass)
```
y_next = Aa · y + (1-Aa) · demand
Aa = 1 / (1 + tau/dt)
```
Usado para: rotação de hélice, thrust de rebocador.

### `RateLimitedActuator` — Limitador de taxa
```
delta = demand - current
delta = clamp(delta, -rate_max·dt, +rate_max·dt)
current += delta
```
Usado para: ângulo de leme, azimute de hélice.

Parâmetros de atuador por elemento:
- Hélice: `rotationTime` (s) — constante de tempo 1ª ordem
- Leme: `speed` (graus/s) — taxa máxima de variação
- Hélice azimutal: `azimuth_speed` (graus/s)
- Hélice CP (passo controlável): `pitch_speed` (unidades/s)

---

## PrescribedController

**Arquivo**: `src/bridge/PrescribedController.cpp`

Alternativa ao autopiloto. Carrega uma série temporal pré-gravada de comandos e reproduce exatamente. Usado para replay de manobras reais ou comparação com resultados físicos.

```
// Interpolação linear em tempo
for each command channel:
    value = interp(time_series, values, current_time)
    apply to demands
```

---

## ManeuverRecorder

**Arquivo**: `src/bridge/ManeuverRecorder.cpp`

Grava estados e comandos durante a simulação para posterior análise ou replay.

---

## Fluxo de controle em `fasttimeDyM.cpp`

```cpp
// Inicialização
for each body:
    sims[i].initialize(w)
    sims[i].initSolver(...)
    sims[i].v.updateStates(w, 0, dt)

// Loop principal
for step = 1 to Nsteps:
    for each body i:
        if module == "maneuver":
            sims[i].v.updateControl(time, dt)   // ← atualiza demands de leme/RPM
        sims[i].solve(dt)                        // ← CVODE avança um passo
    
    time = step * dt
    
    if step % writeInterval == 0:
        writers[i].set_outputs(out_step, time)

// Condição de parada antecipada (Tfinal=0)
if all maneuvers finished: break

// Saída
for each writer: writer.write_csv()
for each sim:    sim.cleanup()   // libera memória CVODE
```

### `updateControl(time, dt)`

Chama a FSM de manobra para atualizar `demandMap` no `Vessel`:

```cpp
// demandMap: mapa de demands indexado por ID de atuador
// ROTATION id = propIndex*3
// PITCH    id = propIndex*3 + 1
// AZIMUTH  id = propIndex*3 + 2
// RUDDER   id = rudder_base_id + rudderIndex
```

Os modelos de força leem `demandMap` indiretamente via `Propeller::rotationSpeed()`, `Rudder::rudderDeflectionAngle()`, etc.
