# Especificação — Alinhamento dos Modelos de Força ao Golden (TMS Dynamics 3.0)

> Feature: `force-model-alignment` · Escopo: **Large/Complex** · Fase: **Specify**
> Fonte da verdade dos dados: `dynamics/vessel-drift-test-data/reference/`
> Fonte da verdade das fórmulas: engine fastTime C++ (`dynamics/nashville/src/*Forces/`)
> cruzada com o MATLAB `dynamics` (ver [[reference_dynamics_repo]]).

## Problem Statement

Os modelos de força do Ymir foram implementados do zero e, na validação cruzada
contra o dataset golden do TMS Dynamics 3.0 (ver
[golden-validation](../../specs/features/golden-validation/README.md)), vários
divergem — alguns em sinal e ordem de grandeza (corrente, squat), outros por
constantes/termos omitidos (vento, restauração, amortecimento). O objetivo é
**fazer cada modelo do Ymir reproduzir os dados de referência dentro de
tolerância de engenharia**, tratando o golden como verdade-terreno.

## Goals

- [ ] Todos os 7 tipos de força (corrente, vento, squat, leme, onda, restauração,
      amortecimento) alinhados ao golden com **erro relativo ≤ 5%** (piso
      absoluto perto de cruzamentos por zero), verificado por teste automatizado.
- [ ] Cada cenário do dataset (`01`–`08`) com um teste golden **passando** em
      `ymir_golden_tests`.
- [ ] Divergências residuais (> tolerância) documentadas com causa-raiz e
      justificativa física — zero "ajuste de tolerância para passar" sem
      explicação.
- [ ] Constantes físicas (rho_air, rho_water, g) reconciliadas com o TMS.

## Decisões (gray areas resolvidas com o usuário)

| Tema | Decisão |
|------|---------|
| Tolerância de alinhamento | **≤ 5% (engenharia)** — aceita diferença de rho/interpolação; combinar erro relativo + piso absoluto perto de zero |
| Constantes físicas | **Casar exatamente com o TMS** (ex.: `rho_air` 1,225 → valor do TMS ~1,27; conferir `rho_water`, `g`) |
| Abrangência | **Todos os 7 modelos** nesta leva |
| Fonte da fórmula correta | **Cruzar** engine fastTime C++ (gerou o golden) **+** MATLAB `dynamics` |

## Out of Scope

| Item | Motivo |
|------|--------|
| Reescrever a arquitetura de forças / `NavalForceModel` | Alinhamento é numérico, não estrutural; arquitetura permanece |
| Portar código do engine de referência 1:1 | AGENTS.md proíbe port; entender a intenção e implementar corretamente |
| Tug/bollard (`Ftg`) numérico | `vessel1.json` não tem bloco `tugs`; sem golden isolado (ver README do dataset) — apenas garantir sinal/limites |
| Azimuth thruster | `azimuthSpeed=0` neste vessel (eixo fixo) |
| Mudar o integrador (RK45) | Método state-in→force-out é independente do integrador |
| Alinhar trajetória completa (motion) | Valida-se **força** por passo, não a integração no tempo |

---

## User Stories

### P1: Corrigir corrente (OBOKATA) ⭐ MVP

**User Story**: Como engenheiro de física, quero que a força de corrente do Ymir
reproduza `Fc` do golden, para que cenários com corrente sejam confiáveis.

**Why P1**: Divergência mais grave medida — **sinal e magnitude errados**
(`Fc_x` Ymir ≈ −83 kN vs golden ≈ +172 kN, erro rel ~−1,5). Corrente é força
primária em operações de atracação.

**Causa provável** (a confirmar no design): (a) `computeObokata` usa
`frontalHeight·dx` como área seccional (área efetiva ≈ 4025 m² vs 1495 m²
tabulada em `vessel1.json`); (b) convenção de sinal do coeficiente/força
invertida vs `dynamics/nashville/src/currentForces/CurrentForces.cpp`.

**Acceptance Criteria**:

1. WHEN o estado golden de `01_current` (OBOKATA) é injetado no `CurrentForces`
   em cada passo THEN `Fc_x`, `Fc_y` e `Mc_z` SHALL casar com o golden com erro
   relativo ≤ 5% (piso absoluto perto de zero).
2. WHEN a corrente vem de proa (incidência ~180° no corpo) THEN o sinal de
   `Fc_x` SHALL corresponder ao golden.
3. WHEN o modelo REGULAR de corrente é usado (VDP) THEN os testes unitários
   existentes de `TestCurrentForces` SHALL continuar passando.

**Independent Test**: `TestGolden01Current.cpp` passa; sinal de `Fc_x` correto.

---

### P1: Corrigir squat ⭐ MVP

**User Story**: Como engenheiro, quero que `Fsq_z` do Ymir reproduza o golden em
água rasa, para que efeitos de squat sejam confiáveis.

**Why P1**: Divergência de **~4 ordens de grandeza** (Ymir ≈ −2,1e4 N vs golden
≈ −7,0e8 N). Bug/lacuna de modelo, não fator de unidade.

**Causa provável**: `SquatForces::computeNaval` usa
`depth = max(|waterDepth|, |z|)` (pega |z|≈23 em vez de 7 m) e a escala
`nabla/(rho·g·L²)` gera afundamento `s` ~1e-4 m onde o golden implica ~3,3 m.
Conferir contra `dynamics/nashville/src/squatForces/SquatForces.cpp`.

**Acceptance Criteria**:

1. WHEN o estado golden de `07_squat` é injetado THEN `Fsq_z` SHALL casar com o
   golden com erro relativo ≤ 5%.
2. WHEN a profundidade efetiva é calculada THEN SHALL usar a profundidade de
   água correta (não `|z|` do vessel), conforme o engine de referência.
3. WHEN `Fn < limiar` (velocidade nula) THEN `Fsq_z` SHALL ser zero.

**Independent Test**: `TestGolden07Squat.cpp` passa.

---

### P1: Reconciliar constantes físicas (vento e globais)

**User Story**: Como engenheiro, quero as constantes do Ymir iguais às do TMS,
para eliminar erro sistemático de constante.

**Why P1**: Vento já casa em sinal/forma; resíduo de ~4% é `rho_air`
(1,225 vs ~1,27). Barato e destrava tolerância apertada em várias forças.

**Acceptance Criteria**:

1. WHEN `rho_air` é alinhado ao valor do TMS THEN `Fwd_x` de `02_wind` SHALL
   casar com o golden com erro relativo ≤ 2%.
2. WHEN `rho_water` e `g` são conferidos contra o engine de referência THEN
   quaisquer diferenças SHALL ser corrigidas ou documentadas.
3. WHEN as constantes mudam THEN nenhum teste unitário existente SHALL quebrar
   sem justificativa registrada.

**Independent Test**: `TestGolden02Wind.cpp` aperta para ≤ 2%.

---

### P1: Restauração — acoplamento hidrostático off-diagonal

**User Story**: Como engenheiro, quero `Fr`/`Mr` do Ymir reproduzindo o golden
mesmo com pitch, para restauração hidrostática confiável.

**Why P1**: Ymir usa matriz de rigidez **diagonal** e omite os termos
`hydrostaticRestoring[2][4]/[4][2]`; divergência chega a ~6,5% (`Fr_z`) e ~13,5%
(`Mr_y`) quando o pitch cresce.

**Acceptance Criteria**:

1. WHEN o estado golden de `08_restoring` é injetado THEN `Fr_z` e `Mr_y` SHALL
   casar com o golden com erro relativo ≤ 5% em toda a trajetória.
2. WHEN os termos off-diagonal de `hydrostaticRestoring` são aplicados THEN o
   acoplamento heave↔pitch SHALL reproduzir o golden.
3. WHEN o vessel está em equilíbrio THEN a força líquida vertical SHALL tender a
   zero (limite físico).

**Independent Test**: `TestGolden08Restoring.cpp` aperta `Fr_z`/`Mr_y` para ≤ 5%.

---

### P2: Amortecimento — termo linear de heave em baixa velocidade

**User Story**: Como engenheiro, quero `Fd` casando com o golden inclusive em
baixa velocidade.

**Why P2**: `Fd_z` já casa a 0,05–0,6% com movimento; em baixa velocidade o erro
sobe a ~7% (Ymir omite um pequeno termo linear de heave presente no TMS).

**Acceptance Criteria**:

1. WHEN o estado golden de `08_restoring` é injetado THEN `Fd_z` SHALL casar com
   o golden com erro relativo ≤ 5% (piso absoluto perto de cruzamentos por zero).
2. WHEN o termo linear/potencial de heave é conferido contra o engine de
   referência THEN diferenças SHALL ser corrigidas.

**Independent Test**: `TestGolden08Restoring.cpp` (bloco damping) aperta para ≤5%.

---

### P2: Leme (rudder)

**User Story**: Como engenheiro, quero `Frd` casando com o golden.

**Why P2**: Depende de fluxo (surge inicial 5 m/s); `GoldenFrame` já fornece o
fluxo no corpo, mas o cenário acopla corrente/squat — precisa isolar `Frd`.
Divergência ainda **não medida**.

**Acceptance Criteria**:

1. WHEN o estado golden de `06_rudder` é injetado no `RudderForces` THEN `Frd`
   (x, y, Mz) SHALL casar com o golden com erro relativo ≤ 5%.
2. WHEN o ângulo de leme é 0 THEN `Frd` SHALL ser ~0.
3. WHEN não há fluxo THEN `Frd` SHALL ser ~0.

**Independent Test**: `TestGolden06Rudder.cpp` passa.

---

### P2: Onda (regular + JONSWAP)

**User Story**: Como engenheiro, quero as forças de onda (`Fwv_*`) casando com o
golden.

**Why P2**: Excitação/deriva espectral; requer alinhar fase e espectro. O caso
`REGULAR` (`03`) é determinístico e deve ser atacado primeiro; `JONSWAP` (`04`)
depende de semente (o engine de referência usa `rand()` sem `srand()`).

**Acceptance Criteria**:

1. WHEN o estado golden de `03_wave_regular` é injetado THEN as componentes de
   onda relevantes (`Fwv_ex`, `Fwv_md`, `Fwv_sd`, `Fwv_dd`) SHALL casar com o
   golden com erro relativo ≤ 5%.
2. WHEN o espectro/fase do Ymir é configurado THEN SHALL reproduzir a
   convenção do engine de referência (direção, fase, semente).
3. WHEN `04_wave_jonswap` não é reprodutível bit a bit THEN o teste SHALL validar
   estatísticas (média/RMS) em vez de valores instantâneos, e isso SHALL ser
   documentado.

**Independent Test**: `TestGolden03WaveRegular.cpp` passa; `04` valida estatística.

---

### P3: Thruster

**User Story**: Como engenheiro, quero `Fth` casando com o golden na rampa de rpm.

**Why P3**: Só existe em `module=maneuver`; o trial `05_thruster` auto-termina
~150 s (VLCC pesado). Sinal útil é a rampa rpm→thrust, não regime permanente.

**Acceptance Criteria**:

1. WHEN a telemetria golden de rpm (`05_thruster`) é aplicada THEN `Fth_x` SHALL
   casar com o golden com erro relativo ≤ 5% ao longo da rampa.
2. WHEN rpm = 0 THEN `Fth` SHALL ser 0.

**Independent Test**: `TestGolden05Thruster.cpp` passa na janela da rampa.

---

### P3: Tug — sinal e limites (sem golden numérico)

**User Story**: Como engenheiro, quero garantir que o modelo de tug é
fisicamente consistente.

**Why P3**: `vessel1.json` não tem bloco `tugs`; sem golden isolado.

**Acceptance Criteria**:

1. WHEN bollard pull é aplicado THEN `Ftg` SHALL ter sinal/direção corretos
   (teste analítico, não golden).

**Independent Test**: mantém `TestTugForces` (unitário analítico).

---

## Edge Cases

- WHEN uma coluna golden cruza zero (oscilação amortecida) THEN o teste SHALL
  usar piso absoluto para não explodir o erro relativo.
- WHEN o vessel golden reporta `yaw = π/2` em repouso THEN a reconstrução de
  fluxo (`GoldenFrame::nautToBodyFrame`) SHALL usar esse yaw sem reinterpretar a
  convenção.
- WHEN unidades do `vessel1.json` estão em tonelada/kN THEN o config SHALL
  converter para SI (×1000) — ver `Vessel1Params.h`.
- WHEN mudar `rho_air` afeta outra força que usa a constante THEN o impacto SHALL
  ser revalidado nos testes golden de todas as forças.
- WHEN o engine de referência é fisicamente questionável (ex.: profundidade de
  squat) THEN seguir o golden (verdade-terreno) e **documentar** a ressalva.

---

## Requirement Traceability

| ID | Story | Fase | Status |
|----|-------|------|--------|
| FMA-01 | P1: Corrente OBOKATA | Design | Pending |
| FMA-02 | P1: Squat | Design | Pending |
| FMA-03 | P1: Constantes (vento/globais) | Design | Pending |
| FMA-04 | P1: Restauração off-diagonal | Design | Pending |
| FMA-05 | P2: Amortecimento (heave baixa vel.) | - | Pending |
| FMA-06 | P2: Leme | - | Pending |
| FMA-07 | P2: Onda (regular + JONSWAP) | - | Pending |
| FMA-08 | P3: Thruster | - | Pending |
| FMA-09 | P3: Tug (sinal/limites) | - | Pending |
| FMA-10 | Infra: estender harness golden p/ cenários 01,03,05,06,07 | - | Pending |
| FMA-11 | Infra: medir divergência de leme/onda/thruster (baseline) | - | Pending |

**Formato do ID:** `FMA-NN` · **Status:** Pending → In Design → In Tasks → Implementing → Verified
**Cobertura:** 11 requisitos, 0 mapeados para tarefas (Tasks pendente).

---

## Success Criteria

- [ ] `ymir_golden_tests` cobre os 8 cenários (`01`–`08`), todos passando com
      erro ≤ 5% (ou estatístico documentado para `04_wave_jonswap`).
- [ ] Sinal de corrente e magnitude de squat corrigidos (não são mais findings).
- [ ] Constantes físicas reconciliadas com o TMS; suíte C++ (271+) verde via
      `turbo run test --filter @ymir/core`.
- [ ] Doc de findings `golden-validation/README.md` atualizado: divergências
      fechadas ou com causa-raiz + ressalva.
- [ ] Cada correção referencia a fórmula do engine de referência
      (`dynamics/nashville/src/*Forces/`) + MATLAB no commit/ADR.

---

## Próximos passos (pipeline)

1. **Design** (`design.md`) — para P1 (FMA-01..04): ler as fórmulas de referência
   (fastTime + MATLAB), mapear parâmetro-a-parâmetro, decidir mudanças por modelo.
2. **Tasks** (`tasks.md`) — quebrar por modelo, com dependências (FMA-10/11
   antes dos testes; FMA-03 antes de apertar tolerâncias).
3. **Execute** — implementar + verificar por teste golden, um modelo por vez.
