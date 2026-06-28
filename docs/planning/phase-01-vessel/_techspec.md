# TechSpec: Camada de Embarcação — Fase 1

## Resumo Executivo

A Fase 1 introduz a camada de domínio naval em cima do motor de física existente. Três
linhas de trabalho simultâneas: (1) criar entidades de atuador `Thruster` e `Rudder` em
`libs/vessel/` que substituem o estado interno de `ThrustForces`/`RudderForces`; (2) criar
`DynamicVessel` com três controladores despacháveis via `std::variant`; (3) refatorar
`NavalSimulation` de single-body para N-bodies com suporte a registro de vessel.

**Tradeoff principal:** a refatoração de `ThrustForces`/`RudderForces` e `NavalSimulation`
é um breaking change deliberado — todos os testes existentes precisam ser migrados. Em
troca, o estado de atuador passa a ser testável em isolamento, `VesselConfig` torna-se
imutável em runtime, e a simulação passa a suportar N embarcações simultâneas.

---

## Arquitetura do Sistema

### Visão Geral dos Componentes

```
libs/vessel/ (libs/physics/ ← libs/vessel/ ← libs/simulation/)
├── Thruster               ← entidade: ThrusterConfig + ActuatorState + update(dt)
├── Rudder                 ← entidade: RudderConfig + ActuatorState + update(dt)
├── VesselState            ← POD: luzes, shapes COLREGS, estado operacional
├── DynamicVessel          ← aggregate root: owns Thruster[], Rudder[], VesselController
└── controllers/
    ├── ManeuverController ← LOS + PID heading/velocidade + lista de waypoints
    ├── PrescribedController← interpolação linear de séries temporais
    └── BerthManeuverSystem ← FSM 3 estados + rebocadores paramétricos

libs/physics/forces/ (modificado)
├── ThrustForces           ← remove currentRPM_; adiciona setActuatorState()
└── RudderForces           ← remove currentAngle_; adiciona setActuatorState()

libs/simulation/ (modificado)
└── NavalSimulation        ← single-body → N-bodies; registra DynamicVessel por corpo
```

### Fluxo de Tick por Corpo

```
NavalSimulation::step(dt):
  para cada BodyEntry em entries_:
    buildContext(entry, dt)
    if entry.vessel:
      entry.vessel->updateControl(t, dt, entry.ctx)   // controller → demandas em entidades
      entry.vessel->updateStates(dt)                  // entidades avançam dinâmica própria
      entry.vessel->syncToForceModels(entry.thrust,   // entidade → ThrustForces/RudderForces
                                      entry.rudder)
  sim_.step(dt)                                       // CVODE integra todos os corpos
```

A defasagem de um tick entre o estado calculado pelo vessel e o que CVODE integra é
intencional e idêntica ao padrão Jacobi do World na Fase 2.

---

## Design de Implementação

### Interfaces Principais

#### Thruster

```cpp
// libs/vessel/include/ymir/vessel/Thruster.h
class Thruster {
public:
    struct ActuatorState {
        double currentRPM          = 0.0;
        double currentPitch        = 0.0;
        double currentAzimuth_deg  = 0.0;
        double demandedRPM         = 0.0;
        double demandedPitch       = 0.0;
        double demandedAzimuth_deg = 0.0;
    };

    explicit Thruster(const ThrusterConfig& cfg);

    void setDemand(double rpm, double pitch_ratio, double azimuth_deg) noexcept;
    void update(double dt) noexcept;

    const ActuatorState&  state()  const noexcept;
    const ThrusterConfig& config() const noexcept;

    ThrustForces::ThrusterCommand toCommand() const noexcept;
};
```

`update(dt)` implementa:
- RPM: filtro de 1ª ordem `currentRPM += (1 - e^{-dt/τ}) * (demandedRPM - currentRPM)`, τ = `cfg.rotationTime`.
- Azimuth: rate limiter `Δazimuth = clamp(demanded - current, -rate*dt, +rate*dt)`, rate = `cfg.azimuthSpeed`.
- Pitch: idêntico ao azimuth com `pitchRate` (a definir em `ThrusterConfig`).

#### Rudder

```cpp
// libs/vessel/include/ymir/vessel/Rudder.h
class Rudder {
public:
    struct ActuatorState {
        double currentAngle_rad  = 0.0;
        double demandedAngle_rad = 0.0;
    };

    explicit Rudder(const RudderConfig& cfg);

    void setDemand(double angle_rad) noexcept;
    void update(double dt) noexcept;

    const ActuatorState& state()  const noexcept;
    const RudderConfig&  config() const noexcept;

    RudderForces::RudderCommand toCommand() const noexcept;
};
```

`update(dt)` implementa rate limiter: `Δangle = clamp(demanded - current, -rate*dt, +rate*dt)`,
rate = `cfg.angleSpeed` (rad/s).

#### VesselController (conceito via std::variant)

```cpp
// libs/vessel/include/ymir/vessel/DynamicVessel.h
using VesselController = std::variant<
    ManeuverController,
    PrescribedController,
    BerthManeuverSystem
>;
```

Todos os três tipos satisfazem o conceito implícito:
```cpp
void update(double t, double dt, const NavalContext& ctx,
            std::vector<Thruster>& thrusters, std::vector<Rudder>& rudders);
```

#### DynamicVessel

```cpp
// libs/vessel/include/ymir/vessel/DynamicVessel.h
class DynamicVessel {
public:
    DynamicVessel(const VesselConfig& cfg,
                  std::vector<Thruster> thrusters,
                  std::vector<Rudder>   rudders);

    DynamicVessel(const DynamicVessel&)            = delete;
    DynamicVessel& operator=(const DynamicVessel&) = delete;
    DynamicVessel(DynamicVessel&&)                 = delete;
    DynamicVessel& operator=(DynamicVessel&&)      = delete;

    void setController(VesselController controller);

    void updateControl(double t, double dt, const NavalContext& ctx);
    void updateStates(double dt) noexcept;
    void syncToForceModels(ThrustForces* tf, RudderForces* rf) noexcept;

    VesselState& vesselState() noexcept;
    const VesselState& vesselState() const noexcept;

    const Thruster& thruster(std::size_t idx) const;
    const Rudder&   rudder(std::size_t idx)   const;
    std::size_t thrusterCount() const noexcept;
    std::size_t rudderCount()   const noexcept;

private:
    const VesselConfig&    cfg_;
    std::vector<Thruster>  thrusters_;
    std::vector<Rudder>    rudders_;
    VesselController       controller_;
    VesselState            vesselState_;
};
```

`updateControl` faz:
```cpp
std::visit([&](auto& ctrl) {
    ctrl.update(t, dt, ctx, thrusters_, rudders_);
}, controller_);
```

#### ManeuverController

```cpp
// libs/vessel/include/ymir/vessel/controllers/ManeuverController.h
class ManeuverController {
public:
    struct Waypoint {
        double x_m, y_m;
        double demandedSpeed_mps;
    };
    struct Config {
        std::vector<Waypoint> waypoints;
        double captureRadius_m = 50.0;
        double headingKp, headingKi, headingKd;
        double speedKp,   speedKi,   speedKd;
    };

    explicit ManeuverController(Config cfg);

    void update(double t, double dt, const NavalContext& ctx,
                std::vector<Thruster>& thrusters, std::vector<Rudder>& rudders);

    bool waypointsExhausted() const noexcept;
    std::size_t activeWaypointIdx() const noexcept;

private:
    struct PID {
        double kp, ki, kd;
        double integral_ = 0.0, prevError_ = 0.0;
        double update(double error, double dt) noexcept;
        void   reset() noexcept;
    };

    Config      cfg_;
    std::size_t activeWpt_ = 0;
    PID         headingPid_, speedPid_;
};
```

LOS bearing: `atan2(dy, dx)` no referencial inercial, convertido para erro de heading
`wrapToPi(bearing - yaw)`. Propulsor 0 recebe demanda de RPM; leme 0 recebe demanda
de ângulo. (Embarcações com múltiplos propulsores/lemes: todos recebem a mesma demanda
via `for (auto& t : thrusters) t.setDemand(rpm, ...)` — comportamento refinável em
versões futuras sem quebrar interface.)

#### PrescribedController

```cpp
// libs/vessel/include/ymir/vessel/controllers/PrescribedController.h
class PrescribedController {
public:
    struct TimeSeries {
        std::vector<double> times;   // monotônico crescente, segundos
        std::vector<double> values;
    };
    struct Config {
        std::vector<TimeSeries> thrusterRPM;      // um por thruster (rpm)
        std::vector<TimeSeries> rudderAngle_rad;  // um por rudder
    };

    explicit PrescribedController(Config cfg);

    void update(double t, double dt, const NavalContext& ctx,
                std::vector<Thruster>& thrusters, std::vector<Rudder>& rudders);

private:
    static double interpolate(const TimeSeries& ts, double t) noexcept;
    Config cfg_;
};
```

`interpolate` usa `std::lower_bound` para localizar o intervalo e interpola linearmente.
Além do último ponto: retorna `ts.values.back()`. Erro garantido < 1e-6 por ser interpolação
exata de double (sem acumulação de erros).

#### BerthManeuverSystem

```cpp
// libs/vessel/include/ymir/vessel/controllers/BerthManeuverSystem.h
class BerthManeuverSystem {
public:
    enum class Phase { Navigating, Sideway, TurnROTTUG };

    struct TugForceConfig {
        double escortForce_N = 0.0;  // força no modo ESCORTING
        double pushForce_N   = 0.0;  // força no modo PUSH
        double bearing_deg   = 0.0;  // bearing relativo à popa (referencial do corpo)
        double arm_m         = 0.0;  // braço para cálculo de momento
    };
    struct BerthWaypoint {
        double x_m, y_m;
        Phase  targetPhase;
        double transitionDist_m;
        double headingTarget_rad;    // heading alvo ao atingir waypoint
    };
    struct Config {
        std::vector<BerthWaypoint>  waypoints;
        std::vector<TugForceConfig> tugs;
        double headingKp, headingKi, headingKd;
        double rotRateKp,  rotRateKi,  rotRateKd;
        double lateralKp,  lateralKd;
        double captureRadius_m = 5.0;
        double maxRudderAngle_rad = 0.35;  // ~20 deg
    };

    explicit BerthManeuverSystem(Config cfg);

    void update(double t, double dt, const NavalContext& ctx,
                std::vector<Thruster>& thrusters, std::vector<Rudder>& rudders);

    Phase currentPhase() const noexcept;

private:
    Config      cfg_;
    Phase       phase_ = Phase::Navigating;
    std::size_t activeWpt_ = 0;
    // PIDs internos para cada fase
};
```

Rebocadores são paramétricos: `Forces` de cada rebocador são calculadas em `update()`
e injetadas diretamente no `NavalContext` não — elas precisam ser aplicadas como
`ForceModel` separado. **Decisão:** `BerthManeuverSystem` calcula e acumula as forças
de rebocador em um membro interno `tugForces_` (Vector6 por corpo), exposto via
`tugForces() const`. `NavalSimulation` ou o test setup cria um `TugForces` que lê desse
membro antes do step. (Ver seção de sequenciamento — isso é resolvido no passo 6 do build.)

Transições de estado:
- `Navigating → Sideway`: distância ao waypoint ativo < `transitionDist_m`
- `Sideway → TurnROTTUG`: erro lateral < 2.0m e ângulo heading < 5°
- Sem transições reversas

#### NavalSimulation Refatorado

```cpp
// libs/simulation/include/ymir/simulation/NavalSimulation.h
class NavalSimulation {
public:
    NavalSimulation() = default;

    void addBody(int id, std::unique_ptr<RigidBody6DOF> body);
    void addNavalForceModel(int bodyId, std::unique_ptr<NavalForceModel> model);
    void registerVessel(int bodyId, DynamicVessel& vessel,
                        ThrustForces* tf = nullptr, RudderForces* rf = nullptr);

    void initialize();
    void step(double dt);
    void reset();
    void setEnvironment(const NavalEnvironment& env);

    BodyState state(int bodyId) const;
    double    time()  const noexcept;

private:
    struct BodyEntry {
        RigidBody6DOF*                body   = nullptr;
        std::vector<NavalForceModel*> models;
        NavalContext                  ctx;
        Vector6                       q_avg{};
        DynamicVessel*                vessel = nullptr;
        ThrustForces*                 thrust = nullptr;
        RudderForces*                 rudder = nullptr;
    };

    Simulation              sim_;
    NavalEnvironment        env_{};
    std::map<int, BodyEntry> entries_;

    NavalContext buildContext(BodyEntry& entry, double dt) const;
};
```

A ordem de iteração de `std::map<int>` é por ID crescente — determinística.

### Modificações em ThrustForces / RudderForces

**ThrustForces.h** — adições:
```cpp
struct ThrusterCommand {
    double currentRPM         = 0.0;
    double currentAzimuth_deg = 0.0;
    double currentPitch       = 0.0;
};
void setActuatorState(std::size_t id, const ThrusterCommand& cmd) noexcept;
```

Remoções: `currentRPM_` (array interno), filtro de 1ª ordem em `computeNaval()`,
mutação de `cfg_.thrusters[i].commandRPM`.

`computeNaval()` lê `commands_[i].currentRPM` para calcular thrust. Não atualiza mais
o estado — apenas lê.

**RudderForces.h** — adições:
```cpp
struct RudderCommand {
    double currentAngle_rad = 0.0;
};
void setActuatorState(std::size_t id, const RudderCommand& cmd) noexcept;
```

Remoções: `currentAngle_` interno, rate limiter em `computeNaval()`.

**VesselConfig.h** — modificação:
- `ThrusterConfig.commandRPM` → renomeado para `initialRPM` (valor padrão = 0.0).
- Sem mudança de layout — apenas semântica (config é imutável após construção do Thruster).

### VesselState

```cpp
// libs/vessel/include/ymir/vessel/VesselState.h — header-only, nenhum .cpp
enum class NavigationLight  { Off, On };
enum class ColregsShape     { Ball, Cone, Cylinder, Diamond };
enum class OperationalState {
    Underway, Anchored, Moored, Aground,
    RestrictedManeuverability, NotUnderCommand
};

struct NavigationLights {
    NavigationLight mast      = NavigationLight::Off;
    NavigationLight port      = NavigationLight::Off;
    NavigationLight starboard = NavigationLight::Off;
    NavigationLight stern     = NavigationLight::Off;
    NavigationLight anchor    = NavigationLight::Off;
};

struct VesselState {
    NavigationLights          lights;
    std::vector<ColregsShape> shapes;
    OperationalState          operationalState = OperationalState::Underway;
};
```

Sem lógica. Transições setadas pela aplicação ou pelo `NavalSimulation` via
`vessel.vesselState().operationalState = OperationalState::Anchored`.

### Estrutura de Arquivos

```
libs/vessel/
  CMakeLists.txt                                  ← INTERFACE → STATIC + src files
  include/ymir/vessel/
    Thruster.h                                    (novo)
    Rudder.h                                      (novo)
    VesselState.h                                 (novo, header-only)
    DynamicVessel.h                               (novo)
    controllers/
      ManeuverController.h                        (novo)
      PrescribedController.h                      (novo)
      BerthManeuverSystem.h                       (novo)
  src/
    Thruster.cpp                                  (novo)
    Rudder.cpp                                    (novo)
    DynamicVessel.cpp                             (novo)
    controllers/
      ManeuverController.cpp                      (novo)
      PrescribedController.cpp                    (novo)
      BerthManeuverSystem.cpp                     (novo)
  tests/
    CMakeLists.txt                                (novo)
    TestThruster.cpp                              (novo)
    TestRudder.cpp                                (novo)
    TestManeuverController.cpp                    (novo)
    TestPrescribedController.cpp                  (novo)
    TestBerthManeuverSystem.cpp                   (novo)

libs/physics/forces/
  ThrustForces.h / .cpp                          ← modificado
  RudderForces.h / .cpp                          ← modificado

libs/simulation/
  include/ymir/simulation/NavalSimulation.h      ← modificado
  src/NavalSimulation.cpp                        ← modificado

libs/vessel/ CMakeLists.txt (após mudança):
```
```cmake
add_library(ymir_vessel STATIC
    src/Thruster.cpp
    src/Rudder.cpp
    src/DynamicVessel.cpp
    src/controllers/ManeuverController.cpp
    src/controllers/PrescribedController.cpp
    src/controllers/BerthManeuverSystem.cpp
)
add_library(Ymir::Vessel ALIAS ymir_vessel)

target_include_directories(ymir_vessel
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
)
target_link_libraries(ymir_vessel PUBLIC ymir_physics ymir_common)
target_set_warnings(ymir_vessel)

if(YMIR_BUILD_TESTS)
    add_subdirectory(tests)
endif()
```

---

## Análise de Impacto

| Componente | Tipo | Descrição e Risco | Ação Necessária |
|---|---|---|---|
| `libs/vessel/` | Modificado | INTERFACE → STATIC; novos headers e fontes | Atualizar CMakeLists.txt |
| `ThrustForces` | Modificado | Remove `currentRPM_`; add `setActuatorState()` | Atualizar .h e .cpp; migrar testes |
| `RudderForces` | Modificado | Remove `currentAngle_`; add `setActuatorState()` | Atualizar .h e .cpp; migrar testes |
| `VesselConfig` | Modificado | `commandRPM` → `initialRPM` em `ThrusterConfig` | Atualizar todos os cenários JSON/código |
| `NavalSimulation` | Modificado | API quebrada: construtor, `state()`, `addBody()` | Migrar todos os callers e testes |
| `TestNavalInfra.cpp` | Modificado | Usa `NavalSimulation sim(makeTestBody())` | Migrar para novo construtor |
| `TestNavalIntegration.cpp` | Modificado | Idem | Idem |
| `TestSimulationIntegration.cpp` | Verificar | Pode usar NavalSimulation indiretamente | Verificar e migrar se necessário |

---

## Abordagem de Testes

### Testes Unitários

Localizados em `libs/vessel/tests/`.

**TestThruster.cpp:**
- Filtro de 1ª ordem: `Thruster t(cfg_τ50); t.setDemand(100, 0, 0); t.update(1.0);`
  → `REQUIRE(t.state().currentRPM == Approx(100 * (1 - std::exp(-1.0/50))).epsilon(1e-6))`
- Rate limiter de azimuth: demanda 90°, rate 5°/s, dt=1s → estado após update = 5°.
- Múltiplos updates acumulam: após N updates de dt=1s com rate=5°/s → estado ≤ demanda.
- `setDemand` não muta estado imediatamente — estado só muda após `update(dt)`.

**TestRudder.cpp:**
- Rate limiter simétrico (positivo e negativo).
- Não ultrapassa `cfg.angleMaximum_rad`.
- `toCommand()` retorna o ângulo atual (não o demandado).

**TestPrescribedController.cpp:**
- Interpolação no ponto exato: `t = times[i]` → `value == values[i]` (erro = 0).
- Interpolação entre pontos: erro < 1e-6.
- `t` além do último ponto → retorna `values.back()`.
- `t` antes do primeiro ponto → retorna `values.front()`.

**TestManeuverController.cpp:**
- Navio em `(0,0)`, waypoint em `(100,0)`, heading = 0 → bearing LOS = 0 → erro heading = 0 → demanda de leme = 0.
- Navio dentro de `captureRadius` de waypoint 0 → `activeWaypointIdx() == 1`.
- Lista esgotada → `waypointsExhausted() == true` → todas as demandas = 0.

**TestBerthManeuverSystem.cpp:**
- `currentPhase() == Phase::Navigating` no estado inicial.
- Simular navio dentro de `transitionDist_m` → `currentPhase() == Phase::Sideway`.
- Nenhuma transição reversa possível: Sideway→Navigating não ocorre.

### Testes de Integração

Localizados em `tests/simulation/` (existentes + novos).

- **Waypoint end-to-end**: navio com `ManeuverController`, cenário A→B→C, rodar N steps,
  `REQUIRE(distanceToC < captureRadius)`.
- **Replay fidelidade**: `PrescribedController` com série sintética (coseno); comparar
  ângulo integrado vs analítico; erro posicional < 0.1m após 60s.
- **N corpos independentes**: dois `RigidBody6DOF` em `NavalSimulation`, forças nulas,
  estados iniciais diferentes; após 10 steps, `state(0)` e `state(1)` permanecem
  distintos e nenhum cruza com o outro.
- **Migração de testes existentes**: `TestNavalInfra.cpp` e `TestNavalIntegration.cpp`
  migrados para nova API de `NavalSimulation` com semântica equivalente.

---

## Sequência de Desenvolvimento

### Ordem de Build

1. **Modificar `ThrustForces`** — adicionar `ThrusterCommand` struct e `setActuatorState()`;
   remover `currentRPM_` e filtro interno. Atualizar `.cpp`. Sem novas dependências.

2. **Modificar `RudderForces`** — análogo ao passo 1. Sem novas dependências.

3. **Atualizar `VesselConfig`** — renomear `commandRPM` → `initialRPM`. Atualizar cenários.
   Sem novas dependências.

4. **Criar `Thruster`** — depende de `ThrusterConfig` (passo 3) e
   `ThrustForces::ThrusterCommand` (passo 1).

5. **Criar `Rudder`** — depende de `RudderConfig` (passo 3) e
   `RudderForces::RudderCommand` (passo 2).

6. **Criar `VesselState`** — header-only, sem dependências além de `<vector>`.

7. **Criar `PrescribedController`** — depende de Thruster (passo 4), Rudder (passo 5),
   `NavalContext`.

8. **Criar `ManeuverController`** — depende de Thruster (passo 4), Rudder (passo 5),
   `NavalContext`. PID struct é interno ao `.cpp`.

9. **Criar `BerthManeuverSystem`** — depende de ManeuverController (passo 8),
   Thruster, Rudder, NavalContext.

10. **Criar `DynamicVessel`** — depende de todos os controladores (passos 7-9),
    Thruster, Rudder, VesselState, ThrustForces, RudderForces.

11. **Refatorar `NavalSimulation`** — depende de DynamicVessel (passo 10). Migrar testes
    existentes como parte deste passo.

12. **Atualizar `libs/vessel/CMakeLists.txt`** — INTERFACE → STATIC; adicionar fontes;
    adicionar `tests/` subdirectory.

13. **Criar testes unitários** — um arquivo por classe (passos 4-9), mais `TestDynamicVessel.cpp`.

14. **Criar testes de integração** — cenários end-to-end + migração dos testes existentes.

### Dependências Técnicas

- Nenhum bloqueio externo — CVODE, Catch2 e CMake já configurados.
- `libs/vessel/` precisa ter `target_set_warnings` ativo (como `libs/simulation/`).
- Coverage: `YMIR_ENABLE_COVERAGE` já funcional; basta incluir `libs/vessel/` no relatório.

---

## Considerações Técnicas

### Decisões Chave

**Despacho de controlador via `std::variant`**: zero overhead de vtable no hot path de
tick; state do controlador ativo acessível diretamente por tipo; erro de compilação se
método `update()` estiver ausente. Extensibilidade futura exige alterar o alias —
trade-off aceitável dado que os três controladores são tipos fechados do domínio Ymir.
(Ver ADR-002.)

**Estado de atuador nas entidades, não nos modelos de força**: `Thruster::update()` e
`Rudder::update()` são chamados por `DynamicVessel::updateStates()` — antes do passo de
integração CVODE. `ThrustForces::computeNaval()` lê o estado já avançado via
`commands_[i]`. Testabilidade do atuador em isolamento é o benefício principal.
(Ver ADR-003.)

**`NavalSimulation` N-bodies com `std::map<int, BodyEntry>`**: iteração em ordem de ID
garante determinismo. `BodyEntry` agrupa todos os ponteiros não-owners de um corpo —
evita múltiplos maps sincronizados. (Ver ADR-004.)

**`BerthManeuverSystem::tugForces()` como vetor exposto**: tugs são paramétricos (sem
corpos independentes na Fase 1). A força calculada pelo BSM precisa entrar no corpo
como `ForceModel`. Solução: um `TugParametricForces : NavalForceModel` que lê de um
ponteiro para `BerthManeuverSystem` a cada tick. O simulador cria esse wrapper e
o registra no corpo junto com o BSM. Isso evita que o BSM precise ser também um
`ForceModel` — responsabilidades separadas.

**Sem `std::optional` para controlador padrão**: `DynamicVessel` exige um controlador
no construtor. Se nenhum for fornecido pelo usuário, `PrescribedController{{}, {}}` com
tabelas vazias é o default — produz demandas zero, comportamento de "drift" esperado.

### Riscos Conhecidos

- **Migração de testes existentes**: `TestNavalInfra.cpp` e `TestNavalIntegration.cpp`
  dependem do construtor atual de `NavalSimulation`. A migração é mecânica mas deve ser
  feita no mesmo PR que o passo 11 para não deixar o CI quebrado em commits intermediários.

- **PID sem anti-windup**: integrador acumula quando demanda satura. No cenário de
  atracação (heading fixo por longo tempo), o ki de heading acumula erro grande → overshoot
  ao retomar Navigating. Mitigação na Fase 1: manter ki = 0 nos cenários de teste; estrutura
  do PID está pronta para receber clamp de integral sem breaking change.

- **`BerthManeuverSystem` requer posição de rebocador sem World**: em testes, as posições
  de rebocador são injetadas diretamente na config do BSM. Em produção (Fase 2), os
  rebocadores serão corpos independentes no World. A transição não requer mudança de
  interface — apenas a origem dos dados muda.

---

## Monitoramento e Observabilidade

Nenhum sistema de logging ou métricas de runtime nesta fase. A observabilidade é via:
- `DynamicVessel::vesselState()` por tick — estado operacional legível pela aplicação.
- `DynamicVessel::thruster(i).state()` — RPM e azimuth atuais por tick.
- `NavalSimulation::state(bodyId)` — posição, velocidade e orientação completas por tick.
- `BerthManeuverSystem::currentPhase()` — fase FSM atual.
- `ManeuverController::activeWaypointIdx()` e `waypointsExhausted()`.

---

## Registros de Decisão de Arquitetura

- [ADR-001: DynamicVessel como Hub de Integração](adrs/adr-001.md) — DynamicVessel como aggregate root que possui controladores, Thruster[] e Rudder[]; NavalSimulation mantém referência não-owner.
- [ADR-002: std::variant para Despacho de Controlador](adrs/adr-002.md) — ManeuverController/PrescribedController/BerthManeuverSystem selecionáveis em runtime via std::variant + std::visit; zero vtable overhead.
- [ADR-003: Thruster e Rudder como Entidades com Estado de Atuador](adrs/adr-003.md) — Estado de atuador sai de ThrustForces/RudderForces para entidades de domínio; tipos intermediários ThrusterCommand/RudderCommand evitam dependência circular.
- [ADR-004: Refatoração de NavalSimulation para N Corpos](adrs/adr-004.md) — API quebrada deliberada: construtor de corpo único removido, `state()` exige bodyId, tick itera sobre `std::map<int, BodyEntry>`.
