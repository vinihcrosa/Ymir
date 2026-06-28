# Utilitários Matemáticos — `DyM::Math` e Operações de Suporte

## Namespace `DyM::Math` — `src/basic/DyM.h`

Header-only. Incluído por todos os módulos de força.

---

## Interpolação

### `linear_interpolate(x, y, x_query)`

Interpolação linear 1D com clamp nos extremos:
```
se x_query <= x.front(): retorna y.front()
se x_query >= x.back():  retorna y.back()
senão: interpola entre os dois pontos mais próximos
```

### `gradient(y, x)` — 1D

Gradiente numérico por diferenças centradas (extremos por diferenças unilaterais):
```
grad[0]   = (y[1] - y[0]) / (x[1] - x[0])          // forward
grad[N-1] = (y[N-1] - y[N-2]) / (x[N-1] - x[N-2])  // backward
grad[i]   = (y[i+1] - y[i-1]) / (x[i+1] - x[i-1])  // centered
```

### `gradient(data, axis, along_columns)` — 2D

Versão matricial. `along_columns=true` deriva na direção das colunas, `false` na direção das linhas.

---

## Álgebra Linear

### `invert_matrix_6x6(mat)`

Inversão por eliminação de Gauss-Jordan:
- Montagem de `[A | I]`
- Pivoteamento sem troca de linha (assume não-singular)
- Retorna `I` transformado em `A⁻¹`
- Threshold de singularidade: `|pivot| < 1e-12` → throw

Chamada uma vez no construtor de `Vessel`. **Nunca chamada por passo de tempo.**

### `matVecProduct(A, x, scale=1.0)`

```
result[i] = scale · Σⱼ A[i][j] · x[j]     (6×6 × 6 → 6)
```

### `matVecQuadratic(A, x, scale=1.0)`

```
result[i] = scale · Σⱼ A[i][j] · x[j] · |x[j]|
```

Usado pelo amortecimento quadrático.

### `tons2kg_6x6(M)` e `tons2kg_NxM(M)`

Multiplica todos os elementos por 1000. Usado na leitura de matrizes de massa (input em toneladas).

---

## Transformações de Ângulo

### `deg2rad(degrees)` e `rad2deg(radians)`

Escalares e vetores.

`rad2deg` normaliza para `[0, 360)`:
```
angle_deg = rad * 180/π
return fmod(angle_deg + 360, 360)
```

### `normalize_degrees(deg)` → `[0, 360)`

```
result = fmod(deg, 360)
if result < 0: result += 360
```

### Wrapping de ângulos

```
wrap2_360(deg): [0, 360) — módulo 360
wrap2_180(deg): [-180, 180) — módulo 360 com offset
wrap2_2pi(rad): [0, 2π) — módulo 2π
wrap2_pi(rad):  [-π, π) — módulo 2π com offset
```

---

## Transformação de Referencial

### `Trans_L(yaw, pitch, roll)` → `array<array<double,3>,3>`

Matriz de rotação 3×3 — transforma do frame inercial para o frame do navio.

Ordem de rotação: yaw (Z) → pitch (Y) → roll (X) — convençao ZYX:
```
M[0][0] = cp·ch     M[0][1] = sr·sp·ch - cr·sh     M[0][2] = cr·sp·ch + sr·sh
M[1][0] = cp·sh     M[1][1] = sr·sp·sh + cr·ch     M[1][2] = cr·sp·sh - sr·ch
M[2][0] = -sp       M[2][1] = sr·cp                M[2][2] = cr·cp

onde: sh=sin(yaw), ch=cos(yaw), sp=sin(pitch), cp=cos(pitch), sr=sin(roll), cr=cos(roll)
```

### `rotMatrixYaw(angle)` → `array<array<double,6>,6>`

Rotação 6×6 apenas em yaw (para transformar vetores 6-DOF):
```
[cos(ψ)  -sin(ψ)  0  0  0  0]
[sin(ψ)   cos(ψ)  0  0  0  0]
[0        0       1  0  0  0]
[0        0       0  1  0  0]
[0        0       0  0  1  0]
[0        0       0  0  0  1]
```

---

## Integrador RK4 embutido

### `RK4(Y, dt, compute_acceleration)`

Integrador Runge-Kutta de 4ª ordem para EDOs de 2ª ordem escritas como sistema de 1ª ordem.

Estado: `Y[2N]` onde `Y[0:N]` = posições, `Y[N:2N]` = velocidades.

```
// Função auxiliar interna:
// rhs(Y) → {q_dot, q_ddot}
//   q_dot  = Y[N:2N]  (velocidades)
//   q_ddot = compute_acceleration(Y[0:N], Y[N:2N])

k1 = rhs(Y)
k2 = rhs(Y + 0.5·dt·k1)
k3 = rhs(Y + 0.5·dt·k2)
k4 = rhs(Y + dt·k3)

Y += (dt/6) · (k1 + 2·k2 + 2·k3 + k4)
```

**Nota**: Este RK4 é usado pelo `fasttimeRkDyM` (app legado). O solver principal usa CVODE/BDF. O Van der Pol em `CurrentForces` tem seu próprio RK4 inline manual.

---

## Utilitários gerais

### `linspace(start, stop, num)` → `vector<double>`

Similar ao `numpy.linspace`. Inclui start e stop.

### `clamp(v, lo, hi)`

```
return max(lo, min(hi, v))
```

### `round3(x)`

Arredonda para 3 casas decimais:
```
return round(x * 1000) / 1000
```

### `max_value(v)`

Máximo de um vetor usando `std::max_element`.

### Impressão de debug

```cpp
print_vector(vec, label, precision)    // imprime [v1, v2, ...]
print_matrix(mat, label, precision)    // imprime linha por linha
```

---

## Convenções de normalização de ângulo de incidência

Diferentes módulos usam convenções ligeiramente diferentes:

| Módulo | Convenção |
|--------|-----------|
| Corrente (Obokata) | `atan2(vc_y, vc_x)` em graus, não normalizado |
| Vento | `atan2(v_y, v_x)` → `fmod(deg + 360, 360)` → `[0, 360)` |
| Onda (incidência) | `(450 - wave_dir - yaw_deg + 360) % 360` → `[0, 360)` |
| Leme (ângulo de ataque) | `atan2(v_rel, u_rel)` em radianos |
| Leme (incidência no foil) | `wrap360(-(delta - alpha) · 180/π)` |

A inconsistência existe. O código funciona porque cada tabela de coeficientes foi gerada com a mesma convenção do seu módulo.
