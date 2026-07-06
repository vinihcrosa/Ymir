# Validação cruzada com dataset golden (TMS Dynamics 3.0)

## Objetivo

Validar os modelos de força do Ymir contra o dataset de referência golden
gerado pelo TMS Dynamics 3.0 (`vessel-drift-test-data/reference/`), embarcação
`vessel1` (VLCC `3R_GUAMARE_VLCC_LOADED_L340B60T23`, L=350 B=63 T=23).

## Estratégia dos testes

Abordagem **state-in → force-out**, independente do integrador:

1. A trajetória de movimento golden (`state.csv`: posição + velocidade 6-DOF por
   tempo) é lida linha a linha.
2. Cada estado é injetado no modelo de força correspondente do Ymir.
3. A força calculada é comparada com a decomposição por força golden
   (`loads.csv`).

Como tanto o **estado de entrada** quanto a **força esperada** vêm do motor de
referência, o teste valida a *fórmula* da força, não o passo de tempo. Assim as
diferenças de integrador (Ymir usa RK45 próprio; TMS usa CVODE) não contaminam a
comparação.

Fixtures: `core/tests/golden/` (código) e
`core/tests/fixtures/golden/` (dados subamostrados a cada 25 passos).
Executável: `ymir_golden_tests`.

## Convenção de unidades (crítica)

O `vessel1.json` usa unidades em toneladas (massa em t, forças em kN, rigidez em
kN/m); os CSVs golden estão em SI (N, N·m). Todos os valores de
massa/peso/rigidez são multiplicados por **`kTonneToSI = 1000`** em
`Vessel1Params.h`. Verificado empiricamente: com esse fator, o `Fr_z` de
restauração do Ymir bate com o golden a ~3e-5 de erro relativo em pitch ~0.

## Resultados por força

A convenção de heading do TMS foi reproduzida em `GoldenFrame.h`
(`nautToBodyFrame`, espelhando `NavalDomain`): o fluxo ambiente (velocidade +
direção náutica) é rotacionado ao referencial do corpo usando o `yaw` golden.
Isso destrava as forças dependentes de fluxo (vento, corrente, leme).

| Cenário | Força | Status | Concordância | Observação |
|---------|-------|--------|--------------|------------|
| 08_restoring | Restauração `Fr_z` | ✅ valida | ~3e-5 (pitch≈0) → ~6,5% (pitch alto) | Divergência = acoplamento hidrostático off-diagonal `[2][4]/[4][2]` que o Ymir omite por decisão de projeto |
| 08_restoring | Momento `Mr_y` | ✅ valida | até ~13,5% | Mesma causa (off-diagonal) |
| 08_restoring | Amortecimento `Fd_z` | ✅ valida | 0,05–0,6% (com movimento); ~7% (baixa velocidade) | `vNorm2` do decay agora usa os 6 DOF (alinhado à referência) |
| 02_wind | Vento `Fwd_x` | ✅ valida | <3% (todas as linhas) | `rho_air` reconciliado a **1,275** (valor da referência); resíduo restante é vento aparente (T8, pendente) |

### Correções aplicadas nesta leva (ver `docs/planning/force-model-alignment/`)

- **`rho_air` 1,225 → 1,275** (`PhysicalConstants.h`) — casa o desvio de ~4% do
  vento. Confirmado contra `PhysicalProps.cpp` da referência.
- **Squat — divisão dupla por (ρ·g) corrigida** (`SquatForces.cpp`): o numerador
  do sinkage agora é `volumetricWeight/(ρ·g·L²)` (divisão única, como a
  referência), não `nabla_/(ρ·g·L²)`. Também corrigida a origem de heave
  (`z_rel = z + draft`) no cálculo de profundidade/clamp. Squat saiu de ~4 ordens
  de erro para a **ordem correta** (e8). **Parcial**: ainda não ≤5% no cenário
  `07` (calado 23 m em água de 7 m é semi-artificial; a dinâmica de profundidade
  precisa de um rerun da referência p/ fechar).

### Achados que impedem validação numérica direta (follow-up)

- **Corrente (01) — OBOKATA** — investigada a fundo. As correções que casam o
  golden **em repouso** (usar `atan2(vc)` sem negar → sinal; área
  `sub_depth·dx` → magnitude; `Fc_x` passa de -83 kN para +166 kN vs golden
  +172 kN, ~4% em `t≈0`) **quebram a física de auto-propulsão**: com o vessel se
  movendo, a força de corrente vira **anti-arrasto** (testes de integração 186/187
  falham — sem desaceleração / integrador diverge). Causa: conflito de convenção
  de sinal de `speedToWater` (Ymir = `vessel − fluido`; referência = oposto) e o
  papel corrente-vs-amortecimento no auto-movimento. **Revertido** — não é seguro
  aplicar sem resolver essa convenção. Requer análise de projeto dedicada.
- **Squat (07)** — ver "Correções": divisão dupla resolvida; fechamento ≤5%
  pendente (dinâmica de profundidade / cenário artificial).
- **Restauração (08)** — ⚠️ **corrigido o entendimento**: a referência **também é
  diagonal** (sem off-diagonal). O resíduo (6,5%/13,5%) é **cruzamento por zero**
  no assentamento, não bug de modelo. Sem mudança de código.
- **Leme (06)** — pendente (T9); fórmula de referência mapeada no design.
- **Onda (03,04)** — pendente (T10); fase `rand()` sem seed → validar mean-drift +
  estatística.

## Como estender

1. Gerar fixtures subamostrados do cenário (script no `README` das fixtures).
2. Mapear os parâmetros do `vessel1.json` para o `Config` da força em
   `Vessel1Params.h` (lembrar do fator ×1000).
3. Adicionar `TestGolden<NN>_<cenario>.cpp` seguindo o padrão do 08.
4. Definir tolerâncias **empiricamente** (probe rápido) e documentar a origem de
   qualquer resíduo — não ajustar tolerância para "passar" sem explicar a causa.
