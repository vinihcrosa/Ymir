# Contexto: Embarcação

Representa navios e seus estados operacionais.
Nenhum módulo deste contexto executa cálculos físicos — apenas descrevem e mantêm estado.

---

## Vessel

Concentra todas as características permanentes e operacionais de uma embarcação.

### Características físicas

- Dimensões (LOA, boca, pontal, calado)
- Massa e tensor de inércia (6×6)
- Massa adicionada (6×6) — coeficientes hidrodinâmicos WAMIT
- `invTotalMass = (M + A)⁻¹` — calculada uma vez no construtor, usada a cada tick
- Volume de deslocamento, coeficiente de bloco (Cb), centro de gravidade (cg), centro de flutuação (cf)
- Matriz de rigidez hidrostática (`hydro_rest`) — fornecida por WAMIT
- Tabelas WAMIT: RAO de força, RAO de movimento, mean drift, por frequência e ângulo de incidência

### Propulsores

Cada propulsor tem:

- Tipo: fixo, controlável (CP), azimutal (Z-drive)
- Posição no casco `[x, y, z]` relativa à origem WAMIT
- Tabelas open-water: `J → Kt(J)`, `J → Kq(J)`
- Parâmetros: diâmetro, `hullEfficiency` (wake fraction), `asternEfficiency`, `rotationTime` (constante de tempo 1ª ordem)
- Power limit: potência máxima em kW
- Azimutal: `azimuth_speed` graus/s (rate limiter)

Estado dinâmico dos atuadores (atualizado a cada tick em `updateStates`):

```
// Rotação — FilterActuator 1ª ordem:
Aa = 1 / (1 + rotationTime / dt)
n_current = Aa · n_current + (1-Aa) · n_demanded

// Azimute — RateLimitedActuator:
delta = clamp(az_demanded - az_current, -azimuth_speed·dt, +azimuth_speed·dt)
az_current += delta
```

### Lemes

Cada leme tem:

- Posição no casco `[x, y, z]`
- Área do perfil, tabelas `beta → Cl(beta)`, `beta → Cd(beta)`
- `speed` graus/s (rate limiter do ângulo)
- Referência ao propulsor associado (para slipstream)

### Pontos de amarração e fundeio

- Lista de fairleads com posição no casco (usada por Mooring e Anchoring)
- Configuração de âncoras: comprimento e peso linear da amarra

### Sistema de controle/manobra

Selecionado por configuração da simulação. Quatro modos:

| Modo | Comportamento |
|------|--------------|
| `drift` | Sem propulsão, deriva sob vento/corrente/ondas |
| `simulation` | Forças externas pré-definidas, sem autopiloto |
| `maneuver` | Autopiloto por waypoints — LOS + PID de heading e velocidade |
| `berthManeuver` | Atracação com rebocadores — FSM de 3 estados |

**ManeuverController (modo `maneuver`):**

```
1. LOS bearing = atan2(wp.x - x, wp.y - y)
2. heading_err = wrap180(LOS_bearing - heading)
3. rudder_cmd  = Kp·err + Ki·∫err·dt + Kd·Δerr/dt
4. speed_err   = v_demanded - SOG → PID → rpm_cmd
5. transição: if dist(vessel, wp) < captureRadius → próximo waypoint
```

**BerthManeuverSystem (modo `berthManeuver`):**

FSM por waypoint, `controllerType` define o estado:

- `navigating` — LOS + PID de heading/velocidade, rebocadores em ESCORTING
- `sideway` — heading fixo, rebocadores PUSH controlam erro lateral por PD
- `turnROTTUG` — PID de ROT (Rate of Turn) via leme, rebocadores PUSH para sway

**PrescribedController** — replay de séries temporais de comandos pré-gravados. Interpolação linear por tempo. Usado para comparação com dados físicos reais ou reprodução de manobras.

### Cinemática derivada (calculada em `updateStates` a cada tick)

```
SOG = sqrt(u_inercial² + v_inercial²)
COG = atan2(u_inercial, v_inercial)   // 0=Norte, convenção náutica
driftAngle = wrap_pi(COG - yaw)
speedToWater[0,1] = velocidade relativa da corrente no frame do navio
```

---

## VesselState

Representa o estado operacional visível da embarcação — informações que não afetam a física mas fazem parte do estado da simulação e são visíveis no Viewer.

**Responsabilidades:**

- Luzes de navegação (mastro, bordo, popa, âncora, especiais)
- Shapes diurnos (bola, cone, cilindro, losango — indicam estado operacional)
- Estado operacional (navegando, fundeado, amarrado, manobra restrita, etc.)
- Sinalizações visuais adicionais

**Notas:**

- Não executa cálculos
- Estado atualizado por comandos externos (API) ou por eventos internos (ex: âncora lançada → acende luz de fundeio)
- Dados consumidos pelo Viewer para renderização correta das luzes e shapes
