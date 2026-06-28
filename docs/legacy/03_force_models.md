# Modelos de Força — Implementação Detalhada

Todos os modelos de força recebem `const Vessel& bd` (referência constante) e leem o estado atual. Nenhum deles modifica o estado — só retornam um vetor `loads[6]` = `[Fx, Fy, Fz, Mx, My, Mz]` em N e N·m.

---

## 1. InertialForces — Acoplamento Coriolis/Giroscópico

**Arquivo**: `src/inertialForces/InertialForces.cpp`

Captura os termos de inércia não-lineares que surgem quando o navio gira enquanto se move. Não é "força de inércia" no sentido de F=ma — são os termos de acoplamento que aparecem quando as equações de Newton são escritas no frame do navio.

### Implementação

```
u  = q_dot[0]  (surge velocity)
v  = q_dot[1]  (sway velocity)
ψ̇  = q_dot[5]  (yaw rate)
Vc = speedToWater  (velocidade relativa da corrente no frame do navio)

M11 = totalMass[0][0]  (massa total surge + adicionada)
M22 = totalMass[1][1]  (massa total sway + adicionada)
ΔXa = addedMass[0][0] - addedMass[1][1]  (assimetria de massa adicionada)

Fi[0] (surge)  = ψ̇ · (M22 · v + ΔXa · Vc[1])
Fi[1] (sway)   = ψ̇ · (-M11 · u + ΔXa · Vc[0])
Fi[2] (heave)  = 0
Fi[3] (roll)   = m · [cg_z·(u·ψ̇ - w·ṗ) + cg_y·(u·θ̇ - v·ṗ)]
Fi[4] (pitch)  = m · [cg_x·(v·ṗ - u·θ̇) + cg_z·(v·ψ̇ - w·θ̇)]
Fi[5] (yaw)    = ψ̇ · (Xa'[0][5]·v - Xa'[1][5]·u)   ← giroscópico de massa adicionada
```

onde `Xa'[i][j]` = `addedMass[i][j]` (notação de coeficiente de massa adicionada).

---

## 2. RestoringForces — Mola Hidrostática

**Arquivo**: `src/restoringForces/RestoringForces.cpp`

Força restauradora de Arquimedes: ao sair da posição de equilíbrio, o navio recebe uma força proporcional ao deslocamento.

### Implementação

```
delta_q[i] = q[i] - wavesOriginPosition[i]   (para i = 0,1)
delta_q[2] = q[2] - wavesOriginPosition[2] + draft - tide  (heave com maré e calado)
delta_q[3:6] = q[3:6]                         (ângulos absolutos)

Fr[i] = -hydro_rest[i][i] · delta_q[i]    (só diagonal usada)

// Correção de heave: peso menos empuxo
Fr[2] -= (mass · g - volumetricWeight)

// Roll: momento do empuxo sobre o CG
Fr[3] += (Fr[2] · cf[1] - mass·g·(cg[1] - wavesOriginPosition[1])) · cos(roll)

// Pitch: idem
Fr[4] -= (Fr[2] · cf[0] - mass·g·(cg[0] - wavesOriginPosition[0])) · cos(pitch)
```

**`hydro_rest`**: matriz de rigidez hidrostática (N/m para translações, N·m/rad para rotações). Tipicamente não-zero em: `[2][2]` (heave), `[3][3]` (roll), `[4][4]` (pitch). Diagonal pelo WAMIT.

**`cf`**: centro de flutuação (m) — onde a força de Arquimedes age.  
**`cg`**: centro de gravidade (m) — onde o peso age.

---

## 3. DampingForces — Amortecimento Hidrodinâmico

**Arquivo**: `src/dampingForces/DampingForces.cpp`

Três componentes físicas diferentes:

### 3.1 Potential damping (radiação)
Energia irradiada como ondas pela oscilação do navio. Linear, depende da velocidade corporal:
```
Fd_potential[i] = -Σⱼ damping_potential[i][j] · q_dot[j]
```

### 3.2 Linear damping com modulação exponencial
Amortecimento viscoso linear, mas modulado para se anular a velocidades altas (onde o quadrático domina):
```
vNorm2 = Σ aux_velocity[i]²
decayFactor = exp(-linearDampingCoeff · vNorm2)

aux_velocity[0] = -speedToWater[0]  (surge: usa velocidade relativa à água)
aux_velocity[1] = -speedToWater[1]  (sway: idem)
aux_velocity[2:6] = q_dot[2:6]      (outros DOFs: velocidade corporal)

exp_velocity = aux_velocity · decayFactor   (só surge e sway são modulados)

Fd_linear[i] = -Σⱼ damping_linear[i][j] · exp_velocity[j]
```

O `linearDampingCoeff` controla quão rapidamente o linear se anula. Valor típico: 0.1–1.0.

### 3.3 Quadratic damping
Arrasto proporcional a v·|v| (forma de Morison):
```
Fd_quadratic[i] = -Σⱼ damping_quadratic[i][j] · q_dot[j] · |q_dot[j]|
```

### Total
```
Fd[i] = Fd_potential[i] + Fd_linear[i] + Fd_quadratic[i]
```

---

## 4. SquatForces — Efeito de Águas Rasas (ICORELS)

**Arquivo**: `src/squatForces/SquatForces.cpp`

Modelo ICORELS: em águas rasas, o navio afunda (squat) devido ao aumento de velocidade do fluxo sob o casco. O afundamento é convertido em força vertical via rigidez hidrostática.

### Implementação

```
depth = max(|water_depth + tide|, |q[2]|)

v = sqrt(speedToWater[0]² + speedToWater[1]²)  (velocidade sobre o fundo)
Fn = v / sqrt(g · depth)  (número de Froude de profundidade)

// Coeficiente Cs por bloco
if Cb < 0.7: Cs = 1.7
if Cb < 0.8: Cs = 2.0
else:        Cs = 2.4

// Coeficiente Cf por Froude
if Fn > 0.7: Cf = 0.3; Fn = min(Fn, 0.8)
else:        Cf = 0.0

// Sinkage (m)
s = -(Cs + Cf) · (∇ / ρ·g·L²) · Fn² / sqrt(1 - Fn²)
s = max(-depth - 0.1 - q[2], s)  (não afunda mais que o fundo)

Fsq[2] = hydro_rest[2][2] · s   (força vertical = rigidez heave × sinkage)
```

onde `∇ = volumetricWeight / (ρ·g)` é o volume de deslocamento.

---

## 5. CurrentForces — Arrasto de Corrente

**Arquivo**: `src/currentForces/CurrentForces.cpp`

Dois modelos selecionáveis via `body.fsiModels.current`.

### 5.1 Modelo OBOKATA (padrão)

Integra o arrasto ao longo de seções transversais do casco. Captura o momento de yaw induzido pelo gradiente de velocidade rotacional.

```
// Parâmetros globais
sub_depth = wavesOriginPosition[2] - q[2]  (calado efetivo)
hydro_scale = 0.5 · ρ · (L/n_sections) · sub_depth
dL = L / n_sections

// Para cada seção i em [0, n_sections]:
section_x = q[0] + sectionLocalPosition[i] · cos(ψ)
section_y = q[1] + sectionLocalPosition[i] · sin(ψ)

// Velocidade local (inclui acoplamento com yaw rate)
vc_sec[0] = speedToWater[0]
vc_sec[1] = speedToWater[1] - q_dot[5] · sectionLocalPosition[i]
// ← o yaw cria corrente transversal diferente em proa e popa

angle_inc_sec = atan2(vc_sec[1], vc_sec[0])  (graus)

// Interpola coeficientes por ângulo de incidência
cd_x_sec = interp(Cdc_incAngle, Cdc_x, angle_inc_sec)
cd_y_sec = interp(Cdc_incAngle, Cdc_y, angle_inc_sec)

Fc[0] += cd_x_sec · (vc_sec[0]² + vc_sec[1]²)  → somado por seção
Fc[1] += cd_y_sec · (vc_sec[0]² + vc_sec[1]²)
Fc[5] += (cd_y_sec · v²_sec - cd_y_mk · v²_mk) · sectionLocalPosition[i]

// Escala todos
for i: Fc[i] *= hydro_scale

// Momento adicional de yaw (termo de arrasto de meia-nau)
Fc[5] += 0.5·ρ·L·sub_depth·v²_mk · (L·cd_z_mk + cd_y_mk·(midshipDist - origin_x))

// Momentos de roll e pitch por braço de alavanca
Fc[3] = -Fc[1] · (frontalHeight - origin_z) · cos(roll) · cos(pitch)
Fc[4] =  Fc[0] · (lateralHeight - origin_z) · cos(roll) · cos(pitch)
```

### 5.2 Modelo REGULAR

Baseado em área projetada (similar ao vento), com adição de oscilador Van der Pol para modelar VIM (Vortex-Induced Motion).

```
// Correção de área por afundamento (orientação 3D com Trans_L)
Syz = frontalArea ± ΔA   (área corrigida por posição vertical)
Sxz = lateralArea ± ΔA

// Coeficientes VIM por Van der Pol (ver seção 5.3)
CL1 = vanDerPolSolverCross_(...)   // coef. transversal
CL2 = vanDerPolSolverInline_(...)  // coef. longitudinal

Fc[0] = 0.5·ρ · Syz · (cd_x + CL2) · v²_mk
Fc[1] = 0.5·ρ · Sxz · (cd_y + CL1) · v²_mk
Fc[5] = 0.5·ρ · Sxz · L · cd_z · v²_mk + Fc[1]·(midshipDist - origin_x)
```

### 5.3 Oscilador Van der Pol (VIM)

Modela desprendimento periódico de vórtices. Resolvido por RK4 a cada passo:

```
// Equação de Van der Pol:
ÿ + A·(y² - 1)·ẏ + B·y = f(t)

// Parâmetros calculados da velocidade de corrente e da frequência de Strouhal:
freq = 2π · St · |Vc| / B₀          (frequência natural de desprendimento)
St   = vdp_coeff_.st                 (número de Strouhal — parâmetro de entrada)
B₀   = beam                          (largura do navio)

// Para oscilação transversal (cross-flow):
A = Rz = ez · freq
B = Qz = freq²
f = az / B₀ · acc[0]

// O estado do oscilador {qz, dqzdt} é persistido entre passos (muda a cada timestep)
// RK4 com h = dt/6

// Coeficiente resultante:
CL1 = clO · qz / 2
```

---

## 6. WindForces — Arrasto de Vento

**Arquivo**: `src/windForces/WindForces.cpp`

Forma: `F = 0.5 · ρ_ar · Cd(β) · A · |V_rel|²`

```
// Velocidade aparente do vento no frame do navio (corrigida por yaw rate)
vwd_rel[0] = speedToWind[0] - q_dot[5] · wavesOriginPosition[1]
vwd_rel[1] = speedToWind[1] + q_dot[5] · wavesOriginPosition[0]

// Ângulo de incidência
angle_inc = atan2(vwd_rel[1], vwd_rel[0])  (normalizado 0..360°)

// Coeficientes interpolados
cd_x = interp(Cdwd_incAngle, Cdwd_x, angle_inc)
cd_y = interp(Cdwd_incAngle, Cdwd_y, angle_inc)
cd_z = interp(Cdwd_incAngle, Cdwd_z, angle_inc)

// Área exposta
if windModel == "ACSINKAGE":
    dz = q[2] - (wavesOriginPosition[2] - draft)  // sinkage correction
    f_area = windFrontalArea + beam · dz
    l_area = windLateralArea + length · dz
else:  // REGULAR
    f_area = windFrontalArea  (fixo)
    l_area = windLateralArea

v2 = vwd_rel[0]² + vwd_rel[1]²

Fwd[0] (surge)  = 0.5 · ρ_ar · cd_x · f_area · v2
Fwd[1] (sway)   = 0.5 · ρ_ar · cd_y · l_area · v2
Fwd[5] (yaw)    = 0.5 · ρ_ar · cd_z · l_area · L · v2 + Fwd[1]·(midshipDist - origin_x)
Fwd[3] (roll)   = -Fwd[1] · (frontalHeight - origin_z) · cos(roll) · cos(pitch)
Fwd[4] (pitch)  =  Fwd[0] · (lateralHeight - origin_z) · cos(roll) · cos(pitch)
```

`ρ_ar` = 1.225 kg/m³ (definido em `PhysicalProps`).

---

## 7. ThrustForces — Propulsão

**Arquivo**: `src/thrustForces/ThrustForces.cpp`  
**Detalhes da curva**: `src/thrustForces/OpenWaterCurve.cpp`

Modelo de propulsor quasi-estático baseado em curvas de água aberta (open-water curves).

### Coeficiente de avanço J

```
Va = relativeVelocity()[0]    // velocidade axial de avanço (wake-corrected)
if Va > 0: Va *= hullEfficiency   // wake fraction

n  = rotationSpeed / 60.0    // rev/s (convertido de RPM)
D  = diameter                // diâmetro (m)
P  = pitchRatio              // relação passo/diâmetro

J = Va / (n · D)             // coeficiente de avanço
```

### Empuxo e torque por interpolação

```
// Lookup table: J → Kt(J), Kq(J)
Kt = interp(J_table, Kt_table, J)
Kq = interp(J_table, Kq_table, J)

T = Kt · ρ · n² · D⁴          // empuxo (N)
Q = Kq · ρ · n² · D⁵          // torque (N·m) — não usado diretamente nas forças
```

### Redução de empuxo por velocidade transversal

```
// Coeficiente de redução: vale 1 se v_lateral < limite, cai até 0
thrustReduction = (|v_lat| <= v_lim) ?
    (1 - |v_lat|/v_lim) · (1 - 2·(1 - 2·red_coeff)·|v_lat|/v_lim)
    : 0

thrust = T · thrustReduction · sign(pitchRatio)

if sign(n) · sign(P) < 0:
    thrust *= asternEfficiency   // ineficiência a ré
```

### Efeito Paddle

Quando o hélice está a ré, gera uma força lateral (efeito de remo):
```
if thrust < 0:
    v_ship = sqrt(u² + v²)
    paddleForce = c0 · (1 + v_ship/(2·c1)) · |P·n/n_max|^c2
```

### Cargas no frame do navio

```
// Posição local do hélice: [x_prop, y_prop, z_prop, azimuth_rad]
xt = thrust · cos(azimuth) + paddleForce · cos(azimuth + π/2)
yt = thrust · sin(azimuth) + paddleForce · sin(azimuth + π/2)
zt = 0

// Momentos em relação à origem WAMIT
kt = -yt · (z_prop - origin_z)                         // roll
mt =  xt · (z_prop - origin_z)                         // pitch
nt =  yt·(x_prop - origin_x) - xt·(y_prop - origin_y) // yaw
```

### Dinâmica de controle (em `Vessel::updateStates`)

```
// 1ª ordem para velocidade de rotação:
Aa = 1/(1 + rotationTime/dt)
n_current = Aa · n_current + (1-Aa) · n_demanded

// Limitação de potência
P_atual = |Q · 2π · n|
if P_atual > maximumPowerW:
    n_demanded *= maximumPowerW / P_atual

// Rate limiter para azimute (azimuth_speed em graus/s):
delta = clamp(az_demanded - az_current, -azimuth_speed·dt, +azimuth_speed·dt)
az_current += delta
```

---

## 8. RudderForces — Leme

**Arquivo**: `src/rudderForces/RudderForces.cpp`

Modelo de perfil hidrodinâmico (foil). O leme gera sustentação e arrasto em função do ângulo de ataque.

```
// Velocidade de inflow no leme
if tem_hélice_associado:
    u_rel = ThrustForces::computeUpstreamAxialVelocity(slipStreamFactor)
    // Teoria do disco atuador: velocidade induzida pelo hélice
    // Vi = Va se Va > 0 (com wake)
    // p2 = sqrt(Va² + 8·|T|/(ρ·π·D²)) - Va
    // u_rel = Va + factor · p2
else:
    u_rel = -(currentLocalVelocity[0] - localVelocity[0])

v_rel = -(currentLocalVelocity[1] - localVelocity[1])

v2 = u_rel² + v_rel²  (se < 1e-12, retorna zero)

// Ângulo de ataque
alpha = atan2(v_rel, u_rel)

// Incidência no foil: beta = -(rudderDeflection - alpha) em graus [0..360]
beta = wrap360(-(delta_rudder - alpha) · 180/π)

// Cl e Cd por interpolação na tabela
Cl = interp(angle_table, Cl_table, beta)
Cd = interp(angle_table, Cd_table, beta)  (sinal negativo na tabela)

// Forças
Fl = 0.5 · ρ_água · area · v2 · Cl   (sustentação — normal ao escoamento)
Fd = 0.5 · ρ_água · area · v2 · Cd   (arrasto — ao longo do escoamento)

// Decomposição no frame do navio
xr = Fd·cos(alpha) + Fl·cos(alpha - π/2)
yr = Fd·sin(alpha) + Fl·sin(alpha - π/2)

// Momentos em relação à origem WAMIT
kr = -yr·(z_leme - origin_z)
mr =  xr·(z_leme - origin_z)
nr =  yr·(x_leme - origin_x) - xr·(y_leme - origin_y)
```

---

## 9. TugForces — Rebocadores

**Arquivo**: `src/tugForces/TugForces.cpp`

Cada rebocador tem estado próprio (`Tug` struct) com thrust, direção e distância ao navio.

### Dinâmica de estado (chamada a cada passo em `update(dt)`)

```
// 1. Thrust — 1ª ordem, ramp-down 2× mais rápido
speedFactor = (thrust > thrust_demanded) ? 2.0 : 1.0
Aa = 1/(1 + 5·dt/(thrustTime/speedFactor))
thrust = Aa·thrust + (1-Aa)·thrust_demanded

// 2. Direção — rate limiter com limite de velocidade angular
dif = wrap_pi(direction_demanded - direction)
rate_max = min(3·maxDirSpeed, maxSpeed/(distance+0.1))
daz = min(|dif|, |rate_max|·dt)
direction += daz · sign(dif)

// 3. Distância — 1ª ordem (line shortening em modo PULL)
if PULL mode:
    bp = bollardPull · 0.1
    f = max(hypot(loads[0],loads[1]), |thrust_demanded|·bollardPull)
    shortening = max(0, 2·(bp-f)/bp)
    distance_desired = line_length - shortening
else (PUSH):
    distance_desired = 0

Aa_d = 1/(1 + 5·dt/pushPullTime)
distance = Aa_d·distance + (1-Aa_d)·distance_desired

// 4. Posição global do rebocador
gfl_x = cos(ψ)·fairlead[0] - sin(ψ)·fairlead[1] + q[0]   // fairlead em inercial
gfl_y = sin(ψ)·fairlead[0] + cos(ψ)·fairlead[1] + q[1]
tug_x = gfl_x + distance·cos(ψ - direction)
tug_y = gfl_y + distance·sin(ψ - direction)
```

### Cálculo de força

```
vessel_speed = hypot(q_dot[0], q_dot[1])   // m/s
vessel_speed_kts = vessel_speed · 1.94384   // para lookup 2D

// Modos:
if ESCORTING: loads = 0 (rebocador apenas escoltando)

if PUSH:
    fpush = eval_1D_table(fpush_angles, fpush_force, eff_dir_deg)  // normalizado [0..1]
    sf = speedPushFactor(vessel_speed, direction)
    // sf = ((1 - tanh(v/v_ref - C·π)) / 2)^exponent — cai a zero em alta velocidade
    resultant = fpush · sf · thrust · bollardPull  // N
    xt = -resultant · cos(direction)
    yt =  resultant · sin(direction)

if PULL:
    fpull = eval_2D_table(fpull_angles, fpull_speeds, fpull_force,
                          eff_dir_deg, vessel_speed_kts)
    resultant = fpull · thrust · bollardPull
    azimuth = -direction
    xt = resultant · cos(azimuth)
    yt = resultant · sin(azimuth)

// Momento de yaw (ponto de aplicação = fairlead)
nt = yt · fairlead[0] - xt · fairlead[1]
```

**`speedPushFactor`**: função tanh que modela a perda de eficiência de empurrão quando o navio está em velocidade. Para rebocador convencional: `typeFactor=1.15`, `maxSpeedFactor=s_max/5`. Para Z-drive: `typeFactor=2.2`, `maxSpeedFactor=s_max/8`.

---

## Referência rápida: constantes físicas

Definidas em `PhysicalProps` (herdada por todos os modelos de força):

```cpp
double gravity()    { return 9.81; }       // m/s²
double rho_water()  { return 1025.0; }     // kg/m³ (água do mar)
double rho_air()    { return 1.225; }      // kg/m³
```
