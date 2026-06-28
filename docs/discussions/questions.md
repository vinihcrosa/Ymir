# Discussões de Design — Camada de Embarcação

Questões arquiteturais abertas para a Fase 1. Antes de ler as perguntas, leia o contexto abaixo — sem ele as opções não fazem sentido.

---

## Contexto geral: o que já existe e o que vamos construir

### O tick hoje (sem controlador)

Cada chamada a `NavalSimulation::step(dt)` executa esta sequência:

```text
1. buildContext(dt)      → monta NavalContext com posição, velocidade, ambiente
2. sim_.step(dt)         → chama cada ForceModel, depois CVODE integra
```

Os `ForceModel`s (ThrustForces, RudderForces, TugForces) lêem o `NavalContext` para saber
a cinemática atual da embarcação (velocidade, heading) e produzem forças no referencial do corpo.
O CVODE integra essas forças e atualiza a posição/velocidade.

### O tick depois da Fase 1 (com controlador)

```text
1. vessel.updateControl(t, dt)   → controlador ativo calcula e escreve demandas
2. vessel.updateStates(t, dt)    → atuadores aplicam filtros, computa cinemática derivada
3. buildContext(dt)              → monta NavalContext (agora inclui demandas aplicadas)
4. sim_.step(dt)                 → ForceModels leem NavalContext + CVODE integra
```

### As três structs que aparecem nas perguntas

**`ThrusterConfig`** — struct que descreve um propulsor. Tem tanto parâmetros físicos fixos
quanto o campo que muda a cada tick:

```cpp
struct ThrusterConfig {
    double diameter        = 1.0;    // fixo: tamanho físico do propulsor
    double pitchRatio      = 1.0;    // fixo: geometria da hélice
    double rotationTime    = 50.0;   // fixo: constante de tempo de 1ª ordem
    double commandRPM      = 0.0;    // MUDA A CADA TICK: demanda do controlador
    // ... mais parâmetros físicos fixos
};
```

**`NavalContext`** — snapshot read-only montado no início de cada tick. Contém o estado
cinemático atual da embarcação (posição, velocidades no referencial do corpo, ambiente).
Os ForceModels recebem isso como `const NavalContext&` — não podem modificar.

**`VesselState`** — vai ser criado na Fase 1. Vai carregar o estado operacional visível
da embarcação: luzes de navegação acesas, modo operacional (Underway, Anchored, Moored...),
fase atual da FSM de atracação, etc.

---

## Q1 — Como o controlador entrega a demanda de RPM ao modelo de força?

### O problema concreto

`ManeuverController` calcula que o propulsor precisa de 80 RPM para atingir a velocidade
demandada. `ThrustForces` precisa saber disso no mesmo tick para calcular a força de empuxo.

Hoje `ThrustForces` já lê `commandRPM` de dentro de `ThrusterConfig`. Mas quem escreve
esse valor? Hoje ninguém — é configurado uma vez e fica fixo. Na Fase 1, o controlador
precisa escrevê-lo a cada tick.

A tensão real: `ThrusterConfig` mistura **dados físicos fixos** (diâmetro, curva de
rendimento) com **estado dinâmico de runtime** (`commandRPM`). Essa mistura é um smell —
mas já existe no código. A questão é: resolvemos agora ou aceitamos e seguimos?

#### Opção A — Controlador escreve direto na config

```cpp
void ManeuverController::compute(ThrusterConfig& thruster, RudderConfig& rudder) {
    thruster.commandRPM = pid_speed_.compute(speedError);
    rudder.angle_deg    = pid_heading_.compute(headingError);
}
```

`ThrustForces` não muda nada — já lê `commandRPM` de onde sempre leu.

- Prós: zero refatoração. Funciona agora.
- Contras: `ThrusterConfig` vira um objeto com dupla personalidade — parâmetros imutáveis de design + estado mutável de operação. Dificulta salvar a configuração original e comparar com o estado atual.

#### Opção B — NavalContext ganha seção mutável de demandas

```cpp
struct NavalContext {
    BodyState state;          // cinemática — read-only
    Vector6   speedToWater;   // ambiente — read-only
    // NOVO:
    struct ControlDemand {
        std::vector<double> rpmDemand;
        std::vector<double> rudderDemand;
    } demand;
};
```

Controlador escreve em `ctx.demand`. `ThrustForces` lê de `ctx.demand.rpmDemand[i]`.

- Prós: NavalContext vira o canal único entre controle e física.
- Contras: NavalContext era uma abstração limpa de "snapshot de estado". Agora tem seção mutável — a semântica quebra. Todo ForceModel recebe um campo `demand` que não usa.

#### Opção C — Separar config estática de estado dinâmico

```cpp
struct ThrusterConfig {          // imutável após init
    double diameter     = 1.0;
    double pitchRatio   = 1.0;
    double rotationTime = 50.0;
    // SEM commandRPM aqui
};

struct ThrusterState {           // muda a cada tick
    double commandRPM   = 0.0;
    double currentRPM   = 0.0;   // estado interno do filtro de 1ª ordem
};
```

Controlador escreve em `ThrusterState`. `ThrustForces` recebe ambos.

- Prós: separação clara. Config pode ser `const`. State é explicitamente mutável. Mais fácil de testar.
- Contras: refatoração de `ThrustForces` e `RudderForces` — que já existem e funcionam.

### Opinião do Claude — Q1

Opção C é a mais limpa mas requer refatorar código que já funciona. Opção B quebra
uma abstração que vale a pena preservar. Opção A é um atalho com dívida técnica explícita.

**Preferência:** A agora, com comentário claro marcando a dívida, e C quando for
necessário serializar ou comparar configurações.

### Sua opinião — Q1

> _Escreva aqui_

Eu tenho uma opinião diferente, me fala sua opinião sobre isso.
Eu modelaria diferente, vc está modelando o thruster apenas como um modelo de força, para mim ele é mais que uma força, claro que a interação entre ele e o corpo é dada por uma força, mas o thruster tem comportamente, tem lógica propria, no caso ele tem tres grandezes que podem ser modificadas, a rotação, pitch e azimuth, e nem uma delas vai direto para um valor, se eu demando um rpm de 100% e ele está em 0%, tem um tempo para chegar nos 100%, isso deve calculado a cada tick, portanto o thruster e de forma analoga, o rudder devem ser entidades de um vessel.
Eu modelaria uma aggregate root chamado DynamicVessel, que é um navio com todas os graus de liberdade e tudo mais, esse DynamicVesse possui rudders e thrusters, e esses rudders e thrusters não são apenas uma config, eles representam um ente de verdade, são realmente entidades nesse sentido.

---

## Q2 — Como a classe `Vessel` escolhe qual controlador rodar a cada tick?

### O problema concreto

`Vessel` vai ter três controladores implementados. A cada tick, exatamente um está ativo.
O usuário pode trocar o modo entre ticks (ex: de `maneuver` para `berthManeuver`).

Internamente, `Vessel::updateControl` precisa:

1. Saber qual controlador está ativo
2. Chamar o método `compute()` desse controlador
3. Escrever os resultados nos atuadores

A questão é a estrutura de dados que guarda os controladores e o mecanismo de dispatch.

#### Opção A — `std::variant` + `std::visit`

```cpp
class Vessel {
    using Controller = std::variant<
        Drift, ManeuverController, BerthManeuverSystem, PrescribedController
    >;
    Controller controller_;
};

void Vessel::updateControl(double t, double dt) {
    std::visit([&](auto& ctrl) { ctrl.compute(t, dt, config_); }, controller_);
}

// Trocar de modo:
void Vessel::setMode(ControlMode m) {
    if (m == ControlMode::Maneuver)
        controller_ = ManeuverController{};
}
```

- Prós: sem virtual, sem heap, tipo-seguro em compilação.
- Contras: conjunto de controladores é fechado — não dá pra plugar controlador externo em runtime sem mudar o tipo do variant.

#### Opção B — Interface abstrata `IController`

```cpp
struct IController {
    virtual void compute(double t, double dt, VesselConfig& cfg) = 0;
    virtual ~IController() = default;
};

class Vessel {
    std::unique_ptr<IController> controller_;
};

void Vessel::updateControl(double t, double dt) {
    controller_->compute(t, dt, config_);
}
```

- Prós: aberto para extensão — alguém cria `MyCustomController : IController` sem tocar em `Vessel`.
- Contras: `make_unique` a cada troca de modo = alocação. Virtual call por tick. Interface abstrata antes de saber se extensibilidade vai ser necessária.

#### Opção C — `std::optional` por controlador + if/else

```cpp
class Vessel {
    ControlMode mode_ = ControlMode::Drift;
    std::optional<ManeuverController>   maneuver_;
    std::optional<BerthManeuverSystem>  berth_;
    std::optional<PrescribedController> prescribed_;
};

void Vessel::updateControl(double t, double dt) {
    if (mode_ == ControlMode::Maneuver && maneuver_)
        maneuver_->compute(t, dt, config_);
    else if (mode_ == ControlMode::BerthManeuver && berth_)
        berth_->compute(t, dt, config_);
}
```

- Prós: explícito, sem abstração, fácil de ler.
- Contras: adicionar controlador = modificar `Vessel` em 4+ lugares. Não escala.

### Opinião do Claude — Q2

O conjunto de controladores da Fase 1 é fechado e conhecido. `std::variant` é o fit
natural para C++17 com conjunto fechado de tipos. Se surgir necessidade de
extensibilidade externa em fases futuras, migra para B — a mudança é localizada em `Vessel`.

**Preferência:** A (`std::variant + std::visit`).

### Sua opinião — Q2

> _Escreva aqui_

Eu não entendi esses controllers, mas eu aceito a opção A.

---

## Q3 — Como o sistema de âncora (Fase 4) vai avisar o `VesselState` que a âncora prendeu?

### O problema concreto

`VesselState` mantém o estado operacional da embarcação: `Underway`, `Anchored`, `Moored`, etc.
Quando a âncora prende no fundo, o estado deve mudar automaticamente para `Anchored`.

O problema: o sistema de âncora não existe ainda (é Fase 4). Mas `VesselState` precisa ser
projetado agora. Se projetarmos mal a interface de notificação, vamos ter que reescrever
`VesselState` na Fase 4 quando a âncora chegar.

Na Fase 1, as únicas mudanças de estado operacional são manuais (o usuário chama uma API).
A questão é: qual interface de notificação projetar já pensando na Fase 4?

#### Opção A — Métodos diretos no `VesselState`

```cpp
// Fase 4: sistema de âncora chama diretamente:
vesselState.onAnchorHolding();
vesselState.onMooringAttached();

// Fase 1: usuário chama diretamente:
vesselState.setOperationalState(OperationalState::Anchored);
```

- Prós: simples, direto.
- Contras: sistema de âncora (Fase 4) vai precisar incluir `VesselState`. Dependendo de onde o sistema de âncora viver na hierarquia de bounded contexts, isso pode conflitar com as regras de dependência do `AGENTS.md`.

#### Opção B — Callbacks `std::function` registrados

```cpp
vesselState.onAnchorHolding([]{ /* transiciona para Anchored */ });

// Fase 4: âncora dispara um callback genérico, não conhece VesselState:
anchorSystem.setOnHoldingCallback([&]{ vesselState.onAnchorHolding(); });
```

- Prós: sistema de âncora não precisa incluir `VesselState`.
- Contras: `std::function` capturando referências tem risco de dangling pointer. Ordem de registro importa.

#### Opção C — Fila de eventos tipada drenada pelo `Vessel` a cada tick

```cpp
enum class VesselEvent { AnchorHolding, MooringAttached };

// Fase 4: âncora empurra evento na fila:
events.push(VesselEvent::AnchorHolding);

// Vessel drena no início do tick:
void Vessel::updateControl(double t, double dt) {
    while (auto ev = events_.pop()) {
        if (*ev == VesselEvent::AnchorHolding)
            state_.setOperationalState(OperationalState::Anchored);
    }
    // ... dispatch para controlador ativo
}
```

- Prós: desacoplamento total. Produtor (âncora) e consumidor (VesselState) não se conhecem.
- Contras: infraestrutura que ficará ociosa até a Fase 4. A fila em si precisa ser projetada (thread-safe? por valor ou ponteiro?).

### Opinião do Claude — Q3

Na Fase 1 não existe sistema de âncora — toda infraestrutura de eventos ficaria ociosa.
Porém a opção A tem um problema real: âncora em `libs/simulation/` precisando incluir
`VesselState` de `libs/vessel/` pode ser inválido nas regras de bounded context.

**Preferência:** A para Fase 1 (só API manual). Decidir entre B e C quando o sistema
de âncora for especificado na Fase 4 — o desenho correto depende de onde ele vai viver.

### Sua opinião — Q3

> _Escreva aqui_

Acho que aqui temos um problema, a relação entre a ancora e o vessel é uma linha, um tipo de interação que é calculada, é a ancora que está unhada, não o vessel, a ancora unhada transmite uma força para o vessel através de uma linha que conecta os dois, portanto o que vamos ter que modelar é a linha no vessel como uma força, o resto ja está feito é só integrar essa força, portanto não se deve ter uma função no vessel de anchorHolding ou qualquer outra coisa, o vessel não deve saber da ancora especificamente.
Vai existir um modelo de guincho que controla o tamanho da linha, mas esse é o único controle que terá a ver com a ancora dentro do vessel, mas no fim é tratado como uma força do mesmo jeito.

---

## Q4 — Quem cria e possui a instância de `Vessel`?

### O problema concreto

`NavalSimulation::step(dt)` precisa chamar `vessel.updateControl(t, dt)` e
`vessel.updateStates(t, dt)` a cada tick. Mas o usuário também precisa de acesso
ao `Vessel` entre ticks — para trocar waypoints, ler `VesselState`, mudar o modo de controle.

A questão é: quem é o dono do objeto `Vessel`? Isso determina quem controla o lifetime,
quem pode chamar métodos nele, e como os dois objetos são criados.

#### Opção A — `NavalSimulation` é dono, recebe via `unique_ptr`

```cpp
auto vessel = std::make_unique<Vessel>(config);
Vessel* vessel_ptr = vessel.get(); // guarda ponteiro não-owning ANTES de mover
NavalSimulation sim(std::move(vessel));

// Entre ticks, usuário usa o ponteiro não-owning:
vessel_ptr->setWaypoints(waypoints);
sim.step(dt);
```

- Prós: `NavalSimulation` garante que `Vessel` existe enquanto a simulação existe.
- Contras: usuário precisa guardar ponteiro não-owning separado antes do `move`. Se esquecer, perde acesso direto. Lifetime implícito.

#### Opção B — Usuário é dono, passa referência para `NavalSimulation`

```cpp
Vessel vessel(config);
NavalSimulation sim(body, vessel); // sim recebe Vessel& — não-owning

// Entre ticks, usuário usa diretamente:
vessel.setWaypoints(waypoints);
auto state = vessel.state();
sim.step(dt);
```

- Prós: acesso direto e limpo. Lifetime explícito. Testável sem `NavalSimulation`.
- Contras: usuário gerencia lifetimes. Se destruir `vessel` antes de `sim`, comportamento indefinido.

#### Opção C — Fábrica cria os dois juntos

```cpp
auto [sim, vessel] = makeNavalVessel(body, config);

vessel.setWaypoints(waypoints);
sim.step(dt);
```

- Prós: conveniente, impossível ter um sem o outro.
- Contras: os dois têm lifetimes diferentes por natureza. Acoplá-los em fábrica é artificial.

### Opinião do Claude — Q4

Opção B segue o mesmo padrão que `VesselConfig` já usa no projeto: usuário cria, simulação
usa como referência. Lifetime explícito é melhor que lifetime escondido dentro de
`NavalSimulation`. Em testes de unidade do controlador, você cria um `Vessel`, chama
`updateControl` diretamente, e não precisa montar uma `NavalSimulation` inteira.

**Preferência:** B. Usuário possui `Vessel`. `NavalSimulation` recebe `Vessel&`.

### Sua opinião — Q4

> _Escreva aqui_

Acho que a gente ja falou disso, o modelo que temos hj de um único vessel está errado, uma simulação deve ter N corpos, e o modelo de ter NavalSimulation restringe futuras integrações entre dominios diferentes de simulação, o ideal é ter um world, nele estão todos os objetos, e através desse world que controlamos tudo.
