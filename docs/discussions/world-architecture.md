# Discussão: Arquitetura do World

Continuação de `multi-physics.md`. Decisão tomada: **Opção B — World como orquestrador
de domínios**. Este documento detalha como implementar isso no Ymir, com base nos padrões
do OpenFAST e SIMA.

---

## O que o World é (e o que não é)

**É:**
- O relógio global — controla `t` e `dt` do sistema inteiro
- O container de todos os corpos físicos (via `Simulation` interno)
- O ponto de acoplamento entre domínios — resolve quem passa força para quem
- O ambiente compartilhado — corrente, vento, batimetria, onde todos os domínios lêem

**Não é:**
- Um domínio físico — não calcula forças diretamente
- Um gerenciador de consultas espaciais apenas (a confusão atual do roadmap)
- Um god object — não sabe nada de hidrodinâmica, estrutura, ou elétrico

---

## Estrutura proposta

```text
World
  ├── Simulation                    ← container genérico de AbstractBody
  ├── Environment                   ← corrente, vento, ondas, batimetria (global)
  ├── Domain[]                      ← lista de domínios registrados
  │     ├── NavalDomain             ← gerencia corpos navais + NavalContext por corpo
  │     ├── StructuralDomain        ← guindaste, cabos, containers
  │     ├── AeroDomain              ← pás, forças de vento em estruturas
  │     └── ElectricalDomain        ← gerador, pitch, potência
  └── CouplingRegistry              ← mapa de quem passa o quê para quem
```

---

## O tick do World

Baseado no padrão OpenFAST / SIMA. Acoplamento fraco com Jacobi como padrão,
predictor-corrector como opção para pares críticos (cabo rígido).

```text
World::step(dt):

  Passo 1 — Cada domínio constrói seu contexto
    NavalDomain: reconstrói NavalContext para cada corpo naval
    StructuralDomain: monta estado da estrutura
    (usa Environment compartilhado para corrente, vento, profundidade)

  Passo 2 — Cada domínio computa forças de acoplamento
    "Qual força meu corpo aplica no ponto de interface com outro domínio?"
    Exemplo: StructuralDomain calcula tensão no cabo navio-guindaste
    Exemplo: AeroDomain calcula empuxo aerodinâmico na nacele da turbina
 
  Passo 3 — World resolve o acoplamento
    Para cada CouplingPort registrado:
      força_no_navio += tensão_do_cabo (do StructuralDomain)
      força_no_guindaste -= tensão_do_cabo (Newton: ação = -reação)

  Passo 4 — Cada domínio aplica forças de acoplamento recebidas
    NavalDomain: injeta força_no_navio como ForceModel especial
    StructuralDomain: injeta força_no_guindaste

  Passo 5 — Simulation::step(dt)
    Todos os corpos integram com CVODE independentemente
    Cada corpo usa as forças computadas nos passos 2-4
```

A defasagem: forças de acoplamento do passo 2 usam estado do tick *anterior*.
Para `dt` ≤ 0.5s e acoplamentos navais típicos, isso é aceitável (padrão SIMA/OpenFAST).

---

## Interface de domínio (IDomain)

Todo domínio implementa esta interface. World não sabe nada além dela.

```cpp
class IDomain {
public:
    virtual ~IDomain() = default;

    // Chamado uma vez na inicialização. Domínio registra seus corpos no Simulation.
    virtual void initialize(Simulation& sim, const Environment& env) = 0;

    // Passo 1: constrói contexto interno (NavalContext, etc.)
    virtual void prepareContext(double t, double dt, const Environment& env) = 0;

    // Passo 2: computa forças de acoplamento e as expõe via CouplingPort
    virtual void computeCoupling(double t, double dt) = 0;

    // Passo 4: recebe forças de acoplamento de outros domínios
    virtual void applyCoupling(const CouplingData& data) = 0;

    // Passo 5 (pós-integração): atualiza estado interno após CVODE
    virtual void postStep(double t, double dt) = 0;
};
```

---

## CouplingPort — o que cruza fronteiras de domínio

Um `CouplingPort` é um ponto de interface entre dois domínios. Carrega:

```cpp
struct CouplingPort {
    int    bodyId;         // qual corpo físico
    Vector3 pointLocal;   // ponto de aplicação no referencial do corpo
    Vector6 force;        // força + momento nesse ponto
};
```

Exemplos:
- **Cabo navio-guindaste**: StructuralDomain exporta `CouplingPort{navio, fairlead_pos, tensão}` e `CouplingPort{guindaste, ponto_de_fixação, -tensão}`
- **Pá turbina → torre**: AeroDomain exporta `CouplingPort{turbina, hub_pos, empuxo_aero}`
- **Container pousando no convés**: StructuralDomain exporta `CouplingPort{navio, posição_pouso, peso_container}`

O `CouplingRegistry` no World mapeia quais ports existem e quem produz/consome cada um.

---

## Como os corpos são criados e atribuídos a domínios

Duas opções:

### Opção 1 — Domínio cria e registra seus corpos

```cpp
NavalDomain navalDomain;
navalDomain.addVessel(vesselConfig);       // cria RigidBody6DOF internamente
navalDomain.addVessel(tugConfig);          // outro corpo naval

world.addDomain(std::move(navalDomain));
world.initialize();                        // domínios registram corpos no Simulation
```

Cada domínio é responsável pelo tipo de corpo que sabe criar. World não precisa saber
o que é um `RigidBody6DOF`.

- Prós: cada domínio encapsula sua criação de corpo. Simples de usar.
- Contras: o mesmo corpo físico não pode ser "dividido" entre dois domínios facilmente
  (ex: turbina com base naval E torre estrutural).

### Opção 2 — World cria os corpos, domínios os recebem por ID

```cpp
world.addBody(0, std::make_unique<RigidBody6DOF>(vesselConfig));
world.addBody(1, std::make_unique<CraneBody>(craneConfig));

navalDomain.claim(0);      // domínio naval gerencia corpo 0
structDomain.claim(1);     // domínio estrutural gerencia corpo 1
```

- Prós: um corpo pode ser "parcialmente reclamado" por dois domínios (turbina com física mista).
- Contras: World precisa saber de tipos de corpo. Mais complexo.

### Para o caso da turbina (múltiplos domínios no mesmo corpo físico)

A turbina tem **um** centro de massa físico integrável pelo CVODE mas **três** conjuntos
de forças de domínios diferentes (hidro + aero + elétrico). Com a Opção 1, a turbina seria
um único corpo e os três domínios competiriam por ele. Com a Opção 2, os três domínios
"reivindicam" o mesmo body ID e contribuem com forças independentes — o CVODE soma tudo.

---

## O que muda no código existente

| Componente atual | O que vira |
|---|---|
| `NavalSimulation` | `NavalDomain` — gerencia N corpos navais, um `NavalContext` por corpo |
| `NavalEnvironment` | Sobe para `Environment` no `World` — todos os domínios lêem |
| `Simulation` | Permanece como está — container genérico de `AbstractBody` |
| `NavalForceModel` | Permanece como está — registrado no corpo pelo `NavalDomain` |
| `NavalContext` | Permanece — produzido por `NavalDomain` por corpo a cada tick |

`NavalSimulation` não desaparece de imediato — pode existir como adaptador de compatibilidade
enquanto a Fase 1 roda. A migração para `World + NavalDomain` é escopo da Fase 2.

---

## Ambiente compartilhado (Environment)

Hoje `NavalEnvironment` (corrente, vento, batimetria) vive dentro de `NavalSimulation`.
No modelo com World, o ambiente é global e todos os domínios lêem do mesmo lugar.

```text
Environment (global, owned by World)
  ├── current: speed + direction (nautical convention)
  ├── wind: speed + direction
  ├── waterDepth: scalar ou campo espacial (para batimetria)
  ├── tide: scalar
  └── waves: WaveSpectrum (hoje em libs/world)
```

`NavalDomain` usa `Environment` para construir `NavalContext` por corpo.
`AeroDomain` usa `Environment.wind` para forças nas pás.
`StructuralDomain` usa `Environment` se precisar (ex: guindaste sujeito a vento).

---

## Decisões tomadas

Ver `decisions.md` para raciocínio completo.

| Questão | Decisão | Referência |
| --- | --- | --- |
| Q1 — Quem cria os corpos? | Domínio cria e registra seus próprios corpos | D3 |
| Q2 — Turbina multi-domínio? | Domínios contribuem ForceModels ao mesmo body ID | D4 |
| Q3 — Tipo de acoplamento? | Fraco (Jacobi). Predictor-corrector adiado para Fase 4+ | D2 |
| Q4 — Migração NavalSimulation? | Some na Fase 2, sem coexistência | D5 |
| Q5 — CouplingPort conteúdo? | Força + posição + velocidade do ponto de interface | D6 |
