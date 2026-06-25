# Contexto: Mundo

Representa o estado completo da simulação e o ambiente onde os corpos existem.
Nenhum módulo deste contexto executa cálculos físicos — apenas armazenam e disponibilizam dados.

---

## World

Fonte única da verdade. Contém todas as entidades e permite que qualquer módulo consulte ou modifique o estado.

**Responsabilidades:**
- Criar e remover entidades
- Armazenar estado atual de cada entidade (posição, orientação, velocidade, estado operacional)
- Responder consultas espaciais (ex: "quais corpos estão nessa área?", "qual profundidade nesse ponto?")
- Disponibilizar snapshots do estado completo da simulação
- Ser o único ponto de leitura/escrita de estado para todos os módulos

**Estado por corpo (12 valores):**

```
q[0..5]     = [x, y, z, roll, pitch, yaw]    — posição/ângulo no frame inercial
q_dot[0..5] = [ẋ, ẏ, ż, ṙoll, ṗitch, ẏaw]  — velocidades no frame do navio
```

**Notas:**
- Não executa cálculos
- Não conhece o conceito de tempo — apenas armazena o estado atual
- Todos os módulos leem e escrevem via World, nunca diretamente entre si

---

## Environment

Representa as condições ambientais dinâmicas da simulação.
Pode mudar em qualquer ponto da simulação.

### Vento

Modelo por coeficientes de arrasto interpolados por ângulo de incidência:

```
F_wind[surge] = 0.5 · ρ_ar · Cd_x(β) · A_frontal · |V_rel|²
F_wind[sway]  = 0.5 · ρ_ar · Cd_y(β) · A_lateral · |V_rel|²
F_wind[yaw]   = 0.5 · ρ_ar · Cd_z(β) · A_lateral · L · |V_rel|²
```

- `ρ_ar = 1.225 kg/m³`
- `β` = ângulo de incidência do vento relativo ao navio (inclui yaw rate)
- Cd_x, Cd_y, Cd_z: tabelas por embarcação (interpoladas por ângulo)
- Gera também roll e pitch via braço de alavanca da área exposta
- Variante `ACSINKAGE`: área corrigida pelo squat atual

### Correntes

- Velocidade e direção no frame inercial, convertidos para frame do navio a cada tick
- Alimentam CurrentForces (Obokata ou Regular) e DampingForces
- Variam por região (spatial field)

### Ondas

Sistema em duas camadas:

**SurfaceWaves** — espectro de frequências:

| Tipo | Descrição |
|------|-----------|
| `JONSWAP` | Mar jovem, pico pronunciado, fator de pico γ (padrão 3.3) |
| `PIERSON` | Mar plenamente desenvolvido |
| `REGULAR` | Onda monocromática, para testes e cenários simples |

```
amplitude[i] = sqrt(2 · S(ω[i]) · Δω[i])
phase[i]     = rand() · 2π
k[i]         = ω[i]² / g                   — relação de dispersão (águas profundas)
```

**Spreading direcional (Mitsuyasu cos-2S):**
- 18 direções, ±90° ao redor da direção principal
- Expoente S(ω) varia com ω/ωp

**WaveComponent** — forças por componente espectral:

Cada componente de mar gera 4 tipos de carga usando tabelas WAMIT da embarcação:

| Carga | DOFs afetados | Descrição |
|-------|--------------|-----------|
| Excitação 1ª ordem | heave, roll, pitch | Superposição de ondas via RAO de força |
| Mean drift | surge, sway, yaw | Determinístico, proporcional a amplitude² |
| Slow drift | surge, sway, yaw | Oscilação de baixa frequência (batimento) |
| Drift damping | surge, sway, yaw | Amortecimento proporcional à velocidade relativa |
| RAO motion | surge, sway, yaw | Deslocamento de posição induzido por ondas (adicionado diretamente à posição) |

**Filtro por DOF:**
- Excitação de força: heave/roll/pitch apenas (os horizontais são cobertos pelo drift e RAO)
- RAO de posição: surge/sway/yaw apenas

**EMA de posição** (τ = 16.5 s): posição suavizada usada para interpolar tabelas WAMIT e evitar que ruído de alta frequência perturbe os cálculos de onda.

### Outros campos ambientais

- Chuva (intensidade)
- Visibilidade
- Estado geral do mar
- Maré (nível de água — afeta batimetria efetiva e cálculo de squat)

**Todos os campos são dinâmicos** — podem ser alterados via API durante a simulação.

---

## Terrain

Representa o ambiente físico fixo e modificável da simulação.
Pode mudar durante a simulação (ex: dragagem, variação de maré).

**Responsabilidades:**
- Manter e disponibilizar:
  - Batimetria (profundidade por coordenada)
  - Linha costeira
  - Margens e canais
  - Píeres e estruturas fixas
  - Obstáculos estáticos
- Responder consultas de profundidade (usado por Hydrodynamics/Squat e Anchoring)
- Aceitar modificações em tempo de execução (ex: dragagem altera batimetria local)

**Dados consumidos por outros módulos:**

| Módulo | Dado consultado |
|--------|----------------|
| SquatForces | `water_depth` local para cálculo de Froude de profundidade |
| Anchoring | batimetria para detectar toque da âncora no fundo |
| Physics | geometria de obstáculos para detecção de colisão |
