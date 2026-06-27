# Fase 5 — Collision Detection

## Objetivo

Implementar detecção e resposta a colisões entre corpos rígidos. Ao final desta fase, dois navios ou um navio e uma estrutura fixa que se interceptam geram evento de colisão, recebem forças de contato que impedem interpenetração, e o simulador permanece estável.

---

## Estado Atual (entrada da fase)

O que já existe:
- `World` com entities e queries espaciais (Fase 2)
- `Terrain` com `FixedStructure` (polígonos de píeres e obstáculos) (Fase 2)
- `EventBus` com `collision` event definido (Fase 2)
- `BodyState` com posição e velocidade de cada corpo

O que falta (esta fase entrega):
- Representação geométrica de casco (bounding volume)
- Broad-phase: culling rápido de pares irrelevantes
- Narrow-phase: teste de intersecção preciso
- Resposta a colisão: forças de contato ou restrição de posição
- Evento `collision` com payload completo

---

## Escopo

### Incluído nesta fase

- `HullGeometry` — representação do casco para detecção de colisão
- `BroadPhase` — AABB sweeptest entre todas as entidades ativas
- `NarrowPhase` — intersecção GJK ou SAT entre bounding shapes
- `CollisionResponse` — impulso físico ou penalidade de posição
- Evento `collision` com pares colidindo, ponto de contato, normal
- Colisão navio-navio e navio-estrutura fixa (Terrain)
- Testes: dois corpos se aproximando param no contato, dois corpos distantes sem falso positivo

### Excluído desta fase

- Física de dano estrutural (deformação, flooding)
- Colisão navio-fundo (grounding) — pode ser derivada de `depthAt < draft`
- Colisão com cabos de amarração
- Rigid body constraint solver completo (Dantzig LCP) — impulso simples é suficiente para V1

---

## Módulos

### HullGeometry

Cada embarcação precisa de uma forma geométrica para colisão. Dois níveis:

**Nível 1 — Bounding box orientado (OBB):**
```
LOA × boca × pontal, centrado no CG
Suficiente para colisão navio-navio conservadora
```

**Nível 2 — Polígono 2D do casco (vista de cima):**
```
Lista de pontos {x, y} no referencial do casco
Usado para colisão mais precisa com estruturas
```

Nível 1 obrigatório. Nível 2 opcional por embarcação — fallback para OBB se ausente.

**Configuração em `VesselConfig`:**
```cpp
struct HullGeometry {
    enum class Type { OBB, Polygon2D };
    Type type = Type::OBB;
    // OBB: usa LOA, boca, pontal de VesselConfig automaticamente
    // Polygon2D:
    std::vector<Point2D> hull_points;  // pontos no referencial do casco
};
```

---

### BroadPhase

Elimina rapidamente pares que não podem colidir. Custo O(n log n).

**AABB per entity:**
```
Para cada entidade i:
  AABB_i = AxisAlignedBBox(hull_rotated_by_yaw(i) + position(i))
  // AABB é o envelope da OBB rotacionada — conservador

Par (i, j) é candidato se AABB_i.overlaps(AABB_j)
```

**Para estruturas fixas (Terrain):**
- AABBs pré-computadas no construtor (não mudam)
- Só re-computadas quando `Terrain::modify` é chamado

---

### NarrowPhase

Testado apenas para pares aprovados pelo broad-phase.

**OBB vs OBB — Separating Axis Theorem (SAT):**
```
Eixos de teste: 3 eixos de A + 3 eixos de B + 9 cross products
Para cada eixo:
  project A onto axis → [min_A, max_A]
  project B onto axis → [min_B, max_B]
  Se max_A < min_B ou max_B < min_A → sem colisão (eixo separador encontrado)
Sem eixo separador → colisão confirmada

Penetration depth = mínima sobreposição entre todos os eixos
Contact normal    = eixo com mínima sobreposição
```

**Polígono 2D vs ponto/polígono — GJK 2D (fallback):**
- Usado quando ambos têm `HullGeometry::Polygon2D`
- GJK retorna: colidindo sim/não, ponto de contato, normal, profundidade

---

### CollisionResponse

**Modelo de impulso (V1 — simples):**
```
Par colidindo (A, B) com profundidade d e normal n̂:

// Velocidade relativa no ponto de contato
v_rel = v_A - v_B

// Componente normal da velocidade relativa
v_n = v_rel · n̂

// Impulso (restitution coefficient e = 0.1 para colisões lentas de navios)
j = -(1 + e) · v_n / (1/m_A + 1/m_B)

// Aplicar impulso
v_A += (j/m_A) · n̂
v_B -= (j/m_B) · n̂

// Correção de posição (evitar interpenetração progressiva)
pos_A += 0.4 · (d / (m_A + m_B)) · m_B · n̂
pos_B -= 0.4 · (d / (m_A + m_B)) · m_A · n̂
```

**Colisão com estrutura fixa (B imóvel):**
```
// B tem massa infinita → apenas A recebe impulso
j = -(1 + e) · v_n / (1/m_A)
v_A += (j/m_A) · n̂
pos_A += 0.4 · d · n̂
```

**Integração no tick:**
Colisão não é um passo do tick de 12 — é executada **após** o passo 11 (CVODE integra), **antes** do passo 12 (eventos):
```
11. physics_.step(dt)         -- CVODE integra
11b. collisions_.detect_and_respond(world_)  -- ← NOVO
12. eventBus_.publish(events) -- inclui collision events
```

---

### CollisionEvent

```protobuf
message CollisionEvent {
    uint32  entity_a     = 1;
    uint32  entity_b     = 2;  // 0 se estrutura fixa
    string  structure_id = 3;  // preenchido se entity_b == 0
    float   contact_x    = 4;
    float   contact_y    = 5;
    float   normal_x     = 6;
    float   normal_y     = 7;
    float   penetration  = 8;
    float   relative_vel = 9;
}
```

---

## Acceptance Criteria

1. Dois corpos se movendo em direção oposta param ao colidir (sem interpenetração ≥ 1 cm)
2. Corpo contra estrutura fixa para corretamente
3. Dois corpos distantes (> LOA entre eles) — zero eventos de colisão
4. Evento `collision` contém ponto de contato e normal corretos
5. Simulação permanece estável após colisão (CVODE não diverge)
6. Performance: broad-phase com 10 embarcações < 1 ms por tick
7. Cobertura de testes ≥ 80%

---

## Decisões Abertas para TechSpec

- `CollisionSystem` vive em `libs/physics/` ou `libs/simulation/`? (tendência: `libs/physics/` pois opera em corpos rígidos)
- Grounding (navio tocando fundo): implementado nesta fase como caso especial ou deixado para depois?
- Restitution `e` — configurável por par ou constante?
- Spatial hash para broad-phase em vez de AABB all-pairs? (só necessário se n > 20)
- Usar CGAL (já presente como dependência) para narrow-phase polygon em vez de GJK próprio?
