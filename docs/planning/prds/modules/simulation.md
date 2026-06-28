# Contexto: Orquestração

Coordena a execução da simulação e a comunicação entre módulos.

---

## Simulation

Controla o avanço do tempo e garante que todos os módulos sejam executados na ordem correta.

### Responsabilidades

- Iniciar, encerrar, pausar e retomar uma simulação
- Controlar velocidade (real-time 1×, acelerado N×)
- Executar ciclos de atualização (ticks) em ordem determinística
- Em real-time: sincronizar loop com tempo de relógio
- Em fast-time: loop roda sem sincronização (o mais rápido possível)

### Ciclo de tick

A cada tick, a Simulation executa na ordem:

```
1. [Environment]    atualiza condições ambientais (vento, corrente, maré)
2. [Terrain]        aplica modificações de terreno pendentes
3. [Vessel]         updateControl(time, dt) — atualiza demands de leme/RPM se modo manobra
4. [Vessel]         updateStates(time, dt)  — transforma corrente/vento p/ frame do navio,
                                              atualiza atuadores (1ª ordem / rate limiter),
                                              calcula SOG/COG/speedToWater, EMA de posição
5. [Hydrodynamics]  calcula forças hidrodinâmicas (damping, current, squat, thrust, rudder, tug)
6. [WaveForces]     calcula forças de onda (excitação + drift + RAO)
7. [InertialForces] acoplamento Coriolis/giroscópico
8. [RestoringForces] restauração hidrostática
9. [Mooring]        forças de cabos de amarração
10. [Anchoring]     forças de fundeio
11. [Physics/CVODE] soma todas as forças → integra movimento → atualiza World
12. [Events]        publica eventos do tick para assinantes internos e externos
```

### Integrador — CVODE

- `dt` passado à Simulation é o **intervalo de output** (ex: 0.05–0.1 s)
- CVODE usa sub-passos adaptativos internamente — pode usar passos menores que `dt` para manter tolerância
- Múltiplos corpos: cada um tem sua própria instância CVODE (estado e solver independentes)

### Múltiplas simulações

- Cada simulação é um par `Simulation + World` independente
- No `apps/server`: isolamento por thread (1–2 simulações real-time)
- No `apps/fast-time`: isolamento por processo (N simulações batch em paralelo)

---

## Events

Sistema de pub/sub para comunicação entre módulos internos e clientes externos.

### Responsabilidades

- Receber eventos publicados por qualquer módulo durante um tick
- Entregar a assinantes internos (outros módulos, sem latência)
- Propagar para clientes externos via Server (Manager, Viewer) — apenas em real-time

### Eventos publicados

| Evento | Publicado por | Descrição |
|--------|--------------|-----------|
| `collision` | Physics | Dois corpos em contato |
| `entity_created` | World | Nova entidade adicionada |
| `entity_removed` | World | Entidade removida |
| `mooring_ruptured` | Mooring | Cabo de amarração rompeu |
| `anchor_holding` | Anchoring | Âncora unhada no fundo |
| `anchor_dragging` | Anchoring | Âncora garreando |
| `environment_changed` | Environment | Condição ambiental alterada |
| `terrain_changed` | Terrain | Batimetria ou estrutura modificada |
| `simulation_started` | Simulation | Simulação iniciada |
| `simulation_paused` | Simulation | Simulação pausada |
| `simulation_ended` | Simulation | Simulação encerrada |
| `vessel_state_changed` | VesselState | Luzes/shapes/estado operacional mudaram |

### Notas

- Fast-time: eventos propagados apenas internamente, não para clientes externos
- Histórico de eventos persistido por Persistence (junto com o histórico de steps)
- Viewer e Manager subscrevem todos os eventos da simulação em real-time
