# Tasks — Alinhamento dos Modelos de Força ao Golden

> **Design**: [design.md](./design.md) · **Spec**: [spec.md](./spec.md)
> **Status**: Em execução

## Status de execução (leva 1)

| Task | Estado | Nota |
|------|--------|------|
| T3 `rho_air`=1,275 | ✅ feito | Wind golden ≤3% (was ~4%) |
| T5 squat divisão dupla + z_rel | 🟡 parcial | 4 ordens → ordem correta (e8); ≤5% pendente (cenário artificial) |
| T6 damping `vNorm2` 6-DOF | ✅ feito | sinal do surge/sway já era correto no Ymir (revertido) |
| T4 corrente sinal+área | ⛔ revertido | casa golden em repouso mas vira anti-arrasto em auto-propulsão (186/187) — precisa resolver convenção de `speedToWater` |
| T7 restauração | ✅ confirmado | ref é diagonal; sem mudança de código |
| T8 vento aparente | ⬜ pendente | fecharia wind <2% |
| T9 leme · T10 onda | ⬜ pendente | fórmulas mapeadas no design |
| T1/T2/T11 | parcial | fixtures 02/08 no repo; suíte 271 verde |

Suíte C++: **271/271 verde** após a leva. Corrente é o próximo item de projeto
(resolver convenção de sinal de `speedToWater` corrente-vs-amortecimento).


## Convenções de teste e gate (não há TESTING.md)

- **Tipo de teste**: `unit` (validação golden state-in→force-out por força).
- **Gate `quick`** (durante iteração de uma força):
  `cmake --build core/build --target ymir_golden_tests && ./core/build/tests/ymir_golden_tests`
- **Gate `full`** (fecho de fase): `pnpm turbo run test --filter @ymir/core`
  (esperado: 271+ testes verdes; nenhum apagado silenciosamente).
- **Critério de alinhamento**: erro relativo ≤ 5% (piso absoluto perto de
  cruzamentos por zero), conforme spec.
- **Pós-código**: `graphify update .` (regra do CLAUDE.md).
- **Tools por task**: MCP nenhum; Skill nenhum (exceto `graphify` no fecho).

---

## Execution Plan

### Phase 1: Foundation (Sequential)

```
T1 → T2
```

### Phase 2: Correções (Parallel após deps)

```
        ┌→ T3 [P] ─────────────┐
        ├→ T4 [P] (dep T1) ─────┤
        ├→ T5 [P] (dep T1) ─────┤
T1,T2 ──┼→ T6 [P] ──────────────┼──→ T11
        ├→ T7 [P] ──────────────┤
        ├→ T8 [P] ──────────────┤
        ├→ T9  (dep T1,T2) ─────┤
        └→ T10 (dep T1,T2) ─────┘
```

### Phase 3: Integração (Sequential)

```
T3..T10 → T11
```

---

## Task Breakdown

### T1: Estender harness golden (fixtures + configs) [FMA-10]

**What**: Gerar fixtures subamostrados e configs de força p/ os cenários
`01_current`, `03_wave_regular`, `05_thruster`, `06_rudder`, `07_squat`; declarar
todas as `vessel1*()` configs e registrar arquivos de teste no CMake — para
eliminar contenção de arquivo compartilhado nas tasks seguintes.
**Where**: `core/tests/fixtures/golden/{01,03,05,06,07}/`,
`core/tests/golden/Vessel1Params.h` (add current/squat/rudder/wave configs),
`core/tests/CMakeLists.txt`
**Depends on**: None
**Reuses**: `GoldenCsv.h`, `GoldenFrame.h`, script de downsample do README das fixtures

**Tools**: MCP NONE · Skill NONE

**Done when**:
- [ ] Fixtures `state.csv`+`loads.csv` (~200 linhas) p/ os 5 cenários
- [ ] `vessel1Current()`, `vessel1Squat()`, `vessel1Rudder()`, `vessel1Wave()`
      adicionadas a `Vessel1Params.h` (mapeadas do `vessel1.json`, ×1000 onde couber)
- [ ] Arquivos-stub `TestGolden0{1,3,6,7}*.cpp` registrados no CMake e compilando
- [ ] Gate `quick` passa (stubs vazios não quebram build)

**Tests**: none (infra) · **Gate**: build

---

### T2: Medir baseline leme (06) e onda (03) [FMA-11]

**What**: Probe rápido comparando Ymir vs golden p/ `Frd` (06) e `Fwv_md`/`Fwv_ex`
(03), registrando erro atual e a causa provável — insumo p/ T9/T10.
**Where**: `docs/planning/force-model-alignment/design.md` (seção "baseline")
**Depends on**: T1
**Reuses**: método probe (compilar contra `libymir_physics.a`)

**Tools**: MCP NONE · Skill NONE

**Done when**:
- [ ] Erro relativo atual de `Frd` (06) registrado com números
- [ ] Erro/estado de `Fwv_md` e `Fwv_ex` (03) registrado
- [ ] Causa provável de cada divergência anotada no design

**Tests**: none (medição) · **Gate**: none

---

### T3: Constante rho_air 1.225 → 1.275 [FMA-03a] [P]

**What**: Alinhar densidade do ar ao valor do engine de referência.
**Where**: `core/libs/common/include/ymir/common/PhysicalConstants.h:8`
**Depends on**: None
**Reuses**: —
**Ref**: `dynamics/nashville/src/basic/PhysicalProps.cpp:7`

**Tools**: MCP NONE · Skill NONE

**Done when**:
- [ ] `rho_air = 1.275`
- [ ] `TestGolden02Wind` apertado p/ `Fwd_x` ≤ 2% e verde
- [ ] Gate `quick` passa; nenhum outro teste golden regride
- [ ] Test count mantido (271+ no gate full)

**Tests**: unit · **Gate**: quick
**Commit**: `fix(common): align rho_air to reference (1.275)`

---

### T4: Corrigir corrente OBOKATA — sinal + área seccional [FMA-01] [P]

**What**: Remover a negação de `vc` (usar `atan2(vc_y, vc_x)`) e trocar a área
seccional de `frontalHeight/lateralHeight·dx` por `sub_depth·dL`
(`sub_depth = orig[2]-z`, `dL = length_BP/n_sections`).
**Where**: `core/libs/physics/src/forces/CurrentForces.cpp:31-101`;
`core/tests/golden/TestGolden01Current.cpp`
**Depends on**: T1
**Reuses**: `Vessel1Params::vessel1Current()`, `GoldenFrame`
**Ref**: `dynamics/nashville/src/currentForces/CurrentForces.cpp:142-195`

**Tools**: MCP NONE · Skill NONE

**Done when**:
- [ ] `Fc_x` com sinal correto e ≤ 5% vs golden em toda a trajetória (01)
- [ ] `Fc_y`, `Mc_z` ≤ 5% (ajustar termo `cd_z`/midship no Mz se necessário)
- [ ] `TestCurrentForces` (unit existente) continua verde
- [ ] Gate `quick` passa; Test count sobe (novo teste)

**Tests**: unit · **Gate**: quick
**Commit**: `fix(physics): correct current OBOKATA sign and sectional area`

---

### T5: Corrigir squat — divisão dupla por (ρ·g) [FMA-02] [P]

**What**: Numerador do sinkage = `volumetricWeight/(ρ·g·L²)` (divisão única);
remover o `/(ρ·g)` extra herdado de `nabla_`.
**Where**: `core/libs/physics/src/forces/SquatForces.cpp:13,49`;
`core/tests/golden/TestGolden07Squat.cpp`
**Depends on**: T1
**Reuses**: `Vessel1Params::vessel1Squat()`
**Ref**: `dynamics/nashville/src/squatForces/SquatForces.cpp:48-49`

**Tools**: MCP NONE · Skill NONE

**Done when**:
- [ ] `Fsq_z` ≤ 5% vs golden (07)
- [ ] Convenção de `depth`/clamp validada contra ref (documentar resíduo se houver)
- [ ] `TestSquatForces` (unit existente) continua verde
- [ ] Gate `quick` passa

**Tests**: unit · **Gate**: quick
**Commit**: `fix(physics): remove double rho*g division in squat sinkage`

---

### T6: Corrigir amortecimento — sinal surge/sway + vNorm2 [FMA-05] [P]

**What**: `aux[0]/[1] = -speedToWater` (alinhar sinal) e `vNorm2` sobre os **6**
componentes (não só surge+sway) no `decayFactor`.
**Where**: `core/libs/physics/src/forces/DampingForces.cpp:20-45`
**Depends on**: None
**Reuses**: `TestGolden08Restoring.cpp` (bloco damping)
**Ref**: `dynamics/nashville/src/dampingForces/DampingForces.cpp:32-50`

**Tools**: MCP NONE · Skill NONE

**Done when**:
- [ ] `Fd_z` ≤ 5% inclusive em baixa velocidade (08), piso absoluto p/ zero-crossing
- [ ] `TestDampingForces` (unit existente) continua verde
- [ ] Gate `quick` passa

**Tests**: unit · **Gate**: quick
**Commit**: `fix(physics): align damping surge/sway sign and velocity norm`

---

### T7: Restauração — tolerância/piso (sem mudança de código) [FMA-04] [P]

**What**: Ajustar piso absoluto em `TestGolden08Restoring` p/ refletir que o
resíduo é cruzamento por zero (ref é diagonal como o Ymir — não há bug de modelo).
**Where**: `core/tests/golden/TestGolden08Restoring.cpp`;
`docs/specs/features/golden-validation/README.md` (atualizar finding);
`docs/planning/force-model-alignment/spec.md` (reclassificar FMA-04)
**Depends on**: None
**Reuses**: —
**Ref**: `dynamics/nashville/src/restoringForces/RestoringForces.cpp:35-40`

**Tools**: MCP NONE · Skill NONE

**Done when**:
- [ ] `Fr_z`/`Mr_y` ≤ 5% fora de cruzamentos (piso absoluto documentado)
- [ ] Finding e spec atualizados (FMA-04 = sem off-diagonal)
- [ ] Gate `quick` passa

**Tests**: unit · **Gate**: quick
**Commit**: `test(physics): document restoring residual as zero-crossing`

---

### T8: Vento aparente — subtrair velocidade do vessel [FMA-03b] [P]

**What**: Em `speedToWind`, subtrair a velocidade do corpo (`bs.u()/v()`), como a
referência (`speedToWind = wind - vessel_vel`).
**Where**: `core/libs/simulation/src/NavalDomain.cpp:194-196`,
`core/libs/simulation/src/NavalSimulation.cpp:135-137`
**Depends on**: None
**Reuses**: `nautToBodyFrame` existente
**Ref**: `dynamics/nashville/src/body/Vessel.cpp` (`velocity_relative`)

**Tools**: MCP NONE · Skill NONE

**Done when**:
- [ ] `speedToWind` subtrai vel. do vessel
- [ ] Testes de simulação existentes continuam verdes
- [ ] Gate `full` passa (mudança na camada de simulação)

**Tests**: unit · **Gate**: full
**Commit**: `fix(simulation): subtract vessel velocity from apparent wind`

---

### T9: Alinhar leme (rudder) [FMA-06]

**What**: Alinhar convenção `β = wrap360(-(δ-α))`, sinal de `cd` (negado),
decomposição `xr/yr` e momentos sobre `localPosition`, conforme baseline (T2).
**Where**: `core/libs/physics/src/forces/RudderForces.cpp`;
`core/tests/golden/TestGolden06Rudder.cpp`
**Depends on**: T1, T2
**Reuses**: `Vessel1Params::vessel1Rudder()`, `GoldenFrame`
**Ref**: `dynamics/nashville/src/rudderForces/RudderForces.cpp:34-69`

**Tools**: MCP NONE · Skill NONE

**Done when**:
- [ ] `Frd` (x,y,Mz) ≤ 5% vs golden (06), isolando o acoplamento corrente/squat
- [ ] `Frd`≈0 com ângulo 0 e sem fluxo
- [ ] `TestRudderForces` (unit existente) continua verde
- [ ] Gate `quick` passa

**Tests**: unit · **Gate**: quick
**Commit**: `fix(physics): align rudder lift/drag convention to reference`

---

### T10: Alinhar onda — mean-drift + estatística [FMA-07]

**What**: Mapear RAO/QTF do `waves` block do `vessel1.json` p/ o `WaveRaoData`;
validar `Fwv_md` instantaneamente e `Fwv_ex` por estatística (fase `rand()` não
é reproduzível).
**Where**: `core/libs/world/src/wave/*`;
`core/tests/golden/TestGolden03WaveRegular.cpp` (+ `04` estatístico)
**Depends on**: T1, T2
**Reuses**: `core/src/wasm/WaveRaoData.h`, wave engine existente
**Ref**: `dynamics/nashville/src/waveForces/WaveComponent.cpp:308-401`,
`SurfaceWaves.cpp:91-187`

**Tools**: MCP NONE · Skill NONE

**Done when**:
- [ ] `Fwv_md` ≤ 5% vs golden (03)
- [ ] `Fwv_ex` validado por RMS/média (03); `04_jonswap` só estatística, documentado
- [ ] Testes de onda existentes (`TestWave*`) continuam verdes
- [ ] Gate `full` passa (camada world)

**Tests**: unit · **Gate**: full
**Commit**: `fix(world): align wave mean-drift to reference; excitation by RMS`

---

### T11: Verificação final + docs + graphify [FMA-10]

**What**: Rodar suíte completa, atualizar finding doc (divergências fechadas),
`graphify update .`.
**Where**: `docs/specs/features/golden-validation/README.md`, graphify-out/
**Depends on**: T3, T4, T5, T6, T7, T8, T9, T10
**Reuses**: —

**Tools**: MCP NONE · Skill `graphify`

**Done when**:
- [ ] Os 8 cenários (01–08) têm teste golden verde ≤5% (04 estatístico)
- [ ] Gate `full` passa: `pnpm turbo run test --filter @ymir/core`
- [ ] Finding doc atualizado; `graphify update .` rodado
- [ ] Sinal de corrente e magnitude de squat não são mais findings

**Tests**: unit · **Gate**: full
**Commit**: `docs(physics): close golden divergence findings`

---

## Validação pré-aprovação

### Check 1 — Granularidade

| Task | Escopo | Status |
|------|--------|--------|
| T1 | fixtures + configs (infra coesa) | ✅ |
| T2 | 2 probes de medição | ✅ |
| T3 | 1 constante | ✅ |
| T4 | 1 arquivo (CurrentForces) | ✅ |
| T5 | 1 arquivo (SquatForces) | ✅ |
| T6 | 1 arquivo (DampingForces) | ✅ |
| T7 | 1 teste + docs | ✅ |
| T8 | vento aparente (2 arquivos, mesma mudança) | ✅ |
| T9 | 1 arquivo (RudderForces) | ✅ |
| T10 | onda (1 subsistema coeso) | ⚠️ maior — fatiar se RAO/QTF crescer |
| T11 | verificação + docs | ✅ |

### Check 2 — Diagrama × Definição

| Task | Depends on (corpo) | Diagrama | Status |
|------|--------------------|----------|--------|
| T1 | None | raiz | ✅ |
| T2 | T1 | T1→T2 | ✅ |
| T3 | None | raiz→T3 | ✅ |
| T4 | T1 | T1→T4 | ✅ |
| T5 | T1 | T1→T5 | ✅ |
| T6 | None | raiz→T6 | ✅ |
| T7 | None | raiz→T7 | ✅ |
| T8 | None | raiz→T8 | ✅ |
| T9 | T1,T2 | T1,T2→T9 | ✅ |
| T10 | T1,T2 | T1,T2→T10 | ✅ |
| T11 | T3..T10 | →T11 | ✅ |

`[P]`: T3,T4,T5,T6,T7,T8 não dependem entre si (arquivos distintos). T1 cria
todas as configs em `Vessel1Params.h` e registra os stubs no CMake → sem
contenção de arquivo compartilhado nas tasks `[P]`. T9/T10 são sequenciais
(dependem de baseline T2).

### Check 3 — Co-locação de teste

Não há TESTING.md; matriz implícita: camada de física/força → `unit` obrigatório,
co-locado no mesmo task (teste golden). Nenhuma task difere tests p/ outra.

| Task | Camada | Requer | Task diz | Status |
|------|--------|--------|----------|--------|
| T3 | common (constante) | unit (via wind) | unit | ✅ |
| T4 | physics/current | unit | unit | ✅ |
| T5 | physics/squat | unit | unit | ✅ |
| T6 | physics/damping | unit | unit | ✅ |
| T7 | tests/docs | unit | unit | ✅ |
| T8 | simulation | unit | unit | ✅ |
| T9 | physics/rudder | unit | unit | ✅ |
| T10 | world/wave | unit | unit | ✅ |
| T1,T2,T11 | infra/medição/verif. | none | none | ✅ |

Nenhuma violação. `Tests: none` só em infra/medição/verificação.

---

## Ordem recomendada de execução

1. **T1** → **T2** (foundation).
2. Paralelo: **T3, T6, T7, T8** (sem deps) + **T4, T5** (após T1).
3. **T9**, **T10** (após T2).
4. **T11** (fecho).

Sugestão: executar cada task de código via sub-agente (contexto enxuto), 1
modelo por vez para os que compartilham `Vessel1Params.h`/CMake, `[P]` real
apenas onde arquivos não colidem.
