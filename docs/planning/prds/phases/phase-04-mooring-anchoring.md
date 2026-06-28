# Fase 4 — Mooring + Anchoring

## Objetivo

Implementar cabos de amarração entre corpos e fundeio com catenária. Ao final desta fase, embarcações podem ser amarradas a píeres ou a outras embarcações (com força de tração via cabo elástico e ruptura) e fundear em profundidade conhecida (catenária da amarra, detecção de unhamento, garreio).

---

## Estado Atual (entrada da fase)

O que já existe:
- `Terrain::depthAt(x, y)` (Fase 2) — Anchoring depende
- `World` com queries de posição (Fase 2)
- `EventBus` com `ExternalEventSink` (Fase 2)
- Tick de 12 passos com passos 9 e 10 como stub (Fase 2)

O que falta (esta fase entrega):
- `Mooring` — cabos entre corpos ou corpo e ponto fixo
- `Anchoring` — âncora, amarra, catenária, garreio

---

## Escopo

### Incluído nesta fase

- `MooringCable` — cabo elástico com tensão, elasticidade, ruptura
- `MooringSystem` — gerencia lista de cabos e executa passo 9 do tick
- `Anchor` — geometria, estado (em suspensão / touching / holding / dragging)
- `Chain` — amarra com peso linear, comprimento
- `AnchoringSystem` — catenária de amarra, detecção de unhamento/garreio, passo 10 do tick
- Eventos: `mooring_ruptured`, `mooring_attached`, `anchor_holding`, `anchor_dragging`
- Testes: cabo em tensão, cabo em folga, ruptura, âncora unhada, âncora garreando

### Excluído desta fase

- Mooring com molas não-lineares avançadas (ORCA-like)
- Simulação dinâmica do cabo (catenária dinâmica) — apenas quase-estático
- Fairlead friction
- Multiple anchor interaction (duas âncoras em teia)

---

## Módulos

### MooringCable

Um cabo conecta dois pontos: `fairlead_A` (posição no casco de entidade A) e `fairlead_B` (posição fixa ou no casco de entidade B).

**Modelo quasi-estático linear:**
```
L_free   = comprimento natural do cabo (m)
L_actual = dist(fairlead_A, fairlead_B)

Se L_actual <= L_free:
    F = 0                              -- cabo em folga

Se L_actual > L_free:
    extension = L_actual - L_free
    F_scalar  = k · extension          -- k = rigidez linear (N/m)
    F_vector  = F_scalar · unit(B - A) -- força de tração na direção do cabo

Se F_scalar > rupture_load:
    cabo rompido → F = 0 + publica mooring_ruptured
```

**Configuração por cabo:**
```cpp
struct CableConfig {
    int   entity_a;          // id da entidade A (ou -1 para ponto fixo)
    int   entity_b;          // id da entidade B (ou -1 para ponto fixo)
    Point3D fairlead_a;      // posição do fairlead no referencial do casco A
    Point3D fairlead_b;      // posição do fairlead no referencial do casco B
    double length_m;         // comprimento natural
    double stiffness_nm;     // rigidez linear (N/m)
    double rupture_load_n;   // carga de ruptura (N)
};
```

**Ponto fixo:** `entity_id = -1` → `fairlead_B` é coordenada inercial absoluta (píer, boia fixa).

**Output por tick:**
- `Forces` de tração no fairlead de cada entidade → entregue ao `Physics`
- Momento (torque) calculado por braço de alavanca do fairlead ao centro de gravidade

---

### MooringSystem

Gerencia todos os cabos ativos de uma simulação.

```cpp
class MooringSystem {
public:
    int  addCable(const CableConfig& cfg);   // retorna cable_id
    void removeCable(int cable_id);

    // Passo 9 do tick:
    void compute(const World& world, std::map<int, Forces>& forces_out);

private:
    std::vector<MooringCable> cables_;
};
```

`compute` itera todos os cabos, calcula força para cada um, acumula em `forces_out[entity_id]`.

---

### Chain (amarra)

Amarra é o cabo de aço entre âncora e embarcação.

```cpp
struct Chain {
    double length_m;        // comprimento total pagado
    double weight_per_m;    // peso linear (N/m)
    double diameter_m;      // diâmetro (para cálculo de área projetada)
};
```

---

### Catenária da Amarra

Modelo catenária quasi-estática para calcular força de retenção em função do comprimento pagado, profundidade e tração horizontal.

**Fórmulas:**
```
Dados:
  L = comprimento da amarra na água (m)
  w = peso linear submerso (N/m) = weight_per_m · (1 - ρ_água/ρ_aço)
  h = profundidade efetiva = Terrain::depthAt(anchor_x, anchor_y) + tide

Catenária:
  a = parâmetro catenária = T_H / w    (T_H = tração horizontal no fundo)

Resolução iterativa (bissecção ou Newton):
  L² - h² = 2·a·h  →  encontrar T_H que satisfaz comprimento L para profundidade h

Forças no fairlead da embarcação:
  F_horiz = T_H  (na direção da linha âncora-embarcação projetada no plano)
  F_vert  = w · (L - sqrt(L² - (2·a·h)))   (componente vertical)

Comprimento na água:
  L_water = min(length_paid, max(0, L_paid - horizontal_scope))
```

**Simplificação aceitável para V1:** tração horizontal calculada como mola linear com constante calibrada pelo comprimento e peso da amarra.

---

### AnchoringSystem

Gerencia âncoras de uma simulação.

```cpp
class AnchoringSystem {
public:
    void launchAnchor(int vessel_id, int fairlead_idx);  // lança âncora
    void retrieveAnchor(int anchor_id);                  // recolhe âncora

    // Passo 10 do tick:
    void compute(const World& world, const Terrain& terrain,
                 std::map<int, Forces>& forces_out);
};
```

**Estados por âncora:**

| Estado | Condição |
|--------|----------|
| `Suspended` | Âncora não tocou fundo |
| `Touching` | Âncora no fundo, amarra em folga (sem retenção) |
| `Holding` | Âncora unhada, amarra tensa, força de retenção ativa |
| `Dragging` | Força horizontal > holding capacity → âncora arrasta |

**Transições:**
```
Suspended → Touching: z_anchor <= -depth_at(anchor_x, anchor_y)
Touching  → Holding:  tension > threshold E tempo_no_fundo > τ_set
Holding   → Dragging: F_horizontal > holding_capacity(anchor_weight, seabed_type)
Dragging  → Holding:  F_horizontal < holding_capacity (âncora para)
```

**Holding capacity (simplificado):**
```
F_hold = anchor_weight · k_seabed
k_seabed: sand=5, mud=3, rock=2  -- fator empírico
```

**Eventos publicados:**
- `anchor_holding` quando `Touching → Holding`
- `anchor_dragging` quando `Holding → Dragging`

---

## Integração no Tick

Passos 9 e 10 substituem stubs da Fase 2:

```
9.  mooring_.compute(world_, forces)     ← Fase 4 (substituiu stub)
10. anchoring_.compute(world_, terrain_, forces) ← Fase 4 (substituiu stub)
```

Forças acumuladas em `forces[id]` são somadas antes do CVODE integrar (passo 11).

---

## Acceptance Criteria

1. Cabo em tensão gera força proporcional à extensão (`F = k · (L - L0)`)
2. Cabo em folga (`L_actual < L_free`) → `F = 0` (sem compressão)
3. Cabo rompido quando `F > rupture_load` → evento `mooring_ruptured` disparado
4. Embarcação amarrada resiste a força de corrente dentro do limite do cabo
5. Âncora lançada toca fundo na profundidade correta de acordo com `Terrain::depthAt`
6. Evento `anchor_holding` disparado após âncora unar
7. `anchor_dragging` disparado quando força horizontal excede `holding_capacity`
8. Embarcação fundeada em corrente fraca permanece estável (oscilações dentro de 1 diâmetro)
9. Cobertura de testes ≥ 80%

---

## Decisões Abertas para TechSpec

- Catenária: resolver iterativamente ou usar aproximação de mola? (trade-off: precisão vs. custo computacional)
- `seabed_type` — vem do Terrain (como atributo do grid) ou configurado por cenário?
- Múltiplas âncoras por embarcação — suportado desde V1?
- `launchAnchor` precisa de posição explícita ou usa posição atual da embarcação + chain length?
- MooringSystem e AnchoringSystem vivem em `libs/physics/` ou `libs/simulation/`? (tendência: `libs/simulation/` pois orquestram estado entre entidades)
