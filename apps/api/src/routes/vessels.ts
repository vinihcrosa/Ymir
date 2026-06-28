import { FastifyPluginAsync } from 'fastify'
import { Type } from '@sinclair/typebox'
import { db } from '../db/index.js'
import { vessels } from '../db/schema.js'
import { eq } from 'drizzle-orm'
import { VesselDTO, CreateVesselDTO } from '@ymir/types'

export const vesselRoutes: FastifyPluginAsync = async (app) => {
  app.get('/vessels', {
    schema: { response: { 200: Type.Array(VesselDTO) } },
  }, async () => {
    return db.select().from(vessels).all().map(row => ({
      ...row,
      createdAt: row.createdAt,
    }))
  })

  app.get<{ Params: { id: string } }>('/vessels/:id', {
    schema: { response: { 200: VesselDTO } },
  }, async (req, reply) => {
    const id = Number(req.params.id)
    const row = db.select().from(vessels).where(eq(vessels.id, id)).get()
    if (!row) return reply.status(404).send({ error: 'Vessel not found' })
    return row
  })

  app.post<{ Body: CreateVesselDTO }>('/vessels', {
    schema: { body: CreateVesselDTO, response: { 201: VesselDTO } },
  }, async (req, reply) => {
    const now = new Date().toISOString()
    const result = db.insert(vessels).values({ ...req.body, createdAt: now }).returning().get()
    return reply.status(201).send(result)
  })

  app.delete<{ Params: { id: string } }>('/vessels/:id', async (req, reply) => {
    const id = Number(req.params.id)
    db.delete(vessels).where(eq(vessels.id, id)).run()
    return reply.status(204).send()
  })
}
