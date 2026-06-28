# Scenario Creator — Especificação

**ID**: FEAT-SC  
**Escopo**: Large  
**Status**: Draft  

---

## Declaração do Problema

Hoje o frontend do Ymir é apenas um painel de telemetria sem fluxo de criação de simulação. O instrutor não tem como selecionar área geográfica, adicionar embarcações, posicioná-las visualmente no mapa e controlar a simulação — toda essa orquestração é inexistente. O resultado é que o produto não pode ser usado para treinar nenhum cenário real.

---

## Objetivos

- [ ] Instrutor consegue criar e iniciar uma simulação completa sem sair do browser
- [ ] Posicionamento de embarcações acontece sobre o mapa real da área
- [ ] Simulação pode ser iniciada e parada a qualquer momento

---

## Fora de Escopo

| Feature | Motivo |
|---------|--------|
| Múltiplas áreas simultâneas | Uma simulação = uma área |
| Autenticação / perfis de usuário | Adiado para fase posterior |
| Replay de simulação gravada | Fora do MVP |
| Edição de parâmetros físicos do navio na UI | Dados vêm do banco; edição é operação administrativa |
| Visualização 3D | Three.js/WebGPU está adiado (ver STATE.md) |
| Criação de área pelo instrutor | Áreas são pré-cadastradas pelo admin |
| Ancoragem e linhas de reboque | Fora do escopo atual do motor de física |
| Salvar/carregar cenário salvo anteriormente | Fluxo de gestão de cenários é P3 |

---

## User Stories

### SC-P1-01 — Selecionar área e visualizar mapa ⭐ MVP

**User Story**: Como instrutor, quero selecionar a área geográfica da simulação e visualizar seu mapa com o polígono de limite, para saber onde posso posicionar as embarcações.

**Por que P1**: Sem área não há contexto geográfico; todo o resto depende desse passo.

**Acceptance Criteria**:

1. WHEN a tela carrega THEN sistema SHALL exibir dropdown/lista com todas as áreas disponíveis na API (`GET /areas`)
2. WHEN instrutor seleciona uma área THEN sistema SHALL exibir mapa centrado na origem da área (lat/lng = 0,0 da simulação)
3. WHEN mapa é exibido THEN sistema SHALL desenhar o polígono de limite da área em destaque
4. WHEN área tem polígono definido THEN mapa SHALL ajustar zoom para caber o polígono inteiro
5. WHEN instrutor tenta posicionar vessel fora do polígono THEN sistema SHALL impedir e exibir mensagem de erro

**Independent Test**: Selecionar "baia_de_guanabara" → mapa exibe polígono da Baía de Guanabara centralizado na origem.

---

### SC-P1-02 — Adicionar e posicionar embarcações ⭐ MVP

**User Story**: Como instrutor, quero selecionar embarcações do banco, adicioná-las ao cenário e posicioná-las arrastando marcadores no mapa, para definir a configuração inicial da simulação.

**Por que P1**: Núcleo da criação de cenário.

**Acceptance Criteria**:

1. WHEN área está selecionada THEN sistema SHALL exibir painel para adicionar embarcações
2. WHEN instrutor clica "Adicionar vessel" THEN sistema SHALL exibir seletor com vessels disponíveis (`GET /vessels`)
3. WHEN instrutor seleciona um vessel THEN sistema SHALL inserir marcador no centro do mapa (posição 0,0)
4. WHEN instrutor arrasta marcador no mapa THEN sistema SHALL atualizar posição (x, y em metros) em tempo real
5. WHEN instrutor adiciona mais de um vessel THEN sistema SHALL permitir N vessels, cada um com marcador independente
6. WHEN instrutor clica no marcador THEN sistema SHALL exibir nome da embarcação e posição atual (x, y, ψ)
7. WHEN instrutor remove vessel da lista THEN sistema SHALL remover marcador do mapa

**Independent Test**: Adicionar dois vessels → arrastar cada um → painel mostra coordenadas distintas em metros.

---

### SC-P1-03 — Definir heading inicial do vessel ⭐ MVP

**User Story**: Como instrutor, quero definir o heading inicial (ψ) de cada embarcação, para configurar a orientação de partida.

**Por que P1**: ψ é uma das condições iniciais do integrador; posição sem heading é configuração incompleta.

**Acceptance Criteria**:

1. WHEN instrutor seleciona marcador de vessel THEN sistema SHALL exibir campo de heading (graus, 0–360)
2. WHEN instrutor altera heading THEN marcador no mapa SHALL girar para refletir a orientação
3. WHEN valor de heading é inválido (fora de 0–360) THEN sistema SHALL normalizar para [0, 360)

**Independent Test**: Definir heading = 90° para vessel → marcador aponta para Leste.

---

### SC-P1-04 — Iniciar e parar simulação ⭐ MVP

**User Story**: Como instrutor, quero iniciar a simulação com as embarcações configuradas e parar a qualquer momento, para conduzir o treinamento.

**Por que P1**: O produto não tem valor sem controle de simulação.

**Acceptance Criteria**:

1. WHEN instrutor clica "Iniciar" com área selecionada e ao menos uma embarcação THEN sistema SHALL salvar cenário na API (`POST /scenarios`) e iniciar o Web Worker
2. WHEN simulação está rodando THEN sistema SHALL exibir posição das embarcações atualizada em tempo real no mapa (marcadores se movem)
3. WHEN instrutor clica "Parar" THEN simulação SHALL pausar e marcadores SHALL congelar na última posição
4. WHEN simulação está rodando THEN botão "Iniciar" SHALL ficar desabilitado
5. WHEN simulação está parada THEN botão "Parar" SHALL ficar desabilitado
6. WHEN sistema não tem área ou vessels configurados THEN botão "Iniciar" SHALL ficar desabilitado com tooltip explicativo

**Independent Test**: Iniciar → marcadores se movem no mapa → Parar → marcadores param → Iniciar novamente → continuam.

---

### SC-P2-01 — Nomear cenário

**User Story**: Como instrutor, quero dar um nome ao cenário antes de iniciar, para identificá-lo no histórico.

**Por que P2**: Util mas não bloqueia o MVP; pode usar nome gerado automaticamente.

**Acceptance Criteria**:

1. WHEN campo de nome está vazio THEN sistema SHALL usar nome gerado: `"Cenário {data}"` 
2. WHEN instrutor digita nome THEN sistema SHALL usar o nome fornecido no `POST /scenarios`

---

### SC-P2-02 — Telemetria em overlay no mapa

**User Story**: Como instrutor, quero ver os valores de x, y, ψ, u, v, r de cada embarcação diretamente no mapa durante a simulação.

**Por que P2**: Complementa a visualização sem ser bloqueante para o MVP.

**Acceptance Criteria**:

1. WHEN simulação está rodando THEN cada marcador SHALL exibir tooltip com estado atual (x, y, ψ)
2. WHEN vessel sai dos limites do viewport do mapa THEN viewport SHALL seguir o centróide das embarcações

---

### SC-P3-01 — Salvar e reutilizar cenário

**User Story**: Como instrutor, quero salvar um cenário criado e reutilizá-lo depois, sem precisar reconfigurar.

**Por que P3**: Não bloqueia o treinamento; instructor pode reconfigurar manualmente.

**Acceptance Criteria**:

1. WHEN simulação é iniciada THEN cenário é persistido na API (`POST /scenarios`)
2. WHEN instrutor abre lista de cenários THEN sistema SHALL exibir cenários salvos com área e vessels

---

## Edge Cases

- WHEN API `/areas` retorna lista vazia THEN sistema SHALL exibir mensagem "Nenhuma área disponível" e desabilitar criação
- WHEN API `/vessels` retorna lista vazia THEN sistema SHALL exibir mensagem "Nenhuma embarcação cadastrada"
- WHEN API está indisponível THEN sistema SHALL exibir erro com botão de retry
- WHEN dois vessels são posicionados na mesma coordenada THEN sistema SHALL permitir (colisão é responsabilidade do motor de física)
- WHEN instrutor recarrega página durante simulação ativa THEN simulação SHALL parar (estado não é persistido além do reload)
- WHEN polígono da área não está disponível THEN mapa SHALL exibir área sem limite visual

---

## Rastreabilidade de Requisitos

| ID | Story | Fase | Status |
|----|-------|------|--------|
| SC-P1-01 | Selecionar área + mapa | Design | Pending |
| SC-P1-02 | Adicionar e posicionar vessels | Design | Pending |
| SC-P1-03 | Heading inicial | Design | Pending |
| SC-P1-04 | Iniciar e parar simulação | Design | Pending |
| SC-P2-01 | Nomear cenário | Design | Pending |
| SC-P2-02 | Telemetria overlay | Design | Pending |
| SC-P3-01 | Salvar cenário | - | Pending |

**Coverage**: 7 requisitos, 0 mapeados para tasks, 7 pendentes

---

## Critérios de Sucesso

- [ ] Instrutor consegue configurar e iniciar simulação em menos de 2 minutos
- [ ] Marcadores se movem no mapa durante simulação sem lag perceptível (20 Hz worker → 20 fps visual)
- [ ] Zero erros de TypeScript no build
- [ ] Funciona em Chrome e Firefox modernos
