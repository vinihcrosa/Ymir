# Feature Spec: Vessel Data — Banco de Dados, Endpoints e Integração com Simulação

**ID**: FEAT-VESSEL-DATA  
**Status**: In Progress  
**Scope**: Large  

---

## Contexto

O arquivo `vessel1.json` contém os dados reais de um VLCC (Very Large Crude Carrier) estudado por engenheiros navais: `VLCC_L350_B63_T230` (L=350m, B=63m, T=23m). Esses dados incluem matrizes de massa, amortecimento, forças de correnteza, forças de onda, leme, propulsor e pontos de conexão.

O banco de dados atual (`vessels`) guarda apenas metadados mínimos (nome, L, B, T, massa). O simulador usa um ID fictício para criar embarcações sem parâmetros físicos reais.

---

## Requisitos

### R01 — Expansão do Banco de Dados

- O banco SQLite deve armazenar todos os dados físicos de `vessel1.json`.
- Tabelas suplementares para: propriedades físicas, correnteza, pontos de conexão, lemes, propulsores e forças de onda.
- Seed automático: ao iniciar a API com o banco vazio, importar `vessel1.json`.

### R02 — Endpoint de Configuração Completa

- `GET /vessels/:id/config` retorna a configuração física completa da embarcação.
- Resposta inclui: dimensões, matrizes (massa, amortecimento, restauração), coeficientes de correnteza, tabelas de leme e propulsor, forças de onda.

### R03 — Arquitetura em Camadas

- Rota → Serviço → Repositório → DB (sem saltar camadas).
- Refatorar `routes/vessels.ts` para usar `vessel-service.ts` e `vessel-repository.ts`.

### R04 — Integração com Simulação

- `simulation.worker.ts` busca `GET /vessels/1/config` ao inicializar.
- Passa `name`, `lpp`, `beam`, `draft` para o módulo WASM (mock ou real).
- WASM mock: ignora parâmetros físicos, usa dimensões para identificação.

---

## Fora de Escopo

- Integração completa dos parâmetros físicos no loop de integração WASM (depende dos bindings C++ Embind).
- Endpoint de importação por upload de arquivo.
- Múltiplos navios com dados completos (apenas vessel1.json como seed).

---

## Critérios de Aceitação

- `GET /vessels/1/config` retorna JSON com todos os campos de `vessel1.json`.
- API inicia sem erros com banco vazio (seed automático).
- Simulation worker loga o nome do navio ao inicializar.
- `tsx apps/api/src/index.ts` compila sem erros de TypeScript.
