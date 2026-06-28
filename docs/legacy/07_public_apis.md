# APIs Públicas — C Interface para Embedding

## WaveAPI — Motor de ondas standalone

**Header**: `include/WaveAPI.h`  
**Implementação**: `applications/wavesEngineLib/wavesEngineLib.cpp`  
**Biblioteca**: `libwaveEngineDyMLib.dylib` (macOS) / `.so` (Linux) / `.dll` (Windows)

### Struct de dados

```c
typedef struct {
    double direction;    // graus náuticos — de onde a componente se propaga
    double omega;        // frequência angular (rad/s)
    double phase;        // fase aleatória (rad) — [0, 2π)
    double wavelength;   // comprimento de onda λ (m) — λ = 2π·g/ω²
    double amplitude;    // amplitude de onda a (m) — a = sqrt(2·S·Δω)
    double amplitude2;   // a² = energia por componente (m²)
} WaveData;
```

### Funções exportadas

```c
// Reset completo — limpa todas as componentes acumuladas
void wave_engine_reset();

// Habilita/desabilita fase independente por bin direcional
// enable=0: fase compartilhada entre direções (mais rápido)
// enable=1: fase independente por (omega, direction) — mais realista
void wave_engine_set_dir_phase(int enable);

// Gera e acumula uma componente de mar
// Pode ser chamada múltiplas vezes para compor mar complexo (swell + wind sea)
void wave_engine(
    double height,           // Hs (m)
    double period,           // Tp (s)
    double direction,        // graus náuticos
    const char* spectrum_type,  // "JONSWAP" | "PIERSON" | "REGULAR"
    const double* omega_array,  // vetor de frequências (rad/s)
    size_t N,                // tamanho do vetor
    int spreading            // 0=sem, 1=com spreading direcional
);

// Poda: mantém no máximo max_rows entradas, priorizando por energia
// DEVE ser chamado após todos os wave_engine() e antes de wave_get_all()
void wave_engine_prune(size_t max_rows);

// Getters
double        wave_get_direction();  // direção da última componente adicionada
size_t        wave_get_N();          // número total de entradas em g_waveData
const WaveData* wave_get_all();      // ponteiro para array interno (válido até próximo wave_engine())
```

### Estado interno (global, static)

```cpp
static double               g_direction  = 0.0;
static vector<WaveData>     g_waveData;
static vector<ComponentInfo> g_components;  // metadados de cada wave_engine() call
static bool                 g_dir_phase  = false;
```

### Ciclo de uso correto

```c
wave_engine_reset();
wave_engine_set_dir_phase(1);

double omega[50];
for (int i = 0; i < 50; i++) omega[i] = 0.1 + i * 0.04;   // 0.1..2.06 rad/s

// Componente 1: swell primário
wave_engine(2.5, 14.0, 220.0, "JONSWAP", omega, 50, 1);

// Componente 2: vento-mar
wave_engine(1.2, 7.0, 180.0, "JONSWAP", omega, 50, 1);

// Componente 3: swell secundário (mais fraco)
wave_engine(0.8, 20.0, 200.0, "JONSWAP", omega, 50, 0);

// Reduz para uso eficiente (ex: simulador em tempo real)
wave_engine_prune(300);

// Lê resultados
size_t n = wave_get_N();
const WaveData* d = wave_get_all();
for (size_t i = 0; i < n; i++) {
    printf("dir=%.1f omega=%.3f amp=%.4f\n", d[i].direction, d[i].omega, d[i].amplitude);
}
```

---

## RealtimeDyMAPI — Simulador em tempo real

**Header**: `include/RealtimeDyMAPI.h`  
**Implementação**: `applications/realtime/realtimeDyMLib.cpp`

API C para embedding do motor completo (navio + física + ondas) em aplicações externas (simuladores gráficos, plataformas de treinamento, etc).

### Fluxo de uso

```c
// 1. Criação e inicialização
void* sim = realtime_create(base_dir_path);
realtime_init(sim);

// 2. A cada frame do simulador gráfico:
// Envia inputs (condições ambientais, comandos de manobra)
realtime_set_environment(sim, wind_speed, wind_dir, current_speed, current_dir, tide);
realtime_set_control(sim, rudder_angle, rpm, azimuth, tug_commands, ...);

// Avança um passo de tempo
realtime_step(sim, dt);

// Lê estado do navio
RealtimeBodyState state;
realtime_get_state(sim, &state);
// state.x, state.y, state.z, state.roll, state.pitch, state.yaw
// state.u, state.v, state.w, state.sog, state.cog
// state.forces[6], state.moments[6]  (cargas totais e por módulo)

// 3. Finalização
realtime_destroy(sim);
```

**Nota**: A API real-time expõe o mesmo `BodyDyM` da fast-time, mas com interface C pura para evitar problemas de ABI entre compiladores.

---

## Diferenças entre fast-time e real-time

| Aspecto | Fast-time | Real-time |
|---------|-----------|-----------|
| Entrada | JSON files | C API call por frame |
| Saída | CSV pós-processamento | Estado lido por polling |
| Controle de tempo | Loop interno (max speed) | Frame rate do host |
| Múltiplos navios | `deque<BodyDyM>` | Uma instância por `void*` |
| Parada | `Tfinal` ou manobra completa | Controlado pelo host |

---

## Notas de integração (C#, Python, etc.)

### Problemas no binding C# (documentados em ANALYSIS.md)

1. `WaveData` struct deve ter exatamente esta ordem de campos e tamanho:
   ```
   sizeof(WaveData) = 6 × sizeof(double) = 48 bytes
   ```
   Qualquer campo extra ou padding diferente causa leitura com offset errado.

2. `wave_engine` tem **7 parâmetros** na versão atual:
   ```c
   void wave_engine(double, double, double, const char*, const double*, size_t, int)
   ```
   Bindings antigos sem `spreading` (int) e `spectrum_type` (const char*) corruptam stack.

3. `wave_get_all()` retorna ponteiro para memória interna. **Não liberar.** Copiar os dados imediatamente — invalida no próximo `wave_engine()`.

### Calling convention

Todas as funções são `extern "C"` sem `__cdecl` explícito. Usar `CallingConvention.Cdecl` em P/Invoke (.NET) ou `CDECL` em ctypes (Python).
