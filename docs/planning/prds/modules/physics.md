# Contexto: Física

Responsável por toda dinâmica de corpos rígidos e forças.
Calcula como cada corpo se move em resposta a forças internas e externas.

---

## Physics

Núcleo da simulação de corpos rígidos.

### Equação central

```
(M + A) · q̈ = ΣF(t, q, q̇)

M = massMatrix (6×6)   — massa do navio
A = addedMass  (6×6)   — massa adicionada hidrodinâmica
q  = [x, y, z, roll, pitch, yaw]
q̈  = acelerações generalizadas
ΣF = soma de todos os módulos de força [Fx, Fy, Fz, Mx, My, Mz]
```

Solução: `q̈ = (M + A)⁻¹ · ΣF` — `invTotalMass` calculada uma vez no construtor por eliminação de Gauss-Jordan.

### Integrador

**SUNDIALS CVODE v5.8.0** — método BDF (Backward Differentiation Formula), estável para sistemas stiff.

- Estado: 12 doubles — `[x, y, z, roll, pitch, yaw, ẋ, ẏ, ż, ṙoll, ṗitch, ẏaw]`
- Solver linear interno: denso (SUNMatrix_Dense)
- Jacobiano: aproximado por diferenças finitas (sem Jacobiano analítico)
- `dt` passado ao integrador é o intervalo de output — CVODE usa sub-passos adaptativos internamente
- Parâmetros típicos: `reltol=1e-8`, `abstol=1e-10`, `maxNumSteps=10000`

### Referenciais

- Velocidades calculadas no **frame do navio** (body-fixed)
- Posições armazenadas no **frame inercial** (Earth-fixed)
- Conversão por rotação 2D no yaw: `ẋ_inercial = cos(ψ)·u - sin(ψ)·v`
- Roll e pitch: aproximação small-angle (`ṗ ≈ φ̇`, `q̇ ≈ θ̇`) — válido para operação normal de navios

### 6 DOFs

| DOF | Símbolo | Direção |
|-----|---------|---------|
| Surge | u | longitudinal |
| Sway | v | transversal |
| Heave | w | vertical |
| Roll | p | rotação em x |
| Pitch | q | rotação em y |
| Yaw | r | rotação em z (proa) |

### Responsabilidades

- Integrar movimento linear e angular via CVODE
- Acumular e aplicar forças de todos os módulos (cada módulo retorna `loads[6]`)
- Atualizar posição e orientação no World a cada tick
- Detectar e resolver colisões

### Ordem de execução das forças por tick

```
1. InertialForces    — acoplamento Coriolis/giroscópico
2. RestoringForces   — mola hidrostática (heave, roll, pitch)
3. DampingForces     — amortecimento (potencial + linear + quadrático)
4. SquatForces       — efeito de águas rasas (ICORELS)
5. CurrentForces     — arrasto de corrente
6. WindForces        — arrasto de vento
7. ThrustForces      — propulsão (curvas Kt/Kq)
8. RudderForces      — leme (modelo de foil Cl/Cd)
9. TugForces         — rebocadores (PUSH/PULL/ESCORTING)
10. WaveForces       — ondas (excitação 1ª ordem + drift + RAO)
```

---

## Hydrodynamics

Calcula forças e torques produzidos pela interação casco-água.

### Amortecimento hidrodinâmico (DampingForces)

Três componentes somados:

```
Fd = Fd_potential + Fd_linear + Fd_quadratic

Fd_potential[i] = -Σⱼ B_pot[i][j] · q̇[j]          — radiação (energia em ondas)
Fd_linear[i]    = -Σⱼ B_lin[i][j] · v[j] · exp(-c·‖v‖²)  — viscoso, se anula em v alto
Fd_quadratic[i] = -Σⱼ B_quad[i][j] · q̇[j] · |q̇[j]|       — forma de Morison
```

### Restauração hidrostática (RestoringForces)

Mola de Arquimedes. Força proporcional ao deslocamento da posição de equilíbrio.
Diagonal da matriz de rigidez hidrostática (`hydro_rest`): non-zero em heave, roll, pitch.
Inclui correção de maré e deslocamento de centro de flutuação vs. centro de gravidade.

### Correntes (CurrentForces)

Dois modelos selecionáveis:

**Modelo OBOKATA (padrão)** — integração por seções transversais:
- Captura yaw rate acoplado com arrasto (velocidade local difere em proa e popa)
- Interpola coeficientes Cd por ângulo de incidência em cada seção
- Gera forças em surge, sway e yaw

**Modelo REGULAR** — por área projetada com VIM (Vortex-Induced Motion):
- Coeficientes Cd escalados por área frontal/lateral
- Oscilador Van der Pol (RK4) para modelar desprendimento periódico de vórtices
- Parâmetros: número de Strouhal (St), coeficientes de amortecimento do oscilador

### Efeito de águas rasas — Squat (SquatForces)

Modelo ICORELS:
```
Fn = v / sqrt(g · h)        — Froude de profundidade
Cs = f(Cb)                   — coeficiente por bloco (1.7–2.4)
s  = -Cs · (∇/L²) · Fn² / sqrt(1 - Fn²)   — afundamento (m)
Fsq[heave] = hydro_rest[2][2] · s
```

### Propulsão (ThrustForces)

Modelo quasi-estático baseado em curvas de água aberta (open-water curves):
```
J  = Va / (n · D)         — coeficiente de avanço
T  = Kt(J) · ρ · n² · D⁴  — empuxo (N)
Q  = Kq(J) · ρ · n² · D⁵  — torque (N·m)
```

Dinâmica de atuador (1ª ordem, em `Vessel::updateStates`):
```
Aa = 1 / (1 + rotationTime/dt)
n_current = Aa · n_current + (1-Aa) · n_demanded
```

Inclui: redução de empuxo por velocidade transversal, efeito paddle (força lateral a ré), limitação de potência máxima.

Propulsores azimutais: rate limiter para azimute (`azimuth_speed` graus/s).

### Leme (RudderForces)

Modelo de foil hidrodinâmico (Cl/Cd):
```
alpha = atan2(v_rel, u_rel)                    — ângulo de ataque
beta  = wrap360(-(delta_rudder - alpha))        — incidência no foil
Fl = 0.5 · ρ · area · v² · Cl(beta)            — sustentação
Fd_drag = 0.5 · ρ · area · v² · Cd(beta)       — arrasto
```

Velocidade de inflow considera slipstream do propulsor associado (teoria do disco atuador).
Rate limiter: `speed` graus/s para ângulo de leme.

### Rebocadores (TugForces)

Três modos:
- **PUSH** — empurrão direto, força reduz com velocidade do navio (`speedPushFactor` tanh)
- **PULL** — tração por cabo, força via tabela 2D (ângulo × velocidade), cable shortening 1ª ordem
- **ESCORTING** — sem força, apenas acompanha

Dinâmica de thrust: FilterActuator 1ª ordem (ramp-down 2× mais rápido que ramp-up).

---

## Mooring

Simula conexões físicas por cabos de amarração entre corpos ou entre corpo e ponto fixo.

**Responsabilidades:**
- Conectar entidades via cabos
- Calcular tensão em função do afastamento
- Simular elasticidade do cabo
- Limitar deslocamentos
- Simular ruptura quando tensão excede limite

**Inputs (por tick):**
- Posições dos pontos de conexão (do World)
- Propriedades do cabo (comprimento, rigidez, carga de ruptura)

**Outputs (por tick):**
- Forças de tração nos pontos de conexão → entregues ao Physics
- Evento de ruptura → publicado em Events

---

## Anchoring

Simula fundeio: âncora, amarra e interação com o fundo.

**Responsabilidades:**
- Lançar e recolher âncoras
- Detectar contato da âncora com o fundo (usando Terrain)
- Determinar quando a âncora está unhada
- Calcular força de retenção em função da catenária da amarra
- Detectar garreio (âncora arrastando pelo fundo)

**Inputs (por tick):**
- Posição da embarcação e ponto de fundeio (do World)
- Batimetria local (do Terrain)
- Propriedades da amarra (comprimento, peso linear)

**Outputs (por tick):**
- Força de retenção → entregues ao Physics
- Eventos: âncora unhada, âncora garreando → publicados em Events
