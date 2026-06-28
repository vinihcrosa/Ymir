# Motor de Ondas — Espectro, Spreading e Forças de Onda

## Visão geral

O sistema de ondas tem duas camadas:

1. **`SurfaceWaves`** — gera o espectro de frequências e spreading direcional a partir de parâmetros de mar (Hs, Tp, Dr, tipo).
2. **`WaveComponent`** — para cada componente espectral, aplica as tabelas WAMIT do navio e calcula as 4 cargas: excitação (1ª ordem), mean drift, slow drift, drift damping + RAO motion.
3. **`WaveForces`** — agrega N `WaveComponent` (um por componente de mar) e soma tudo.

---

## Espectro de frequência — `SurfaceWaves`

**Arquivo**: `src/weather/SurfaceWaves.cpp`

### Parâmetros de entrada

```
Hs    : altura significativa de onda (m)
Tp    : período de pico (s)
Dr    : direção predominante (graus náuticos — de onde vem)
spectrumType : "JONSWAP" | "PIERSON" | "REGULAR"
omega : vetor de frequências angulares (rad/s) — definido pelo usuário
gamma : fator de pico JONSWAP (padrão: 3.3)
alfa  : parâmetro de Phillips (0 = calcular automaticamente)
```

### Construção do espectro

```cpp
// Para cada frequência omega[i]:
deltaOmega[i] = omega[i] - omega[i-1]   (banda de frequência)
nWave[i] = omega[i]² / g                 (número de onda pela relação de dispersão: k = ω²/g)
lambda[i] = 2π / nWave[i]               (comprimento de onda)

// Espectro de densidade de energia S(ω) [m²·s/rad]
spectrum[i] = wave_spectrum(spectrumType, omega[i], Hs, Tp, gamma, alfa)

// Energia por bin [m²]
amplitude2[i] = 2 · S(ω[i]) · ΔΩ[i]

// Amplitude de onda [m]
amplitude[i] = sqrt(amplitude2[i]) = sqrt(2·S·ΔΩ)

// Fase aleatória
phase[i] = rand() * 2π    (uniforme em [0, 2π])
```

### Espectro REGULAR

Onda monocromática. Só usa 2 bins:
```
omega = {2π/Tp, 1.01·2π/Tp}
amplitude = {sqrt(2·S[0]), 0}
```

### Coeficientes de slow drift

```
// Vetor de frequências de baixa frequência (drift lento)
nSlowOmega = 2 · N
waux[i] = (2π/Tp/6) · (i+1)/nSlowOmega   → distribuído em [0, wp/6]
slowOmega[i] = waux[i]  (ligeiramente perturbado)
nSlowWave[i] = slowOmega[i]²/g

// Coef. de espectro para drift lento — proporcional a S²
slowDriftSpectrumCoeff[i] = 8 · deltaOmega[i] · S[i]² · deltaSlow
```

---

## Spreading direcional — `apply_spreading()`

Distribui a energia de cada frequência em N_dir = 18 direções ao redor da direção principal.

### Função de spreading (Mitsuyasu cos-2S)

```
// Parâmetro S(ω): expoente do cosseno — depende de ω/ωp
if ω > ωp: S = s_max · (ω/ωp)^(-2.5)
else:      S = s_max · (ω/ωp)^5
// s_max padrão: definido no header (tipicamente 10)

// Distribuição angular G(θ, S)
G(θ, S) = C(S) · cos(θ/2)^(2S)
C(S) = (1/π) · 2^(2S-1) · Γ(S+1)² / Γ(2S+1)   (normalização de Mitsuyasu)
θ ∈ [-π/2, +π/2]  (ângulos relativos à direção principal)
```

### Discretização

```
N_dir = 18
dtheta = π / N_dir    (≈ 10°)

// Ângulos absolutos de cada bin direcional
dir_angles[j] = Dr + (-π/2 + dtheta/2 + j·dtheta) · 180/π    (graus)

// Para cada (ω[i], θ[j]):
norm = Σⱼ G(θⱼ, S) · dtheta    (renormalização sobre ±90° — preserva energia)

amplitude_dir[i][j]  = amplitude[i] · sqrt(G(θⱼ, S) · dtheta / norm)
amplitude2_dir[i][j] = amplitude_dir[i][j]²

// Fase: ou compartilhada com a 1D (independent_dir_phase=false)
//       ou independente por bin (independent_dir_phase=true)
if independent_dir_phase:
    phase_dir[i][j] = rand() · 2π
else:
    phase_dir[i][j] = phase[i]   (mesma fase em todas as direções)
```

Após apply_spreading, `amplitude_dir[nOmega][N_dir]` substitui `amplitude[nOmega]` como fonte de energia.

---

## WaveForces — Filtro por DOF

**Arquivo**: `src/waveForces/WaveForces.cpp`

Após somar todos os WaveComponent:
```cpp
filterWvForce = {0.0, 0.0, 1.0, 1.0, 1.0, 0.0}
filterRAOMov  = {1.0, 1.0, 0.0, 0.0, 0.0, 1.0}
```

- **Excitação de força** (1ª ordem): só heave (2), roll (3), pitch (4)
- **RAO de posição**: só surge (0), sway (1), yaw (5)

Razão: os movimentos horizontais de baixa frequência (surge/sway/yaw) são melhor modelados pelo slow drift + mean drift. A excitação de 1ª ordem nesses DOFs seria redundante com o RAO de posição.

```
delta_q_rao[i] = q_rao_curr[i] - q_rao_old[i]   (incremento de posição neste passo)
q_rao_old = q_rao_curr                            (mantém estado entre passos)
```

---

## WaveComponent — Cálculo por componente espectral

**Arquivo**: `src/waveForces/WaveComponent.cpp`

Para cada componente de mar (um `WaveComponent` por `SurfaceWaves`), na inicialização:

### `wave_coefficients()` — pré-processamento

Interpola as tabelas WAMIT do navio para as frequências do espectro atual.

```
// Tabelas do navio: indexadas por [dof][omega_WAMIT_idx][angle_WAMIT_idx]
// Precisam ser re-interpoladas para as frequências do espectro (omega[])

// Para mean drift (DOFs 0, 1, 5 — surge/sway/yaw):
mean_drift_[dof][omega_idx][angle_idx] = interp_1d(wv_omega_WAMIT, mdForceAmplitude[dof]·cos(phase), omega[omega_idx])

// Para RAO force (todos 6 DOF):
// Converte amplitude/phase para partes real/imaginária
rao_force_re_[dof][omega_idx][angle_idx] = interp_1d(wv_omega, Amp·cos(Phase), omega[omega_idx])
rao_force_im_[dof][omega_idx][angle_idx] = interp_1d(wv_omega, Amp·sin(Phase), omega[omega_idx])

// Para RAO movement (todos 6 DOF):
rao_mov_re_, rao_mov_im_ — idem

// Derivadas para drift damping (diferenças finitas em ω e em ângulo)
dD_dω[omega_idx][angle_idx]  — derivada da força de drift em relação a ω
dD_da[omega_idx][angle_idx]  — derivada em relação ao ângulo de incidência

// Coeficientes Bw e Br (drift damping integrado sobre todo o espectro)
Bw[angle] = Σᵢ (4·D[i] + ω[i]·∂D/∂ω[i]) · amplitude2[i] · ω[i]/g
Br[angle] = Σᵢ ±2 · ∂D/∂α[i] · amplitude2[i] · ω[i]/g
// (sinal: -1 para irregular, +1 para REGULAR)
```

### `computeWaveComponents(time)` — por passo de tempo

#### Ângulo de incidência

```
wave_dir   = (wv_.Dr + 180°) % 360°   // "de onde vem" → "para onde vai"
wave_inc   = (450° - wave_dir - yaw_deg + 360°) % 360°   // relativo ao navio
// Usa q_avg[5] (yaw suavizado) para evitar ruído
```

#### Fase espacial e temporal por frequência

```
local_wave_dir = (450° - wave_dir) % 360°
wave_dir_cos = cos(local_wave_dir)
wave_dir_sin = sin(local_wave_dir)

// Distância projetada do navio na direção da onda
kd = q_avg[0] · wave_dir_cos + q_avg[1] · wave_dir_sin

// Fase total = fase de espaço + fase de tempo + fase aleatória
zeta[i] = -omega[i]·time + phase[i] + nWave[i]·kd
```

#### Mean drift (DOFs 0, 1, 5 — surge/sway/yaw)

```
// Determinístico: soma ponderada sobre todas as frequências
for omega_idx:
    D = interp_angle(mean_drift_[dof][omega_idx], idx_angle, t_angle)
    mean_drift[dof] += amplitude2[omega_idx] · D
```

#### Slow drift (DOFs 0, 1, 5)

```
// Fase das oscilações de baixa frequência (batimento entre componentes)
cos_zl = Σᵢ cos(nSlowWave[i]·kd - slowOmega[i]·time + slowPhase[i])

for omega_idx:
    slow_drift_spectrum += slowDriftSpectrumCoeff[omega_idx] · D²

slow_drift[dof] += sqrt(2·|slow_drift_spectrum|) · cos_zl
```

#### Excitação de 1ª ordem (DOFs 2, 3, 4 — heave/roll/pitch)

```
// Superposição de ondas: A(ω,β)·cos(ωt + φ + k·kd - fase_RAO)
for omega_idx:
    Re = interp_angle(rao_force_re_[dof][omega_idx], ...)
    Im = interp_angle(rao_force_im_[dof][omega_idx], ...)
    
    f_amp   = amplitude[omega_idx] · sqrt(Re² + Im²)
    f_phase = zeta[omega_idx] - atan2(Im, Re)
    
    excitation[dof] += f_amp · cos(f_phase)
```

#### RAO motion (DOFs 0, 1, 5 — surge/sway/yaw)

```
// Mesma estrutura que excitação, mas usa rao_mov_re_/im_
for omega_idx:
    Re = interp_angle(rao_mov_re_[dof][omega_idx], ...)
    Im = interp_angle(rao_mov_im_[dof][omega_idx], ...)
    
    M_amp   = amplitude[omega_idx] · sqrt(Re² + Im²)
    M_phase = zeta[omega_idx] - atan2(Im, Re)
    
    rao_movement[dof] += M_amp · cos(M_phase)
```

#### Drift damping (DOFs 0, 1, 5)

```
// Amortecimento proporcional à velocidade relativa à água
angle = deg2rad(wave_inc)
Bw = interp(bw_x_1d_, idx_angle, t_angle)  // coef. de frequência
Br = interp(br_x_1d_, idx_angle, t_angle)  // coef. de direção

drift_damping[0] = speedToWater[0]·(cos(α)·Bw_x + sin(α)·Br_x)
                 + speedToWater[1]·(sin(α)·Bw_x - cos(α)·Br_x)
// Idem para sway e yaw
```

---

## WaveAPI — C API do motor de ondas standalone

**Arquivo**: `include/WaveAPI.h`, `applications/wavesEngineLib/wavesEngineLib.cpp`

Para uso externo (C#, Python, etc.) sem instanciar o navio completo.

### Struct de saída

```c
typedef struct {
    double direction;    // ângulo da componente (graus náuticos)
    double omega;        // frequência angular (rad/s)
    double phase;        // fase aleatória (rad)
    double wavelength;   // comprimento de onda (m)
    double amplitude;    // amplitude (m)
    double amplitude2;   // amplitude² = energia (m²)
} WaveData;
```

### Fluxo de uso

```c
// 1. Reset estado interno (limpa componentes anteriores)
wave_engine_reset();

// 2. Opcional: habilitar fase independente por direção
wave_engine_set_dir_phase(1);

// 3. Para cada componente de mar, chamar wave_engine():
double omega[50] = {0.1, 0.14, ...};  // vetor de frequências
wave_engine(
    2.0,            // Hs (m)
    10.0,           // Tp (s)
    45.0,           // Dr (graus)
    "JONSWAP",      // spectrum_type
    omega,          // omega_array
    50,             // N
    1               // spreading: 0=sem, 1=com
);

// Chamar múltiplas vezes acumula componentes (swell + wind sea)
wave_engine(1.5, 7.0, 90.0, "JONSWAP", omega, 50, 1);

// 4. Opcional: reduzir número de linhas (poda por energia)
wave_engine_prune(500);   // mantém no máximo 500 entradas

// 5. Ler resultados
size_t N = wave_get_N();
const WaveData* data = wave_get_all();
for (size_t i = 0; i < N; i++) {
    // data[i].direction, data[i].omega, etc.
}
```

### `wave_engine_prune(max_rows)`

Reduz o número de entradas mantendo as mais energéticas.

```
// Para cada componente c:
slots[c] = max_rows · energy[c] / total_energy   (proporcional à energia Hs²)
// Distribui sobra por largest-remainder
// Dentro de cada componente: ordena por amplitude decrescente, mantém slots[c] entradas
```

**Bug conhecido**: após o prune, `g_components[i].start` e `.count` ficam desatualizados. Segunda chamada a `prune` usa índices stale → UB. Correção: reconstruir `g_components` após o prune.

---

## Relação de dispersão

Usada para calcular o número de onda a partir da frequência angular (profundidade infinita):

```
k = ω² / g     (águas profundas — válido quando kh >> 1)
λ = 2π / k
```

Para águas rasas seria necessário resolver `ω² = g·k·tanh(k·h)` iterativamente — não implementado no código atual.
