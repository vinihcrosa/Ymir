# PRD: Camada de Embarcação — Fase 1

## Visão Geral

A Camada de Embarcação transforma o motor de física Ymir em um simulador naval operacional.
Antes desta fase, corpos físicos existem mas não têm identidade naval — são massas com forças.
Depois dela, uma embarcação navega autonomamente, opera seus propulsores e lemes como
equipamentos reais com dinâmicas próprias, e expõe seu estado operacional de forma consumível
por aplicações externas.

O escopo é deliberadamente restrito: hidrodinâmica completa, sistema de controle, e estado
operacional. Mooring, anchoring e colisão são fases futuras.

Consumidores primários: engenheiros de simulação rodando estudos de dinâmica naval,
desenvolvedores integrando Ymir como motor de simulação, e pipelines de regressão
validando correção física.

---

## Objetivos

- Propulsores e lemes passam a ser entidades com estado dinâmico próprio — RPM atual,
  ângulo atual, demanda e filtros de atuador — não parâmetros passivos de config.
- Embarcações navegam autonomamente por listas de waypoints via autopiloto LOS + PID.
- Séries temporais de comandos gravados são reproduzidas dentro do simulador para
  comparação com dados físicos reais.
- Manobras de atracação assistidas por rebocadores são simuladas via FSM de 3 estados.
- Estado operacional da embarcação (luzes, shapes COLREGS, modo) é exposto por tick.
- A simulação suporta N corpos navais simultâneos, corrigindo a limitação de corpo único.
- Cobertura de testes ≥ 80% para cada controlador e para a camada de atuadores.
- Zero falhas de tick com qualquer controlador ativo.

---

## Histórias de Usuário

### Engenheiro de Simulação

- Quero configurar propulsores com parâmetros físicos reais e ver o RPM atual convergir
  para a demanda com a constante de tempo correta, para validar a dinâmica do atuador.
- Quero fornecer uma lista de waypoints e ter a embarcação navegar por eles sem intervenção,
  para estudar a dinâmica do navio ao longo de rotas operacionais reais.
- Quero reproduzir uma série temporal de comandos de leme e RPM gravados de ensaios físicos,
  para comparar a resposta do simulador com dados experimentais.
- Quero que o estado operacional da embarcação seja exposto por tick, para que meu sistema
  de logging anote a fase de cada manobra.

### Desenvolvedor de Plataforma de Treinamento

- Quero ler luzes de navegação e shapes COLREGS do estado da embarcação a cada tick,
  para renderizar a sinalização correta sem duplicar lógica de estado.
- Quero que a FSM de atracação exponha sua fase atual, para sincronizar narração de
  cenário com o comportamento da embarcação.

### Pipeline de CI

- Quero outputs determinísticos dado o mesmo cenário de entrada, para que testes de
  regressão não sejam frágeis.
- Quero que o erro de interpolação do replay prescrito seja < 1e-6, para que correção
  numérica seja verificável sem intuição de domínio.

---

## Funcionalidades Principais

### 1. Propulsores e Lemes como Entidades

Cada propulsor e cada leme é uma entidade com parâmetros físicos estáticos e estado
dinâmico que evolui a cada tick.

**Propulsor:** carrega RPM demandado, RPM atual (filtrado por constante de tempo),
pitch demandado/atual, e azimuth demandado/atual. A cada tick, o estado avança
aplicando filtro de primeira ordem no RPM e limitador de taxa no azimuth. A força de
empuxo é calculada a partir do estado atual — não da demanda.

**Leme:** carrega ângulo demandado e ângulo atual. A cada tick, o ângulo avança até
a demanda respeitando o limitador de taxa (rad/s). A força hidrodinâmica usa o ângulo atual.

Essa separação torna verificável: dado RPM demandado = 100%, RPM inicial = 0%, e
constante de tempo = 50s, o RPM atual após 1s é analiticamente calculável e testável.

### 2. DynamicVessel — Aggregate Root da Embarcação

`DynamicVessel` é o ponto de entrada para controlar e observar uma embarcação. Possui
os propulsores, lemes e o controlador ativo. Expõe dois métodos chamados pelo simulador
a cada tick:

- **`updateControl`**: executa o controlador ativo, que escreve demandas nos propulsores
  e lemes.
- **`updateStates`**: avança a dinâmica de cada atuador, transforma ambiente para
  referencial do corpo, computa cinemática derivada (SOG, COG, ângulo de deriva).

### 3. ManeuverController — Autopiloto por Waypoints

Navega a embarcação por uma lista de waypoints fornecida pelo usuário. A cada tick
calcula o bearing LOS para o waypoint ativo e opera dois PIDs independentes — um de
heading e um de velocidade — produzindo demanda de leme e de RPM.

- Waypoint capturado quando distância < raio de captura configurável → avança para o próximo.
- Lista esgotada → modo drift (demandas zero).
- Configurável: waypoints com posição e velocidade demandada, raio de captura, seis ganhos PID.

### 4. PrescribedController — Replay de Série Temporal

Reproduz sequências pré-gravadas de comandos de leme e RPM por interpolação linear
em tabelas indexadas por tempo. Uma tabela por leme, uma por propulsor.

- Além do último ponto temporal: mantém último valor.
- Erro de interpolação < 1e-6 é requisito de correção.
- Uso principal: validação contra dados de tanque de reboque ou ensaios de manobrabilidade.

### 5. BerthManeuverSystem — FSM de Atracação com Rebocadores

FSM de três estados que orquestra a estratégia de controle durante uma aproximação
de atracação. Rebocadores são paramétricos nesta fase (forças configuradas, não corpos
independentes).

| Estado | Estratégia | Rebocadores |
| --- | --- | --- |
| `navigating` | LOS + PID heading e velocidade | ESCORTING |
| `sideway` | Heading fixo, deslocamento lateral via rebocadores PD | PUSH |
| `turnROTTUG` | PID de Rate-of-Turn via leme, sway via rebocadores | PUSH |

Transições definidas por limiares de distância, erro de heading e erro lateral.
Cada waypoint de atracação carrega o tipo de controlador que define o estado alvo.

### 6. VesselState — Estado Operacional Visível

Contêiner de estado exposto por tick. Não executa cálculos.

**Luzes de navegação:** mastLight, portLight, starboardLight, sternLight, anchorLight,
specialLights configuráveis.

**Shapes COLREGS:** ball, cone, cylinder, diamond, lista de shapes ativos.

**Estado operacional:** Underway, Anchored, Moored, Aground, RestrictedManeuverability,
NotUnderCommand.

Transições são manuais via API nesta fase. Âncora e amarração operam via modelos de
força externos à embarcação — o `VesselState` não recebe eventos dessas subsistemas
porque a embarcação não tem visibilidade do que acontece fora dela. O estado `Anchored`
é setado pela aplicação quando ela decide que a âncora prendeu.

### 7. Suporte a N Corpos Navais

O simulador passa a suportar múltiplos corpos navais simultâneos. Cada corpo tem seu
próprio conjunto de atuadores, controlador, e contexto naval. Isso permite simular
frota com navio e rebocadores independentes (sem acoplamento de força entre eles nesta
fase — acoplamento via CouplingPort é Fase 2).

---

## Experiência do Usuário

### Fluxo: navegação autônoma por waypoints

1. Usuário instancia `DynamicVessel` com config de casco, propulsores e lemes.
2. Usuário configura `ManeuverController` com lista de waypoints, raio de captura e ganhos PID.
3. Usuário chama `step(dt)` em loop; embarcação navega sem intervenção.
4. Ao final, usuário lê estado final e VesselState para verificar que chegou a cada waypoint.

### Fluxo: validação contra dados experimentais

1. Usuário carrega tabelas de leme e RPM de ensaio físico no `PrescribedController`.
2. Usuário roda simulação com as mesmas condições iniciais do ensaio.
3. Usuário compara trajetória do simulador com a trajetória gravada; erro de interpolação
   < 1e-6 garante que diferenças são físicas, não numéricas.

### Fluxo: estudo de atracação

1. Usuário configura `BerthManeuverSystem` com waypoints de atracação e forças de rebocadores.
2. Usuário monitora `VesselState.berthPhase` a cada tick para rastrear progressão da FSM.
3. Simulação termina quando FSM completa todos os estados e a embarcação para na posição alvo.

---

## Restrições Técnicas de Alto Nível

- `DynamicVessel` reside em `libs/vessel/` no namespace `ymir::vessel`.
- Zero alocação de heap dentro do loop de tick; todo estado pré-alocado nos construtores.
- Estado atual de atuadores é propriedade das entidades (Thruster, Rudder) — nenhum
  campo de config é mutado em runtime.
- PrescribedController: erro de interpolação < 1e-6.
- Cobertura ≥ 80% por módulo, medida via `YMIR_ENABLE_COVERAGE`.
- `DynamicVessel` é não-copiável e não-movível.

---

## Fora do Escopo

- Acoplamento de forças entre corpos (cabo navio-rebocador) — Fase 2 via CouplingPort.
- Rebocadores como corpos independentes com física própria — Fase 2.
- Mooring e Anchoring — fora do roadmap atual.
- Colisão — Fase 4.
- API externa — Fase 3.
- JSON scenario reader para waypoints e config de controlador — Fase 3.
- Anti-windup no PID — adiado; interface projetada para adição futura sem breaking change.
- Batimetria como campo espacial — profundidade é scalar global.

---

## Plano de Entrega

### MVP — Fase 1 (este PRD)

- Entidades Thruster e Rudder com dinâmica própria
- DynamicVessel com updateControl / updateStates
- ManeuverController (LOS + PID)
- PrescribedController (interpolação linear)
- BerthManeuverSystem (FSM 3 estados, rebocadores paramétricos)
- VesselState (luzes, shapes, estado operacional, transições manuais)
- N corpos navais no simulador
- Testes Catch2 cobrindo todos os módulos

**Critérios para avançar para Fase 2:**

- Embarcação navega A→B→C sem desvio > raio de captura
- FSM completa navigating → sideway → turnROTTUG sem transição ilegal
- Replay prescrito: erro < 1e-6 em toda a série temporal
- Filtro de RPM de primeira ordem: resultado analítico verificado
- Zero exceções com qualquer controlador ativo
- Cobertura ≥ 80%

---

## Métricas de Sucesso

- Rastreamento de waypoint: erro posicional ≤ raio de captura em 100% dos waypoints
- FSM: transições corretas em 100% das execuções de teste
- Replay: erro de interpolação < 1e-6
- Dinâmica de atuador: RPM filtrado corresponde à solução analítica dentro de tolerância de ponto flutuante
- Estabilidade: zero falhas de tick em todos os cenários de teste
- Cobertura: ≥ 80% por módulo

---

## Riscos e Mitigações

| Risco | Mitigação |
| --- | --- |
| Ganhos PID de teste exigem ajuste empírico | Usar cenários com solução analítica conhecida (heading constante, zero cross-track) |
| BerthManeuverSystem precisa de posição de rebocador sem World existir | Injetar posições de rebocador programaticamente nos testes da Fase 1 |
| Refatoração de ThrustForces / RudderForces para usar entidades quebra testes existentes | Migrar testes junto com a refatoração; cobertura antes e depois deve ser equivalente |
| N corpos com NavalContext por corpo aumenta custo de tick | NavalContext é struct leve; custo é linear no número de corpos — aceitável |

---

## Registros de Decisão de Arquitetura

- [ADR-001: DynamicVessel como aggregate root com entidades Thruster e Rudder](adrs/adr-001.md) — Propulsores e lemes são entidades com estado e comportamento próprios; DynamicVessel os possui e orquestra o tick de controle.

---

## Questões Abertas

- Despacho de controlador por `std::variant` (decidido, D7): os controladores precisam
  de interface comum implícita para `std::visit` — definir o conceito de controlador
  (quais métodos) é tarefa da TechSpec.
- `VesselState.berthPhase` exposto como campo direto ou como método de consulta?
- Guincho (`Winch`) é incluído na Fase 1 como atuador de comprimento de linha, ou
  adiado para quando MooringLine for implementada?
