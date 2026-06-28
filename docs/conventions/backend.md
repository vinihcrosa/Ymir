# Backend Conventions — apps/api

## Stack

- **Runtime**: Node.js 20 + TypeScript (ESM)
- **Framework**: Fastify v4
- **Validation**: TypeBox + `@fastify/type-provider-typebox`
- **ORM**: Drizzle + better-sqlite3
- **Schemas compartilhados**: `@ymir/types`

---

## Arquitetura em Camadas

```
Route → Service → Repository → DB
```

Cada camada tem responsabilidade única. Saltar camadas é proibido.

| Camada | Responsabilidade | Conhece |
|--------|-----------------|---------|
| **Route** | Validação HTTP, serialização de response | `Request`, `Reply`, Service |
| **Service** | Regras de negócio, orquestração | Repository, tipos de domínio |
| **Repository** | Queries Drizzle, mapeamento de schema | `db`, schema Drizzle |
| **Plugin** | Registra rotas + dependências de um domínio | Fastify instance |

Routes **nunca** acessam repositórios diretamente.
Services **nunca** conhecem `Request` ou `Reply`.

---

## Estrutura de Pastas

```
src/
├── plugins/          # Fastify plugins de infraestrutura
│   ├── db.ts         # Decora fastify com `db`
│   └── error-handler.ts
├── routes/           # HTTP handlers — um arquivo por domínio
│   ├── vessels.ts
│   └── scenarios.ts
├── services/         # Regras de negócio — um arquivo por domínio
│   ├── vessel-service.ts
│   └── scenario-service.ts
├── repositories/     # Drizzle queries — um arquivo por domínio
│   ├── vessel-repository.ts
│   └── scenario-repository.ts
├── db/
│   ├── schema.ts     # Tabelas Drizzle
│   └── index.ts      # Conexão + migrations
├── config.ts         # Env vars validadas
└── index.ts          # Entry point
```

---

## Validação de Requests

**Toda route deve declarar schema** para body, params e querystring quando presentes.
Fastify rejeita requests inválidos automaticamente (HTTP 400) — sem validação manual no handler.

Usar o TypeBox type provider para type safety no handler:

```typescript
// index.ts — registrar o provider uma vez
import { TypeBoxTypeProvider } from '@fastify/type-provider-typebox'

const app = Fastify({ logger: true }).withTypeProvider<TypeBoxTypeProvider>()
```

```typescript
// routes/vessels.ts
import { Type } from '@sinclair/typebox'
import { VesselDTO, CreateVesselDTO } from '@ymir/types'
import { FastifyPluginAsyncTypebox } from '@fastify/type-provider-typebox'

export const vesselPlugin: FastifyPluginAsyncTypebox = async (app) => {
  app.get('/vessels', {
    schema: {
      response: { 200: Type.Array(VesselDTO) },
    },
  }, async () => {
    return vesselService.findAll()
  })

  app.post('/vessels', {
    schema: {
      body: CreateVesselDTO,
      response: { 201: VesselDTO },
    },
  }, async (req, reply) => {
    const vessel = await vesselService.create(req.body) // req.body já tipado
    return reply.status(201).send(vessel)
  })

  app.get('/vessels/:id', {
    schema: {
      params: Type.Object({ id: Type.Number() }),
      response: {
        200: VesselDTO,
        404: Type.Object({ error: Type.String() }),
      },
    },
  }, async (req, reply) => {
    const vessel = await vesselService.findById(req.params.id)
    if (!vessel) return reply.status(404).send({ error: 'Vessel not found' })
    return vessel
  })
}
```

Schemas de domínio vivem em `@ymir/types`.
Schemas locais (params, querystring simples) podem ser definidos inline na route.

---

## Injeção de Dependências

Via Fastify decorators — sem DI container externo.

```typescript
// plugins/db.ts
import fp from 'fastify-plugin'

export const dbPlugin = fp(async (app) => {
  app.decorate('db', db)
})

// Anywhere
app.db.select()...
```

Services e repositories são instâncias singleton importadas diretamente (sem decorators) quando não precisam de ciclo de vida:

```typescript
// routes/vessels.ts
import { vesselService } from '../services/vessel-service.js'
```

---

## Tratamento de Erros

Plugin global de error handler registrado no entry point.
Services lançam erros tipados — nunca objetos plain:

```typescript
// lib/errors.ts
export class NotFoundError extends Error {
  readonly statusCode = 404
  constructor(resource: string, id: number | string) {
    super(`${resource} ${id} not found`)
  }
}

export class ValidationError extends Error {
  readonly statusCode = 400
}
```

O error handler mapeia `statusCode` para HTTP response.
Routes **não** fazem try/catch — o error handler é responsável.

---

## Configuração de Ambiente

Todas as env vars são validadas no startup via TypeBox:

```typescript
// config.ts
import { Type, Static } from '@sinclair/typebox'
import { Value } from '@sinclair/typebox/value'

const ConfigSchema = Type.Object({
  PORT:    Type.Number({ default: 3000 }),
  HOST:    Type.String({ default: '0.0.0.0' }),
  DB_PATH: Type.String({ default: './ymir.db' }),
  NODE_ENV: Type.Union([
    Type.Literal('development'),
    Type.Literal('production'),
    Type.Literal('test'),
  ], { default: 'development' }),
})

export type Config = Static<typeof ConfigSchema>

export const config: Config = Value.Parse(ConfigSchema, {
  PORT:     Number(process.env.PORT),
  HOST:     process.env.HOST,
  DB_PATH:  process.env.DB_PATH,
  NODE_ENV: process.env.NODE_ENV,
})
```

Startup aborta se validação falhar — nunca iniciar com config inválida.

---

## Naming

| Coisa | Padrão | Exemplo |
|-------|--------|---------|
| Arquivos | `kebab-case` | `vessel-service.ts` |
| Classes | `PascalCase` | `VesselRepository` |
| Funções/variáveis | `camelCase` | `findById` |
| Plugins Fastify | `[domain]Plugin` | `vesselPlugin` |
| Tipos/Interfaces | `PascalCase` | `VesselRow` |
| Constantes globais | `SCREAMING_SNAKE` | `MAX_PAGE_SIZE` |

---

## Regras Gerais

- Arquivo máximo: **~200 linhas** — dividir se passar
- Uma exportação nomeada principal por arquivo
- `import type` obrigatório para importações tipo-only
- Sem `any` — exceção documentada com comentário explicando por quê
- Funções exportadas têm tipo de retorno explícito
- Sem `console.log` — usar `app.log` (Fastify logger / Pino)
- Ordenação de imports: `node built-ins → externos → @ymir/* → relativos`
