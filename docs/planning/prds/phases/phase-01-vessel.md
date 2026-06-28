# Fase 1 — Vessel Layer

## Objetivo

Implementar o sistema de controle e o estado operacional das embarcações. Ao final desta fase, uma embarcação pode navegar de forma autônoma por waypoints, executar manobras de atracação com rebocadores, reproduzir comandos gravados, e expor seu estado operacional completo (luzes, shapes, modo).

---

## Estado Atual (entrada da fase)

O que já existe:
- `VesselConfig` — configurações físicas (massa, propulsores, lemes, fairleads)
- `NavalContext` — plano de dados passado às forças por tick
- `NavalSimulation` — orquestrador básico sem sistema de controle
- Todos os modelos de força funcionando (inclusive `ThrustForces`, `RudderForces`, `TugForces`)

O que falta (esta fase entrega):
- `ManeuverController` — autopiloto LOS + PID
- `BerthManeuverSystem` — FSM de atracação com rebocadores
- `PrescribedController` — replay de séries temporais
- `VesselState` — luzes, shapes, estado operacional
- Integração dos controladores no tick de `NavalSimulation`
- `updateControl` e `updateStates` no ciclo de simulação

---

## Escopo

### Incluído nesta fase

- `ManeuverController` (modo `maneuver`) — navegação autônoma por waypoints
- `BerthManeuverSystem` (modo `berthManeuver`) — atracação com rebocadores
- `PrescribedController` (modo `simulation`) — replay de comandos gravados
- `VesselState` — estado operacional visível
- `Vessel::updateControl(time, dt)` — seleção e execução do controlador ativo
- `Vessel::updateStates(time, dt)` — atualização de atuadores e cinemática derivada
- Integração dos dois métodos no tick de `NavalSimulation` (passos 3 e 4)
- Testes: waypoint tracking, FSM de atracação, replay, luzes/shapes

### Excluído desta fase

- World / queries espaciais (Fase 2)
- Terrain / batimetria (Fase 2)
- Mooring e Anchoring (Fase 4)
- Colisões (Fase 5)
- Server / API externa (Fase 3)

---

## Módulos

### ManeuverController

Autopiloto por waypoints. Ativo quando `controlMode = maneuver`.

```
Algoritmo LOS + PID por tick:
1. bearing = atan2(wp.x - x, wp.y - y)              -- LOS bearing para próximo waypoint
2. heading_err = wrap180(bearing - yaw)              -- erro de heading
3. rudder_cmd = Kp·err + Ki·∫err·dt + Kd·Δerr/dt   -- PID → demanda de leme
4. speed_err = v_demanded - SOG                      -- erro de velocidade
5. rpm_cmd = PID(speed_err)                          -- PID → demanda de RPM
6. Se dist(vessel, wp) < captureRadius → próximo wp
7. Se lista de waypoints esgotada → modo drift
```

**Interface de configuração:**
- `waypoints[]` — lista de `{x, y, v_demanded}`
- `captureRadius` (m) — raio de captura do waypoint
- PID gains: `Kp_heading`, `Ki_heading`, `Kd_heading`, `Kp_speed`, `Ki_speed`, `Kd_speed`

**Output por tick:**
- `rudder_cmd` → demanda de leme em graus
- `rpm_cmd` → demanda de RPM por propulsor

---

### BerthManeuverSystem

FSM para atracação com rebocadores. Ativo quando `controlMode = berthManeuver`.

**Estados FSM:**

| Estado | `controllerType` | Comportamento |
|--------|-----------------|---------------|
| `navigating` | LOS+PID | Aproximação: heading + velocidade via leme/propulsor. Rebocadores em ESCORTING. |
| `sideway` | heading fixo | Deslocamento lateral puro. Rebocadores PUSH controlam erro lateral via PD. Heading mantido por leme. |
| `turnROTTUG` | PID de ROT | Rotação no lugar. PID de Rate of Turn via leme. Rebocadores PUSH para controle de sway. |

**Transições:**
- `navigating → sideway`: quando dist < threshold E heading_err < tolerance
- `sideway → turnROTTUG`: quando lateral_err < threshold
- Cada waypoint de atracação carrega `controllerType` que define o estado

**Interface de configuração:**
- `berthWaypoints[]` — `{x, y, heading_target, controllerType}`
- Gains PID por estado
- `tugAssignments[]` — qual rebocador executa qual papel por estado

---

### PrescribedController

Replay de séries temporais de comandos pré-gravados. Ativo quando `controlMode = simulation`.

```
Por tick dado tempo t:
  rudder_cmd[i] = interpolate(timeSeries.rudder[i], t)   -- interpolação linear
  rpm_cmd[i]    = interpolate(timeSeries.rpm[i], t)      -- interpolação linear
```

**Interface de configuração:**
- `timeSeries.rudder[][]` — `{t, delta_deg}` por leme
- `timeSeries.rpm[][]` — `{t, rpm}` por propulsor
- Quando `t > max(timeSeries.t)` → mantém último valor

**Uso principal:** comparação com dados físicos reais, reprodução de manobras calibradas.

---

### VesselState

Estado operacional visível da embarcação. Não executa cálculos.

**Luzes de navegação:**
- `mastLight` (topo de mastro — branca, visível à frente)
- `portLight` (bombordo — vermelha)
- `starboardLight` (estibordo — verde)
- `sternLight` (popa — branca)
- `anchorLight` (fundeio — branca ao topo)
- `specialLights[]` — configurável

**Shapes diurnos (COLREGS):**
- `ball` — fundeado
- `cone` — à vela com motor auxiliar
- `cylinder` — rebocando
- `diamond` — manobra restrita
- `shapes[]` — lista ativa

**Estado operacional:**
```
enum class OperationalState {
    Underway,
    Anchored,
    Moored,
    Aground,
    RestrictedManeuverability,
    NotUnderCommand
};
```

**Transições automáticas:**
- `Anchored` quando âncora unhada (evento `anchor_holding`)
- `Moored` quando amarrado (evento `mooring_attached`)
- Estado manual via API

---

### Vessel::updateControl e updateStates

Dois métodos chamados no tick de `NavalSimulation` antes de qualquer cálculo de força.

**`updateControl(time, dt)` — passo 3 do tick:**
- Seleciona controlador ativo (`drift`/`simulation`/`maneuver`/`berthManeuver`)
- Executa controlador → gera `rudder_cmd[]` e `rpm_cmd[]`
- Escreve demands em `NavalContext`

**`updateStates(time, dt)` — passo 4 do tick:**
```
// Atuadores
Para cada propulsor i:
  Aa = 1 / (1 + rotationTime[i] / dt)
  n_current[i] = Aa·n_current[i] + (1-Aa)·n_demanded[i]

Para cada leme j:
  delta = clamp(az_demanded[j] - az_current[j], -speed[j]·dt, +speed[j]·dt)
  az_current[j] += delta

// Conversão corrente/vento para frame do navio
current_body = R(yaw) · current_inercial
wind_body    = R(yaw) · wind_inercial

// Cinemática derivada
SOG         = sqrt(ẋ_i² + ẏ_i²)
COG         = atan2(ẋ_i, ẏ_i)
driftAngle  = wrap_pi(COG - yaw)
speedToWater[0] = u - current_body[0]
speedToWater[1] = v - current_body[1]
```

---

## Integração no Tick de NavalSimulation

Após esta fase, o tick de `NavalSimulation` deve executar:

```
1. updateControl(t, dt)         ← NOVO (Fase 1)
2. updateStates(t, dt)          ← NOVO (Fase 1)
3. Todas as forças (1-10)       ← já existe
4. CVODE integra                ← já existe
5. Eventos básicos (simplificado) ← já existe
```

O tick completo de 12 passos com `Environment`, `Terrain`, `Mooring`, `Anchoring` e `Events` completo é escopo da Fase 2.

---

## Acceptance Criteria

1. Navio navega por lista de waypoints do ponto A ao B sem desvio > captureRadius
2. FSM de atracação completa todos os 3 estados em sequência e para corretamente
3. PrescribedController reproduz série temporal gravada com erro < 1e-6 (interpolação)
4. `updateStates` aplica filtro de 1ª ordem correto no RPM (verificável analyticamente)
5. `VesselState` transitions disparam corretamente para eventos de âncora e amarração
6. Nenhum tick de `NavalSimulation` falha com sistema de controle ativo
7. Cobertura de testes ≥ 80% para cada controlador

---

## Decisões Abertas para TechSpec

- `ManeuverController` vive em `libs/vessel/` ou `libs/simulation/`? (tendência: `libs/vessel/`)
- `VesselState` é parte de `VesselConfig` ou struct separada?
- Anti-windup no integrador PID — necessário desde o início?
- `BerthManeuverSystem` recebe posição de rebocadores de onde? (depende de como Fase 2 expõe World)
- Serialização de waypoints no JSON scenario reader — extensão desta fase ou da Fase 6?
