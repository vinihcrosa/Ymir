# Contexto: Infraestrutura

Suporte transversal ao sistema — comunicação externa, persistência e observabilidade.

---

## Server (apps/server)

Ponto de entrada único para clientes externos em modo real-time.

**Responsabilidades:**
- Aceitar conexões WebSocket de Manager e Viewer
- Receber e rotear comandos para a Simulation/World corretos
- Propagar eventos da simulação para clientes conectados (via Events)
- Gerenciar múltiplas simulações simultâneas (1–2 em real-time)
- Autenticar e identificar clientes (a definir)

**API (operações expostas):**
- Criar simulação (a partir de scenario)
- Encerrar simulação
- Pausar / retomar
- Alterar velocidade
- Criar / remover entidades
- Enviar comandos a embarcações (propulsor, leme)
- Alterar condições ambientais
- Alterar terreno
- Consultar estado atual
- Subscribir eventos

**Protocolo:** WebSocket + Protobuf (schemas add-only)

---

## Fast-time (apps/fast-time)

App separado para execução batch de simulações aceleradas.

**Responsabilidades:**
- Aceitar submissão de scenarios via API
- Executar simulações no modo fast-time (sem sincronização de relógio)
- Isolar cada simulação em processo separado
- Persistir resultados completos ao final de cada run
- Sem streaming de eventos para clientes

**Fluxo:**
```
Cliente envia scenario → fast-time aceita → inicia processo isolado →
simulação roda até o fim → resultados salvos no SQLite → cliente consulta resultados
```

---

## Persistence (libs/persistence)

Camada de acesso ao SQLite. Usada por server e fast-time.

**Responsabilidades:**
- Salvar e carregar scenarios
- Salvar e carregar configurações de entidades (vessels, buoys, etc.)
- Registrar histórico de simulações (cada step — escrita em lote)
- Registrar logs e dados de performance do servidor

**Notas:**
- Escrita em lote para histórico de steps (evita bottleneck de I/O em simulações longas)
- Leitura por range de tempo (ex: "steps entre t=100s e t=200s")

---

## Telemetry (libs/persistence)

Observabilidade interna do servidor — não ligada a simulações específicas.

**Responsabilidades:**
- Registrar métricas de performance (tempo de tick, uso de CPU/memória por simulação)
- Registrar logs de erros e warnings internos
- Permitir diagnóstico de bugs e análise de performance

**Notas:**
- Dados separados do histórico de simulação no SQLite
- Não é exposto diretamente para Manager/Viewer — uso interno e de operações
