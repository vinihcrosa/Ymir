# Loop de Integração — CVODE e Transformações de Referencial

## Equação central

```
(M + A) · q̈ = Σ F(t, q, q̇)

onde:
  M = massMatrix (6×6, kg, kg·m, kg·m²)
  A = addedMass  (6×6, mesmas unidades)
  q  = vetor de posição/ângulo generalizado [x, y, z, roll, pitch, yaw]
  q̈  = acelerações generalizadas
  ΣF = soma de todas as forças e momentos externos (N, N·m)
```

A solução é:
```
q̈ = (M + A)⁻¹ · Ft
```

`invTotalMass` é calculada uma única vez no construtor de `Vessel` por eliminação de Gauss-Jordan. Usada a cada passo sem recalcular.

---

## CVODE — configuração

**Biblioteca**: SUNDIALS CVODE v5.8.0  
**Método**: BDF (Backward Differentiation Formula) — estável para sistemas stiff  
**Solver linear**: denso (SUNMatrix_Dense + SUNLinSol_Dense)  
**Estado**: 12 doubles (`N_Vector` serial)

```cpp
// Inicialização em BodyDyM::initSolver()
cvode_mem = CVodeCreate(CV_BDF);
CVodeInit(cvode_mem, rhs_sundials, t0, y);    // registra a função RHS
CVodeSStolerances(cvode_mem, reltol, abstol); // tolerâncias relativa e absoluta
CVodeSetMaxNumSteps(cvode_mem, maxNumSteps);  // teto de passos internos
CVodeSetMaxStep(cvode_mem, maxStep);          // passo máximo (0 = sem limite)

// Linear solver
A  = SUNDenseMatrix(N, N, ctx);
LS = SUNLinSol_Dense(y, A, ctx);
CVodeSetLinearSolver(cvode_mem, LS, A);

// Jacobiano: aproximado por diferenças finitas (padrão CVODE — não tem Jacobiano analítico)
```

**Avanço no tempo**:
```cpp
// Em BodyDyM::solve(dt):
double t_target = t_ + dt;
CVode(cvode_mem, t_target, y, &t_, CV_NORMAL);
// CV_NORMAL = avança até exatamente t_target, usando quantos passos internos forem necessários
```

CVODE pode usar passos menores que `dt` internamente (adaptativo). O `dt` passado é o intervalo de output, não o passo de integração.

---

## Função RHS — `rhs_sundials` em `SundialsInterface.cpp`

Chamada pelo CVODE. Assinatura exigida:

```cpp
int rhs_sundials(realtype t, N_Vector y, N_Vector ydot, void* user_data)
```

`user_data` aponta para o objeto `RHS`.

**Implementação**:

```cpp
// 1. Extrai estado do vetor CVODE
vector<double> q(6),   q_dot(6);
for (int i = 0; i < 6; ++i) {
    q[i]     = NV_Ith_S(y, i);
    q_dot[i] = NV_Ith_S(y, i + 6);
}

// 2. Chama o RHS para obter acelerações no frame do navio
RHS* rhs = static_cast<RHS*>(user_data);
rhs->set_time(t);
vector<double> q_ddot = (*rhs)(q, q_dot);  // operator()

// 3. Transforma velocidades do frame do navio para frame inercial
double psi = q[5];  // yaw

// Surge/sway: rotação 2D pelo yaw
ydot[0] = cos(psi)*q_dot[0] - sin(psi)*q_dot[1];   // ẋ inercial
ydot[1] = sin(psi)*q_dot[0] + cos(psi)*q_dot[1];   // ẏ inercial
ydot[2] = q_dot[2];                                   // ż (heave — sem rotação)

// Ângulos: small-angle — sem transformação de Euler completa
ydot[3] = q_dot[3];  // roll rate
ydot[4] = q_dot[4];  // pitch rate
ydot[5] = q_dot[5];  // yaw rate

// Acelerações (no frame do navio — CVODE integra no frame do navio)
for (int i = 0; i < 6; ++i)
    ydot[6+i] = q_ddot[i];

return 0;  // sucesso
```

**Nota sobre small-angle**: Roll e pitch usam a aproximação `ṗ = φ̇`, `q̇ = θ̇`. A transformação completa seria via matriz de Euler, mas para roll/pitch pequenos (navios em operação normal) o erro é desprezível.

---

## `RHS::operator()` — fluxo completo

```cpp
vector<double> RHS::operator()(const vector<double>& q, const vector<double>& q_dot)
{
    // 1. Atualiza estado no Vessel (corrente, vento, hélices, lemes, EMA)
    bd_.updateStates(wt_, cur_time_, dt);

    // 2. Computa todas as forças → Ft[6]
    compute_loads();

    // 3. Aplica RAO motion (delta de posição por ondas)
    update_postion_from_rao();

    // 4. Retorna acelerações
    return compute_acceleration();
}

vector<double> RHS::compute_acceleration()
{
    // q_ddot = invTotalMass · Ft
    return DyM::Math::matVecProduct(bd_.invTotalMass, bd_.Ft);
}
```

---

## `update_position_from_rao`

Ondas de 1ª ordem induzem movimento de corpo rígido (RAO). Esse deslocamento é adicionado diretamente à posição, no frame inercial.

```cpp
void RHS::update_postion_from_rao()
{
    // delta_q_rao[0:6] = deslocamento RAO neste passo (q_rao_curr - q_rao_old)
    // Componentes 0,1 (surge/sway) precisam ser rotacionados pelo yaw para inercial

    double psi = bd_.q[5];
    double dx_rao = Fwv->delta_q_rao[0];  // surge RAO
    double dy_rao = Fwv->delta_q_rao[1];  // sway RAO

    bd_.q[0] += cos(psi)*dx_rao - sin(psi)*dy_rao;  // Δx inercial
    bd_.q[1] += sin(psi)*dx_rao + cos(psi)*dy_rao;  // Δy inercial
    bd_.q[2] += Fwv->delta_q_rao[2];  // heave
    bd_.q[3] += Fwv->delta_q_rao[3];  // roll
    bd_.q[4] += Fwv->delta_q_rao[4];  // pitch
    bd_.q[5] += Fwv->delta_q_rao[5];  // yaw
}
```

O filtro por DOF em `WaveForces::wave()`:
- `filterWvForce = {0,0,1,1,1,0}` → excitação de força só em heave/roll/pitch
- `filterRAOMov  = {1,1,0,0,0,1}` → RAO de posição só em surge/sway/yaw

---

## `Vessel::updateStates` — detalhes

Chamado todo passo antes de computar forças.

### 1. Corrente e vento para frame do navio

```cpp
// Velocidade da corrente/vento em frame inercial → frame do navio
double psi = q[5];
speedToWater[0] = vc_inercial[0]*cos(psi) + vc_inercial[1]*sin(psi) - q_dot[0];
speedToWater[1] = -vc_inercial[0]*sin(psi) + vc_inercial[1]*cos(psi) - q_dot[1];
// (velocidade relativa do fluido no frame do navio)
```

### 2. EMA de posição

```cpp
double alpha = 1.0 - exp(-deltaTime / 16.5);  // τ = 16.5 s
for (int i = 0; i < 6; ++i)
    q_avg[i] = alpha*q[i] + (1.0 - alpha)*q_avg[i];
```

`q_avg` é usado pelos modelos de onda para evitar que ruído de alta frequência nas posições perturbe a interpolação de tabelas.

### 3. Cinemática inercial

```cpp
u = q_dot[0]*cos(psi) - q_dot[1]*sin(psi);  // componente Leste da velocidade inercial
v = q_dot[0]*sin(psi) + q_dot[1]*cos(psi);  // componente Norte
sog = sqrt(u*u + v*v);
cog = atan2(u, v);  // 0=Norte, convenção náutica
driftAngle = wrap_to_pi(cog - q[5]);
```

### 4. Dinâmica de hélices (1ª ordem)

```cpp
// Para cada hélice i:
double Aa = 1.0 / (1.0 + rotationTime / dt);
rotation_current = Aa * rotation_current + (1 - Aa) * rotation_demanded;
// Idem para azimute e pitch_ratio
```

### 5. Rate limiter de leme

```cpp
double max_delta = angleSpeed * dt;  // graus/passo
error = demanded - current;
delta = clamp(error, -max_delta, max_delta);
current += delta;
```

---

## Parâmetros do solver CVODE — guia prático

| Parâmetro | Típico | Efeito |
|-----------|--------|--------|
| `reltol` | 1e-8 | Tolerância relativa. Reduzir → mais preciso, mais lento |
| `abstol` | 1e-10 | Tolerância absoluta. Importante para estados perto de zero |
| `maxNumSteps` | 10000 | Teto de sub-passos internos entre dois `CVode()` calls |
| `maxStep` | 0.0 (sem limite) | Limita passo interno. Útil se sistema tem descontinuidades |
| `dt` (output) | 0.05–0.1 s | Intervalo de output, não passo de integração |
