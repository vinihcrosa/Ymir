# Decisões de Arquitetura — Ymir

Log de decisões tomadas. Cada entrada tem: o que foi decidido, por que, e o que foi
descartado. Referencia os arquivos de discussão onde o problema foi levantado.

---

## D1 — World como orquestrador de domínios (não só consulta espacial)

**Fonte:** `multi-physics.md`
**Data:** 2026-06-26

**Decisão:** `World` é o coordenador de alto nível do sistema. Possui o relógio global,
o container de corpos (`Simulation`), o ambiente compartilhado (`Environment`), e os
domínios físicos (`IDomain[]`). Não é apenas um índice espacial.

**Por quê:** O roadmap original tratava `World` como consulta espacial, mas os casos
de uso reais (frota multi-navio, guindaste, turbina offshore) exigem coordenação de
domínios heterogêneos com troca de forças. A arquitetura FMI/OpenFAST/SIMA converge
para este modelo. Implementar depois custa mais do que projetar certo agora.

**Descartado:**
- *World = só consulta espacial*: não resolve troca de forças entre domínios heterogêneos.

---

## D2 — Acoplamento fraco (Jacobi) como padrão; predictor-corrector adiado

**Fonte:** `world-architecture.md` Q3, `multi-physics.md`
**Data:** 2026-06-26

**Decisão:** O acoplamento entre domínios usa forças do tick anterior (Jacobi / loose
coupling). Predictor-corrector não é implementado nas Fases 1-3.

**Por quê:** OpenFAST e SIMA usam loose coupling como padrão para `dt` ≤ 0.5s e obtêm
resultados corretos para simulação naval. Os casos de uso das Fases 1-3 (navios,
rebocadores com TugForces simplificado, World básico) não envolvem cabos rígidos de alta
rigidez. Predictor-corrector adiciona iteração por tick — complexidade real sem caso de
uso real ainda. A interface `IDomain` já permite adicionar iteração no `World::step()` no
futuro sem modificar os domínios.

**Condição de revisão:** quando chegar cabo rígido entre rebocador independente e navio
(Fase 4+), revisar se `dt` pequeno resolve ou se iteração é necessária.

**Descartado:**
- *Predictor-corrector desde o início*: overengineering. Nenhum caso de uso nas Fases 1-3.
- *Acoplamento forte (solver conjunto)*: requer reformulação do DAE — não modular, não justificado.

---

## D3 — Domínio cria e registra seus próprios corpos

**Fonte:** `world-architecture.md` Q1
**Data:** 2026-06-26

**Decisão:** Cada `IDomain` cria os corpos que gerencia e os registra no `Simulation`
durante `initialize()`. `World` não conhece tipos de corpo.

```cpp
NavalDomain naval;
naval.addVessel(shipConfig);    // cria RigidBody6DOF internamente
naval.addVessel(tugConfig);

world.addDomain(std::move(naval));
world.initialize();             // naval.initialize() registra os corpos em Simulation
```

**Por quê:** Cada domínio é o único que sabe como criar seu tipo de corpo. Manter esse
conhecimento no domínio preserva o encapsulamento. `World` não precisa saber o que é um
`RigidBody6DOF` — isso é detalhe do domínio naval. YAGNI: o caso de "World cria corpos
e distribui" só é necessário para corpos compartilhados entre domínios, que é um caso
futuro não confirmado.

**Descartado:**
- *World cria corpos, domínios recebem por ID*: cria acoplamento entre World e tipos
  concretos de corpo. Complexidade sem caso de uso imediato.

---

## D4 — Corpos com múltiplos domínios: domínios adicionam ForceModels ao mesmo body ID

**Fonte:** `world-architecture.md` Q2
**Data:** 2026-06-26

**Decisão:** Para um corpo físico que pertence a múltiplos domínios (ex: turbina offshore
com hidro + aero + elétrico), um domínio é o *dono* do corpo (cria e registra) e os
outros domínios *contribuem* adicionando `ForceModel`s a ele. `AbstractBody::addForceModel`
já suporta múltiplos modelos — não há mudança de interface.

```cpp
// NavalDomain cria e possui o corpo da turbina:
naval.addBody(turbineConfig);           // registra no Simulation como ID 3

// AeroDomain contribui forças aerodinâmicas ao mesmo corpo:
aeroDomain.contributeForces(bodyId=3, std::make_unique<BladeForces>(bladeConfig));

// ElectricalDomain contribui torque do gerador:
electricalDomain.contributeForces(bodyId=3, std::make_unique<GeneratorModel>(genConfig));
```

O CVODE soma todas as forças de todos os modelos — exatamente como já funciona.

**Por quê:** Reutiliza infraestrutura existente. A alternativa (AbstractBody com física
mista interna) esconderia a separação de domínios dentro de um objeto — hard to test,
hard to extend. O modelo "dono + contribuidores" mantém cada domínio responsável por
seu próprio ForceModel.

**Quando implementar:** Somente na Fase com turbina ou objeto multi-domínio. Nas Fases
1-2, cada domínio é exclusivo para seus corpos. A interface já suporta, não requer código
extra agora.

---

## D5 — NavalSimulation some na Fase 2; sem coexistência

**Fonte:** `world-architecture.md` Q4
**Data:** 2026-06-26

**Decisão:** `NavalSimulation` é usado como está na Fase 1. Na Fase 2, é substituído por
`NavalDomain + World`. Não coexistem — um substitui o outro.

**Por quê:** Coexistência = dois caminhos de código fazendo a mesma coisa. Testes ficam
ambíguos. Documentação duplicada. O custo de manter `NavalSimulation` como "compatibilidade"
é maior do que a migração direta. A migração é cirúrgica:

```text
NavalSimulation::step()     → World::step()
NavalSimulation::addModel() → NavalDomain::addForceModel()
NavalSimulation::state()    → World::state(bodyId)
NavalEnvironment            → Environment (no World)
```

Ainda estamos em versão 0.x — breaking changes são aceitáveis. A API pública muda uma vez,
de forma limpa, na Fase 2.

**Descartado:**
- *Coexistência gradual*: duplicação de código, testes e documentação. Dívida garantida.

---

## D6 — CouplingPort carrega força + cinemática do ponto de interface

**Fonte:** `world-architecture.md` Q5
**Data:** 2026-06-26

**Decisão:** `CouplingPort` carrega posição e velocidade do ponto de interface no
referencial global, além da força.

```cpp
struct CouplingPort {
    int     bodyId;
    Vector3 pointLocal;     // ponto de aplicação no referencial do corpo
    Vector3 positionWorld;  // posição do ponto no referencial global (tick atual)
    Vector3 velocityWorld;  // velocidade do ponto no referencial global (tick atual)
    Vector6 force;          // força + momento aplicados por este domínio
};
```

**Por quê:** Para cabo modelado como mola, a tensão é `k × (pos_A - pos_B)`. Se
`StructuralDomain` não tem `pos_B` (posição do fairlead no navio) do tick atual, usa
a posição do tick anterior — defasagem dupla: posição E força desatualizadas. Isso
pode causar instabilidade para qualquer rigidez de cabo. A posição do ponto de interface
já está disponível no tick atual: `prepareContext()` roda antes de `computeCoupling()`,
portanto `NavalDomain` já atualizou seu estado quando `StructuralDomain` precisa da
posição do fairlead. Custo: campos extras no struct — negligível.

**Descartado:**
- *Só força*: defasagem dupla para cabo-mola. Instável para rigidez não trivial.

---

## D7 — Camada de Embarcação: despacho por std::variant + std::visit

**Fonte:** `questions.md` Q2
**Data:** 2026-06-26

**Decisão:** `Vessel` usa `std::variant<Drift, ManeuverController, BerthManeuverSystem,
PrescribedController>` para guardar o controlador ativo. Dispatch via `std::visit`.

**Por quê:** O conjunto de controladores é fechado e especificado em compilação. `std::variant`
é o idioma C++17 correto para esse padrão — sem heap por tick, sem virtual, tipo-seguro.
Interface virtual (`IController`) só é justificada se houver extensibilidade em runtime
por código externo — não há esse requisito nas Fases 1-N.

**Descartado:**
- *IController virtual*: heap allocation por troca de modo, interface abstrata prematura.
- *std::optional por controlador + if/else*: não escala, modifica Vessel em múltiplos
  lugares por novo controlador.

---

## D8 — Thruster e Rudder são entidades com estado e comportamento próprios

**Fonte:** `questions.md` Q1
**Data:** 2026-06-26

**Decisão:** `Thruster` e `Rudder` são entidades do domínio naval — não configs passivas.
Cada um tem parâmetros físicos estáticos, estado dinâmico mutável por tick, e lógica de
atualização própria. O `DynamicVessel` (aggregate root) possui essas entidades.

```text
DynamicVessel (aggregate root)
  ├── Thruster[] (entidades)
  │     ├── config: ThrusterConfig  — imutável: diâmetro, curva Kt, rotationTime
  │     ├── state: commandRPM, currentRPM, commandPitch, currentPitch,
  │     │         commandAzimuth, currentAzimuth
  │     ├── update(dt)              — aplica filtro 1ª ordem + limitadores de taxa
  │     └── computeForce(ctx)      — retorna vetor de força a partir do estado atual
  └── Rudder[] (entidades)
        ├── config: RudderConfig    — imutável: área, ângulo máximo, angleSpeed
        ├── state: commandAngle, currentAngle
        ├── update(dt)              — aplica limitador de taxa (rad/s)
        └── computeForce(ctx)      — retorna força de sustentação/arrasto
```

O controlador ativo (`ManeuverController`, etc.) escreve em `Thruster.commandRPM` e
`Rudder.commandAngle`. `DynamicVessel::updateStates(dt)` chama `update(dt)` em cada
entidade. `ThrustForces` e `RudderForces` lêem `currentRPM` e `currentAngle` das entidades
— não da config.

**Por quê:** O thruster tem três grandezas independentes com dinâmicas próprias: RPM
(constante de tempo rotationTime), pitch (se CPP) e azimuth (velocidade azimuthSpeed). Cada
uma tem a mesma estrutura — demanda + estado atual + filtro de tempo. Modelar isso como
campos soltos numa config é um smell. A entidade encapsula essa lógica, é testável isoladamente
(dado commandRPM=100, dt=1s, rotationTime=50s → currentRPM=1.98), e remove a ambiguidade
entre "config do propulsor" e "estado de operação do propulsor".

Esta decisão também resolve Q1 de `questions.md` naturalmente: o controlador escreve na
entidade, não na config. Nenhum campo de config é mutado em runtime.

**O que muda no código existente:**

- `ThrusterConfig` perde `commandRPM` — vira só parâmetros físicos estáticos
- `ThrustForces::currentRPM_` sai da força e vai para `Thruster::currentRPM`
- `ThrustForces::compute()` recebe `const Thruster&` em vez de ler config diretamente
- Análogo para `RudderForces` / `Rudder`

**Descartado:**
- *Mutação direta em ThrusterConfig (minha proposta original)*: config com dupla
  personalidade (estático + runtime). Encobre onde o estado realmente vive.
- *ThrusterConfig + ThrusterState separados como structs*: melhor que mutação direta,
  mas ainda dados sem comportamento. A lógica do filtro de 1ª ordem ficaria em
  `updateStates`, acoplando DynamicVessel à física do propulsor.

---

## D9 — VesselState não recebe eventos de âncora; Vessel não sabe da âncora

**Fonte:** `questions.md` Q3
**Data:** 2026-06-26

**Decisão:** `Vessel` não tem nenhuma API de âncora (`onAnchorHolding`, `setAnchored`, etc.).
A interação âncora-navio é mediada por uma linha física (corrente/cabo) que transmite força
ao casco no ponto do fairlead. Do ponto de vista do `Vessel`, isso é um `ForceModel` —
igual a qualquer outra força.

O que pertence ao `Vessel` é o **guincho** (`Winch`) — um atuador que controla o
comprimento da linha. O guincho segue o mesmo modelo de entidade de D8: tem estado
(comprimento atual, velocidade de arriamento) e controle (arriar/virar). A força resultante
no fairlead é calculada pela catenária/modelo de linha — implementado como `MooringForces`
ou similar.

```text
Vessel
  └── Winch (entidade atuadora)
        ├── lineLength: comprimento atual da linha
        ├── command: arriar / virar / segurar
        └── update(dt): avança comprimento conforme velocidade de arriamento

MooringLineForces (ForceModel no corpo)
  ├── lê Winch.lineLength para calcular geometria da catenária
  └── retorna tensão no fairlead como Vector6 de força
```

O estado `OperationalState::Anchored` no `VesselState` é setado manualmente pela aplicação
(ou derivado de condições físicas observáveis: velocidade perto de zero + linha em tensão).
Não é disparado por evento interno — o Vessel não tem condições de saber se a âncora prendeu;
quem sabe é o modelo de âncora no World.

**Por quê:** A âncora é um objeto do World — existe no fundo do mar, tem propriedades
físicas de solo, e a decisão de "prende ou arrasta" depende de forças no fundo. O Vessel
não modela o fundo do mar. A interface correta é: força no fairlead → Vessel. Tudo o que
acontece do outro lado da linha é fora do Vessel.

**Consequência para Q3 de `questions.md`:** A questão "como VesselState recebe evento
anchor_holding" é mal formulada. Não existe esse evento no Vessel. A questão se torna:
"como o World notifica a aplicação de que a âncora prendeu?" — isso é responsabilidade
do World e do sistema de eventos global, não do Vessel.

**Descartado:**

- *onAnchorHolding() no VesselState*: viola o encapsulamento — Vessel não tem visibilidade do estado da âncora.
- *Fila de eventos AnchorHolding no Vessel*: mesmo problema. Evento gerado por entidade fora do Vessel (âncora) sendo consumido dentro — acoplamento implícito.

---

## D9 — Eventos VesselState: métodos diretos na Fase 1

**Fonte:** `questions.md` Q3
**Data:** 2026-06-26

**Decisão:** `VesselState` expõe métodos diretos para transições de estado operacional
(`onAnchorHolding()`, `onMooringAttached()`). Chamada direta pelo caller. Fila de eventos
tipada não é implementada na Fase 1.

**Por quê:** Na Fase 1, não existe sistema de âncora nem de amarração — as únicas
transições são manuais via API. Toda infraestrutura de eventos (fila, tipos, dispatch)
ficaria ociosa. A fila tipada é o design correto para a Fase 4 (quando âncora e amarração
chegam), e será projetada com o contexto do sistema de eventos da Fase 2.

**Condição de revisão:** Fase 4. O sistema de âncora vai precisar de uma interface de
notificação — decidir então entre fila tipada (D6-pattern) ou callbacks.

**Descartado:**
- *Fila de eventos na Fase 1*: infraestrutura ociosa por 3+ fases.
- *Callbacks std::function*: risco de dangling reference, gerenciamento de lifetime
  complexo em C++.

---

## D10 — Posse da Vessel: usuário possui, NavalSimulation recebe referência

**Fonte:** `questions.md` Q4
**Data:** 2026-06-26

**Decisão:** Usuário cria e possui `Vessel`. Passa `Vessel&` para `NavalSimulation`
(Fase 1) e futuramente para `NavalDomain` (Fase 2). Lifetimes explícitos.

```cpp
Vessel vessel(config);
NavalSimulation sim(body, vessel);

vessel.setWaypoints(waypoints);  // acesso direto entre ticks
sim.step(dt);
auto state = vessel.state();     // acesso direto
```

**Por quê:** Padrão consistente com `VesselConfig` existente. Testável: testes de
controlador criam `Vessel` e chamam `updateControl` sem precisar de `NavalSimulation`.
Lifetime explícito é melhor que lifetime escondido — o usuário vê claramente que
`vessel` deve sobreviver enquanto `sim` existir. `NavalSimulation` como dono via
`unique_ptr` não resolve o problema de acesso — só o esconde atrás de um getter.

**Descartado:**
- *NavalSimulation possui via unique_ptr*: acesso ao Vessel requer getter adicional
  em NavalSimulation; lifetime escondido.
- *Fábrica co-construtora*: acoplamento artificial de lifetimes que são naturalmente
  independentes.

---

## D11 — NavalSimulation se torna multi-corpo na Fase 1 (correção do bug de corpo único)

**Fonte:** `multi-physics.md` Limitação 1
**Data:** 2026-06-26

**Decisão:** `NavalSimulation` é corrigido para suportar N corpos navais ainda na Fase 1,
antes de qualquer trabalho de controlador. Cada corpo tem seu próprio `NavalContext`.

**Por quê:** Rebocadores como corpos independentes com física naval própria são requisito
da Fase 1 (`TugForces` atual é simplificação — rebocador como parâmetro externo, não como
corpo). A limitação de 1 corpo é um bug de design, não uma feature. Corrigir depois custa
mais — entra na Fase 2 junto com World e exige refatoração maior.

**O que muda:**

```cpp
// HOJE (bug):
explicit NavalSimulation(std::unique_ptr<RigidBody6DOF> body);
// corpo único, ID hardcoded 0

// FASE 1 (corrigido):
NavalSimulation();
void addNavalBody(int id, std::unique_ptr<RigidBody6DOF> body,
                  std::unique_ptr<VesselConfig> config);
BodyState state(int id) const;
```

Cada corpo registrado recebe seu próprio `NavalContext` e set de `NavalForceModel`s.
O acoplamento entre corpos (navio ↔ rebocador) via `TugForces` continua como está — o
rebocador ainda é tratado como força paramétrica no navio principal. Rebocador como corpo
independente com física própria é Fase 2+ (requer troca de forças entre corpos).
