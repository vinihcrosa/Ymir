import { FastifyPluginAsync } from 'fastify'
import { Type } from '@sinclair/typebox'
import { db } from '../db/index.js'
import { scenarios } from '../db/schema.js'
import { eq } from 'drizzle-orm'
import { ScenarioDTO, CreateScenarioDTO } from '@ymir/types'

function rowToScenario(row: typeof scenarios.$inferSelect): ScenarioDTO {
  // Legacy rows persisted before InitialCondition gained `instanceId` stored the
  // body id under `vesselId`. Backfill it so the response satisfies the schema.
  const initialConditions = (JSON.parse(row.initialConditions) as Array<Record<string, number>>)
    .map((ic) => ({ instanceId: ic.instanceId ?? ic.vesselId, ...ic }))
  return {
    ...row,
    initialConditions,
    description: row.description ?? undefined,
    areaId: row.areaId ?? undefined,
    createdAt: row.createdAt,
  }
}

export const scenarioRoutes: FastifyPluginAsync = async (app) => {
  app.get('/scenarios', {
    schema: { response: { 200: Type.Array(ScenarioDTO) } },
  }, async () => {
    return db.select().from(scenarios).all().map(rowToScenario)
  })

  app.get<{ Params: { id: string } }>('/scenarios/:id', {
    schema: { response: { 200: ScenarioDTO } },
  }, async (req, reply) => {
    const id = Number(req.params.id)
    const row = db.select().from(scenarios).where(eq(scenarios.id, id)).get()
    if (!row) return reply.status(404).send({ error: 'Scenario not found' })
    return rowToScenario(row)
  })

  app.post<{ Body: CreateScenarioDTO }>('/scenarios', {
    schema: { body: CreateScenarioDTO, response: { 201: ScenarioDTO } },
  }, async (req, reply) => {
    const now = new Date().toISOString()
    const result = db.insert(scenarios).values({
      ...req.body,
      initialConditions: JSON.stringify(req.body.initialConditions),
      createdAt: now,
    }).returning().get()
    return reply.status(201).send(rowToScenario(result))
  })

  app.delete<{ Params: { id: string } }>('/scenarios/:id', async (req, reply) => {
    const id = Number(req.params.id)
    db.delete(scenarios).where(eq(scenarios.id, id)).run()
    return reply.status(204).send()
  })
}
