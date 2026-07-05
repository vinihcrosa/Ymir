# PRD — Simulação no Servidor (Server-Side Simulation)

## Overview

Hoje a simulação de física do Ymir roda **no browser do usuário**: cada aba instancia seu próprio
motor (WASM) e executa o loop de tick localmente. Este documento descreve mover a simulação para o
**servidor**, transformando o cliente web em um **terminal remoto** que envia comandos e recebe o
estado da simulação em tempo real.

**Problema que resolve:**
- Existem hoje **dois motores** de física (o WASM real e um fallback cinemático em JavaScript) que
  podem divergir de comportamento — não há uma fonte de verdade única.
- A simulação é presa ao cliente: morre ao fechar a aba, não pode ser retomada, e não pode ser
  compartilhada nem persistida.

**Para quem é:** operadores do simulador naval (quem controla embarcações, leme, propulsão e
condições ambientais e observa o resultado) e a equipe de desenvolvimento do Ymir, que passa a
manter um único motor.

**Por que é valioso:**
1. **Motor único** — elimina a divergência entre motores e estabelece uma fonte de verdade.
2. **Fundação** — a simulação autoritativa no servidor é o pré-requisito para persistir/retomar
   runs e, no futuro, permitir múltiplos usuários acessando diferentes simulações.

## Goals

- Rodar 100% da simulação no servidor; o cliente não executa nenhuma física localmente.
- Preservar **paridade funcional completa** com o comportamento atual do usuário (nada que ele faz
  hoje pode deixar de funcionar).
- Manter a simulação **viva no servidor** independentemente da conexão do cliente, com **reconexão**
  que retoma o estado atual.
- Reduzir para **um único motor** de física mantido pelo time.
- Entregar a fundação sem introduzir persistência em disco nem funcionalidades novas de comando.

**Marco de sucesso do MVP:** um usuário abre o cliente, cria/carrega um cenário, controla a
embarcação e o ambiente, vê a simulação evoluir em tempo real; ao fechar e reabrir a aba, reconecta
à mesma simulação em andamento — tudo com o comportamento idêntico ao de hoje.

## User Stories

**Persona primária — Operador da simulação**

- Como operador, quero **carregar um cenário** e iniciar a simulação, para observar o comportamento
  das embarcações.
- Como operador, quero **dar play, pausar e resetar** a simulação, para controlar sua execução.
- Como operador, quero **comandar leme e propulsão (RPM/thruster)** de uma embarcação, para manobrá-la.
- Como operador, quero **ajustar as condições ambientais** (corrente, vento, ondas), para ver a
  resposta da embarcação.
- Como operador, quero **ver as posições e estados das embarcações atualizando em tempo real**, para
  acompanhar a manobra.
- Como operador, quero que, ao **fechar e reabrir** o cliente, a simulação continue de onde estava,
  para não perder a run em andamento.

**Persona secundária — Desenvolvedor Ymir**

- Como desenvolvedor, quero um **único motor de física** rodando no servidor, para não manter dois
  caminhos que divergem.
- Como desenvolvedor, quero que o **contrato de estado** entre servidor e cliente reuse os tipos já
  existentes, para reduzir superfície nova de manutenção.

## Core Features

Prioridade **P0** = obrigatório no MVP.

### P0 — Simulação hospedada no servidor
Toda a física roda em um processo no servidor. O servidor é dono do loop de tick e do estado
autoritativo. Cada simulação tem uma identidade própria para permitir reconexão.

### P0 — Cliente como terminal remoto
O cliente web deixa de rodar física. Ele conecta à simulação no servidor, envia comandos e renderiza
o estado recebido. A experiência do usuário no cliente permanece a mesma de hoje.

### P0 — Canal de comandos (paridade com hoje)
O cliente pode enviar ao servidor exatamente o conjunto atual de ações:
- Criar/carregar cenário
- Play / Pausar / Resetar
- Comando de leme
- Comando de propulsão (RPM/thruster)
- Ajuste de condições ambientais (corrente, vento, ondas)

### P0 — Stream de estado em tempo real
O servidor transmite o estado da simulação (tempo e estado de cada embarcação: posição, atitude,
velocidades) em tempo real, na mesma cadência do comportamento atual, para o cliente renderizar.

### P0 — Simulação persistente na sessão + reconexão
A simulação continua rodando no servidor mesmo se o cliente cair ou fechar. Ao reconectar, o cliente
recebe o snapshot atual e retoma a observação/controle.

### P0 — Motor único
Remoção do fallback cinemático (mock) e do motor local do cliente. A física passa a ter uma única
implementação, no servidor.

## User Experience

**Jornada principal (idêntica à atual do ponto de vista do usuário):**

1. Usuário abre o cliente e monta/carrega um cenário (embarcações, área, ambiente).
2. Dá **play** — a simulação inicia no servidor e as embarcações começam a se mover na tela.
3. Seleciona uma embarcação e ajusta **leme** e **propulsão**; a embarcação responde.
4. Ajusta **condições ambientais**; a resposta hidrodinâmica muda.
5. **Pausa** / **reseta** conforme necessário.
6. Fecha a aba. A simulação **continua** no servidor.
7. Reabre o cliente e **reconecta** — vê a simulação no ponto atual e retoma o controle.

**Considerações de UX:**
- O render deve permanecer suave mesmo que o estado chegue em cadência de rede (o cliente pode
  interpolar entre snapshots) — do ponto de vista do usuário, o movimento não deve "picotar".
- Estados de conexão devem ser visíveis: conectando, conectado, reconectando, sem servidor.
- Enquanto um comando ainda não foi confirmado pelo servidor, a UI deve refletir que a ação está
  em trânsito (sem prometer um resultado que a física ainda não produziu).
- Sem servidor disponível, o cliente comunica claramente que a simulação está indisponível (não há
  modo offline).

## High-Level Technical Constraints

- **Reuso do motor existente:** a física do servidor deve produzir o mesmo comportamento do motor
  atual — sem reimplementar a dinâmica naval.
- **Reuso do contrato de dados:** o estado transmitido deve reusar os tipos de estado já definidos
  no projeto, evitando um contrato paralelo.
- **Separação de responsabilidades:** o acesso a dados de projeto (cenários, embarcações, áreas)
  permanece no canal HTTP existente; o novo canal é exclusivo para o ciclo de vida e o controle da
  simulação em tempo real.
- **Desempenho na perspectiva do usuário:** atualização de estado em tempo real (cadência atual de
  ~20 Hz) e resposta perceptível a comandos em menos de ~100 ms na rede local.
- **Sem autenticação nesta fase:** operação em rede local/confiável.

*(Detalhes de biblioteca de rede, threading, formato de serialização e limpeza de sessões são
decisões de TechSpec, não deste PRD.)*

## Non-Goals (Out of Scope)

- **Persistência em disco / histórico de runs (SQLite)** — apenas a fundação é entregue; gravar e
  retomar runs de disco fica para fase futura.
- **Múltiplos usuários simultâneos na mesma ou em diferentes simulações** — é o norte de longo prazo,
  não o MVP. O MVP garante que a arquitetura não impede isso.
- **Novos comandos** — sem fast-time (aceleração de tempo), sem waypoints/navegação autônoma, sem
  criação/remoção dinâmica de entidades além do que já existe.
- **Autenticação, multi-tenant e controle de acesso.**
- **Modo offline / física no cliente** — explicitamente removido.
- **Servidor C++ nativo com Protobuf** — descartado (ver ADR-001).
- **Mudanças no modelo físico** — nenhuma força, integrador ou parâmetro de embarcação muda.

## Phased Rollout Plan

### MVP (Fase 1) — Fundação
- Simulação rodando no servidor com identidade própria.
- Cliente como terminal remoto (sem física local).
- Canal de comandos com **paridade completa** com o cliente atual.
- Stream de estado em tempo real.
- Simulação persistente na sessão do servidor + reconexão com snapshot atual.
- Motor único (mock e worker local removidos).

**Critério para avançar:** um usuário completa a jornada principal com comportamento idêntico ao de
hoje, incluindo fechar/reabrir e reconectar à simulação em andamento; nenhuma capacidade atual é
perdida.

### Fase 2 — Persistência e retomada
- Gravação do histórico da run e retomada a partir do disco.
- Consulta a estados passados (rebobinar/reproduzir).

**Critério para avançar:** uma run pode ser encerrada, recuperada posteriormente e continuada ou
revisada.

### Fase 3 — Multiusuário e múltiplas simulações
- Vários usuários acessando diferentes simulações simultâneas.
- Distinção de papéis (controlador vs. observador) e possivelmente autenticação.

**Critério de sucesso de longo prazo:** múltiplos operadores trabalham em simulações distintas de
forma isolada e estável.

## Success Metrics

- **Paridade:** 100% das ações do cliente atual funcionam no novo modelo (checklist de paridade
  fechado).
- **Motores mantidos:** de 2 (WASM + mock) para **1**.
- **Tempo real:** estado atualiza na cadência atual (~20 Hz) sem regressão perceptível de suavidade.
- **Latência de comando→efeito:** < ~100 ms em rede local.
- **Reconexão:** ao reabrir o cliente, a simulação em andamento é retomada em < ~2 s, no ponto atual.
- **Continuidade:** simulação sobrevive à desconexão do cliente em 100% dos casos testados.
- **Qualidade:** cobertura de testes ≥ 80% (regra global do projeto).

## Risks and Mitigations

- **Risco de adoção — regressão de experiência:** se o novo modelo "picotar" ou perder alguma ação,
  o usuário percebe piora. *Mitigação:* checklist de paridade explícito e validação lado a lado com
  o comportamento atual antes do corte.
- **Dependência de servidor sempre online:** sem servidor não há simulação. *Mitigação:* estados de
  conexão claros na UI e comunicação explícita de indisponibilidade; escopo restrito a rede
  local/confiável nesta fase.
- **Sims órfãs consumindo recursos:** simulações vivas sem cliente podem acumular. *Mitigação:*
  política de limpeza/expiração (definida no TechSpec); baixo volume de sims no MVP torna o risco
  gerenciável.
- **Expectativa de multiusuário antecipada:** stakeholders podem esperar múltiplos usuários já no
  MVP. *Mitigação:* Non-Goals e Rollout deixam claro que é fase futura; MVP apenas não impede.

## Architecture Decision Records

- [ADR-001: Motor de simulação no servidor via WASM em Node, cliente como terminal remoto](adrs/adr-001.md)
  — Rodar o mesmo motor WASM em processo Node e transformar o cliente em terminal remoto, em vez de
  construir um servidor C++ nativo com Protobuf.

## Open Questions

- **Política de expiração de simulações órfãs:** por quanto tempo uma sim sem cliente conectado
  continua rodando antes de ser encerrada? (a definir no TechSpec)
- **Limite de simulações simultâneas** por instância no MVP (1? 2? sem limite explícito?).
- **Identidade de reconexão:** como o cliente reencontra "sua" simulação ao reabrir (identificador
  guardado localmente, lista de sims ativas, etc.)? (dimensão de UX + TechSpec)
- **Feedback de comando em trânsito:** qual o comportamento visual exato enquanto um comando ainda
  não foi refletido em um snapshot? (detalhe de UX)
</content>
