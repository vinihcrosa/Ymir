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

| Cenário | Força | Status | Concordância | Observação |
|---------|-------|--------|--------------|------------|
| 08_restoring | Restauração `Fr_z` | ✅ valida | ~3e-5 (pitch≈0) → ~6,5% (pitch alto) | Divergência = acoplamento hidrostático off-diagonal `[2][4]/[4][2]` que o Ymir omite por decisão de projeto |
| 08_restoring | Momento `Mr_y` | ✅ valida | até ~13,5% | Mesma causa (off-diagonal) |
| 08_restoring | Amortecimento `Fd_z` | ✅ valida | 0,05–0,6% (com movimento); ~7% (baixa velocidade) | Ymir omite um pequeno termo linear de heave presente no TMS |

### Achados que impedem validação numérica direta (follow-up)

- **Squat (07)** — o modelo de squat do Ymir diverge do golden em ~4 ordens de
  grandeza (Ymir ≈ -2,1e4 N vs golden ≈ -7,0e8 N). **Não é fator de unidade** —
  é lacuna/bug de modelo. A fórmula do Ymir produz um afundamento `s` ~1e-4 m,
  enquanto o golden implica ~3,3 m. Investigar `SquatForces::computeNaval`
  (uso de `depth = max(|waterDepth|, |z|)` e escala de `nabla/(rho·g·L²)`).
- **Corrente (01) / Vento (02) / Leme (06) / Onda (03,04)** — dependem do fluxo
  relativo em **referencial do corpo** (`speedToWater` / `speedToWind`).
  Reconstruir esses vetores a partir do golden exige reproduzir a convenção de
  heading do TMS (o golden mostra `yaw = π/2` em repouso, e `Fc` é dominado por
  surge, não por sway — ou seja, a convenção de ângulo não é trivial). Requer
  mapear a convenção antes de validar numericamente. Pendente.

## Como estender

1. Gerar fixtures subamostrados do cenário (script no `README` das fixtures).
2. Mapear os parâmetros do `vessel1.json` para o `Config` da força em
   `Vessel1Params.h` (lembrar do fator ×1000).
3. Adicionar `TestGolden<NN>_<cenario>.cpp` seguindo o padrão do 08.
4. Definir tolerâncias **empiricamente** (probe rápido) e documentar a origem de
   qualquer resíduo — não ajustar tolerância para "passar" sem explicar a causa.
