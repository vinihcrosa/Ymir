# Discussão: Multi-Física e Co-Simulação

Questão arquitetural aberta que afeta todas as fases do projeto. Decidir errado aqui
trava o crescimento do simulador. Decidir certo aqui é o que permite integrar guindaste,
turbina eólica, sistema elétrico no mesmo mundo sem reescrever o motor.

---

## O problema hoje

### Limitação 1 — NavalSimulation suporta exatamente 1 corpo

```cpp
// NavalSimulation.cpp:
NavalSimulation::NavalSimulation(std::unique_ptr<ymir::RigidBody6DOF> body)
{
    body_ptr_ = body.get();
    sim_.addBody(0, std::move(body));  // hardcode: ID 0, corpo único
}
```

Toda operação depois disso usa `sim_.state(0)` — ID fixo. Dois navios no mesmo mundo,
ou navio + rebocador com física própria, são impossíveis com a estrutura atual.

Isso contrasta com `Simulation` (o container genérico), que já foi projetado para N corpos:

```cpp
// Simulation.h — já suporta N corpos:
void addBody(int id, std::unique_ptr<AbstractBody> body);
BodyState state(int id) const;
```

`NavalSimulation` subusa `Simulation`.

### Limitação 2 — NavalContext pressupõe que todo corpo é hidrodinâmico

`NavalContext` carrega: cinemática naval, velocidade relativa à água, velocidade relativa
ao vento, profundidade, maré. Os `NavalForceModel`s recebem isso como contrato.

Um guindaste no convés do navio não tem `speedToWater`. Uma pá eólica não tem `waterDepth`.
Um modelo elétrico de gerador não tem nenhuma das duas coisas. Se tudo precisa passar por
`NavalContext`, objetos não-navais ficam de fora ou recebem campos que não fazem sentido.

---

## O que queremos simular no futuro (casos concretos)

### Caso A — Navio + guindaste de cais integrados

O guindaste pega containers e coloca no navio. As interações físicas reais:

- Peso do container suspenso → esforço no cabo → momento no guindaste → carga no píer
- Peso do container pousando no navio → força vertical no convés → mudança de trim/assentamento
- Vento nas pás do guindaste → momento no guindaste (independente de hidrodinâmica)

O guindaste **não é** um objeto hidrodinâmico. Mas os efeitos dele chegam no navio que **é**.
Os dois precisam rodar no mesmo `dt` e trocar forças.

### Caso B — Turbina eólica offshore

Três domínios simultâneos no mesmo objeto físico:

- **Base/monopilha**: hidrodinâmica (ondas, corrente, resposta de fundo)
- **Torre + nacele**: aerodinâmica (empuxo do vento nas pás, momento de tombamento)
- **Gerador**: elétrico (torque × velocidade angular → potência, controle de pitch)

Esses domínios são acoplados: a força nas pás depende da velocidade do vento **e** da
velocidade de rotação do rotor (que depende da carga elétrica). Não dá pra simular um
sem saber o estado do outro.

### Caso C — Frota: navio + múltiplos rebocadores

Cada rebocador é um corpo com física naval própria (propulsão, hidrodinâmica). Mas os
cabos entre rebocador e navio criam forças que agem nos dois. O `TugForces` atual é
uma simplificação: recebe força de reboque como parâmetro externo, não simula o rebocador
como corpo independente.

---

## Por que isso é uma decisão de fundação

A tentação é resolver cada caso quando ele aparecer: "quando precisar do guindaste,
adicionamos". Mas a arquitetura de acoplamento entre corpos e domínios é difícil de
refatorar depois — ela aparece em todo ForceModel, em toda interface de contexto, e
na forma como o tick é orquestrado.

Se não decidirmos agora o modelo de extensibilidade, cada fase vai criar seu próprio
workaround e o resultado é um motor impossível de manter.

---

## As opções

### Opção A — NavalSimulation vira multi-corpo (correção incremental)

Corrigir a limitação 1 sem tocar na limitação 2. `NavalSimulation` passa a suportar
N corpos navais, cada um com seu próprio `NavalContext`.

```text
NavalSimulation
  ├── RigidBody6DOF (navio)   + NavalContext próprio
  ├── RigidBody6DOF (rebocador A) + NavalContext próprio
  └── RigidBody6DOF (rebocador B) + NavalContext próprio
```

O acoplamento entre corpos (cabo navio-rebocador) é injetado como `ForceModel` que
lê o estado de outro corpo via referência.

- Prós: mudança menor. Funciona para frota de corpos todos navais.
- Contras: não resolve o problema de domínios heterogêneos (guindaste, turbina elétrica).
  Cada domínio novo vai exigir um outro `XyzSimulation` paralelo sem coordenação.

---

### Opção B — World como orquestrador de domínios (co-simulação)

`World` é o coordenador de mais alto nível. Não é só consulta espacial (como o roadmap
atual sugere) — é o relógio compartilhado e o ponto de acoplamento entre domínios.

```text
World  (relógio + troca de forças entre domínios)
  ├── NavalDomain      → gerencia N corpos navais + NavalContext por corpo
  ├── StructuralDomain → gerencia guindaste, cabos, containers
  ├── AeroDomain       → gerencia pás, forças de vento em estruturas não-navais
  └── ElectricalDomain → gerencia gerador, controle de pitch, potência
```

Cada domínio gerencia seus corpos dentro do `Simulation` genérico. O `World` expõe
uma interface de acoplamento: um domínio pode registrar que produz uma força num
ponto físico, e outro domínio pode consumir essa força.

```text
Tick do World:
  1. Cada domínio computa estado interno (sem acoplamento)
  2. World resolve acoplamentos (cabo: força em A = -força em B)
  3. Cada domínio aplica forças de acoplamento recebidas
  4. Simulation::step(dt) — CVODE integra todos os corpos
```

- Prós: resolve o problema na raiz. Cada domínio é isolado e extensível.
  Guindaste, turbina, gerador cabem sem modificar domínios existentes.
- Contras: mais complexo de implementar. Requer definir a interface de acoplamento
  antes de ter casos concretos — risco de over-engineer.

---

### Opção C — AbstractBody como ponto de extensão (domínios como corpos)

Em vez de domínios separados, cada "tipo de física" é uma subclasse de `AbstractBody`
com seu próprio contexto interno. `Simulation` genérico já pode guardar qualquer
`AbstractBody`.

```cpp
class CraneBody : public AbstractBody {
    // step() resolve mecânica do guindaste internamente
    // estado exposto via state() como BodyState (posição 6-DOF do centro de massa)
};

class WindTurbineBody : public AbstractBody {
    // step() resolve hidrodinâmica + aerodinâmica + elétrico internamente
};
```

O acoplamento (cabo guindaste → navio) seria um `ForceModel` especial que recebe
ponteiro para o `AbstractBody` do outro corpo e lê `state()` diretamente.

- Prós: reutiliza a infraestrutura existente (`Simulation`, `AbstractBody`).
  Não requer `World` como orquestrador.
- Contras: o acoplamento via `ForceModel` que lê outro `AbstractBody` cria
  dependência de ordem de execução — quem step primeiro? O cabo lê estado do
  tick anterior, não do tick atual (defasagem). Para acoplamentos rígidos (cabo
  inextensível) isso pode ser instável numericamente.

---

## A questão central

O acoplamento entre domínios (cabo, contato, força de interface) é o problema difícil.
Há duas filosofias:

**Acoplamento fraco (loose coupling):** cada domínio usa o estado do outro do tick
*anterior*. Simples de implementar, mas introduz defasagem de um `dt`. Aceitável
para acoplamentos suaves (cabo elástico longo, força de vento lenta). Problemático
para acoplamentos rígidos (contato rígido, cabo inextensível).

**Acoplamento forte (tight coupling):** resolve o acoplamento simultaneamente com
a integração. Requer solver conjunto (ex: CVODE com sistema de equações acoplado)
ou iteração de ponto fixo dentro do tick. Correto numericamente, mas muito mais
complexo de implementar e manter.

A maioria dos simuladores de engenharia naval usa acoplamento fraco com `dt` pequeno
o suficiente para que a defasagem seja desprezível. FMI (Functional Mock-up Interface)
é o padrão industrial para co-simulação com acoplamento fraco.

---

## Opinião do Claude

A opção A (correção incremental) resolve o problema imediato (frota multi-navio)
mas vai criar dívida quando o guindaste ou a turbina chegarem.

A opção B (World como orquestrador) é a arquitetura certa para o longo prazo —
mas tem o risco de over-engineer antes de ter casos concretos suficientes para
saber o que a interface de acoplamento precisa suportar.

A opção C (AbstractBody como extensão) é pragmática mas o problema de ordem de
execução e defasagem de acoplamento é real e vai aparecer.

**Preferência:** A no curto prazo (Fase 1-2), com a decisão B/C tomada explicitamente
antes da Fase 3 (quando o World precisa ser especificado de qualquer forma). O PRD
da Fase 2 deve incluir esta discussão como decisão central — não como detalhe.

Uma coisa que **não deve** esperar: a correção do corpo único no `NavalSimulation`.
Isso é um bug de design que vai impedir rebocadores independentes já na Fase 1.

---

## Sua opinião

> _Escreva aqui_

Eu acho que a opção B é a melhor, me parece ser a correta, mesmo que possa ocorrer overengineer é um risco baixo considerando o potencial que possui.
Se eu entendi bem o world nesse caso é um orquestrador, nele existem todos os bodies, e ele controla o tick, dentro dele tem os diferentes dominios, que tem acesso de alguma forma a esses bodies, e quando é dado o tick ele calcula os diferentes tipos de forças para no final fazer a integração com tudo, to certo nesse pensamento?

---

## Questões que precisam de resposta antes da Fase 3

1. O acoplamento entre domínios vai ser fraco (defasagem de 1 tick) ou forte (solver conjunto)?
2. `World` vai ser apenas consulta espacial ou vai orquestrar o tick multi-domínio?
3. Um objeto como a turbina offshore (múltiplos domínios no mesmo corpo físico) é um
   `AbstractBody` com física mista interna, ou são vários `AbstractBody`s acoplados?
4. O padrão FMI é relevante como referência, ou queremos uma interface proprietária?
