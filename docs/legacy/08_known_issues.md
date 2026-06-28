# Problemas Conhecidos e TODOs no Código

## Bugs confirmados

### BUG-1: `wave_engine_prune` — estado corrompido após segunda chamada

**Arquivo**: `applications/wavesEngineLib/wavesEngineLib.cpp:115`  
**Severidade**: Crash / Undefined Behavior

Após a primeira chamada a `wave_engine_prune()`, `g_waveData` é substituído pelo vetor podado, mas `g_components[i].start` e `g_components[i].count` ficam com os valores antigos.

Se `wave_engine_prune()` for chamado uma segunda vez, ele calcula:
```cpp
auto begin = g_waveData.begin() + g_components[i].start;  // índice STALE
auto end   = begin + g_components[i].count;                // count STALE
```
→ Acesso fora de bounds → UB → crash.

**Correção**:
```cpp
// Adicionar ao final de wave_engine_prune(), antes de retornar:
size_t offset = 0;
for (size_t i = 0; i < n; ++i) {
    g_components[i].start = offset;
    g_components[i].count = slots[i];
    offset += slots[i];
}
```

---

### BUG-2: Fase errada no CSV do programa exemplo `wavesEngine.cpp`

**Arquivo**: `applications/wavesEngineLib/wavesEngine.cpp:~105`  
**Severidade**: Dado incorreto no output (só afeta o exemplo, não a lib)

No branch de spreading direcional, o CSV escreve `swave.phase[i]` (fase 1D, não-direcional) em vez de `swave.phase_dir[i][j]`:

```cpp
// ERRADO (atual):
fout << swave.phase[i] << ","

// CORRETO:
fout << swave.phase_dir[i][j] << ","
```

`wavesEngineLib.cpp` já usa `swave.phase_dir[i][j]` corretamente. Só o exemplo está errado.

---

### BUG-3: `rand()` sem srand — fases determinísticas no executável

**Arquivo**: `src/weather/SurfaceWaves.cpp`  
**Severidade**: Baixa (comportamento não-documentado)

```cpp
phase[i] = static_cast<double>(rand()) / RAND_MAX * 2 * M_PI;
```

`rand()` sem `srand()` usa semente padrão (0). Toda execução gera as mesmas fases. Para simulações estocásticas, isso pode ser intencional (reprodutibilidade) ou bug dependendo do contexto.

Para irregularidade real: chamar `srand(time(NULL))` ou usar `std::mt19937` com semente aleatória. O gerador interno do espectro irregular já usa `std::mt19937` com semente fixada (0) — explicitamente para reprodutibilidade.

---

## TODOs no código (comentários `// TODO`)

### `RestoringForces.cpp:23`
```cpp
// TODO 2025/09/2023: Clean this mess: bd.wavesOriginPosition[2]
delta_q[2] = bd.q[2] - bd.wavesOriginPosition[2] + bd.draft - bd.tide;
```
A fórmula mistura coordenadas de duas origens. Funcionalmente correto mas confuso.

### `CurrentForces.cpp` (Obokata)
```cpp
// TODO 2025/09/2023: Check the draft sign
sub_depth_ = bd_.wavesOriginPosition[2] - bd_.q[2];
```
Sinal depende da convenção de z (positivo pra cima). Verificar em casos com heel significativo.

### `SquatForces.cpp:19-22`
```cpp
// TODO: Get actual depth
depth_ = bd_.water_depth + bd_.tide;
// TODO: Check if it is correct
depth_ = max(abs(depth_), abs(bd_.q[2]));
```
Lógica de profundidade efetiva em maré com variação de calado pode estar incompleta.

### `WaveComponent.cpp:144-162`
```cpp
// Process mean drift forces for surge (0), sway (1), and yaw (5) DOFs
```
Mean drift só processado para 3 DOFs. Heave/roll/pitch são computados via excitação + RAO, não via mean drift. Design intencional mas não documentado.

---

## Limitações de modelo

### Relação de dispersão — águas profundas apenas
```
k = ω²/g   (válida quando k·h >> 1)
```
Para h < λ/4 (~h < π·g/2ω²), o erro é significativo. Não há correção de águas rasas no motor de ondas.

### Small-angle para roll/pitch
`SundialsInterface.cpp` usa `ṗ ≈ φ̇`, `q̇ ≈ θ̇` diretamente. Para roll/pitch > ~20°, a transformação de Euler completa seria necessária.

### Amortecimento linear só diagonal
```cpp
// Comentado em RestoringForces e outros:
// for (int j = 0; j < 6; ++j) loads[i] -= bd.hydro_rest[i][j] * delta_q[j];
```
Acoplamentos off-diagonal da matriz de rigidez hidrostática ignorados. Válido para navios simétricos, mas impreciso para plataformas com geometria complexa.

### Van der Pol — parâmetros empíricos
Os coeficientes VIM (St, ez, az, ey, ay, clO, cd0, ci0, k) são empíricos e dependem da geometria do navio. Sem calibração específica, o modelo pode dar resultados qualitativos mas não quantitativos.

### Propulsão — sem efeito de interação entre hélices
Múltiplos propulsores são computados independentemente. Interações hidrodinâmicas (slipstream de uma hélice atingindo outra) não modeladas.

### Rebocador — geometria simplificada
```cpp
// Simplified — no tug hull length/beam offset
tug_.position[0] = gfl_x + tug_.distance * cos(psi - tug_.direction);
```
O rebocador é tratado como um ponto no final do cabo. Sem colisão, sem geometria de casco.

---

## Pontos de atenção para extensão

**Adicionar novo modelo de força**:
1. Criar `src/newForce/NewForce.h` e `.cpp` herdando de `PhysicalProps`
2. Adicionar `optional<NewForce> Fnew` em `BodyDyM`
3. Instanciar em `BodyDyM::initializeForces_()`
4. Adicionar `NewForces& Fnew_` em `RHS`
5. Chamar `Fnew_.compute()` em `RHS::compute_loads()`
6. Somar ao `bd_.Ft[i]`
7. Adicionar campo `Fnew[6]` em `Vessel` para tracking
8. Adicionar colunas de output em `IO_OutputWriter`

**Adicionar novo módulo de manobra**:
1. Criar controlador em `src/controller/`
2. Adicionar case em `ManeuverSystem` ou criar novo `XyzManeuverSystem`
3. Adicionar string de módulo em `controlDict.json` parsing
4. Adicionar branch em `fasttimeDyM.cpp` (ou criar novo executável em `applications/`)
