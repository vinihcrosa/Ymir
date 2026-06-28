import type { FastifyPluginAsync } from 'fastify'
import { Type } from '@sinclair/typebox'
import { vesselService } from '../services/vessel-service.js'
import { VesselDTO, CreateVesselDTO, VesselConfigDTO } from '@ymir/types'

const IdParams = Type.Object({ id: Type.String() })

export const vesselRoutes: FastifyPluginAsync = async (app) => {
  app.get('/vessels', {
    schema: { response: { 200: Type.Array(VesselDTO) } },
  }, async () => {
    return vesselService.findAll()
  })

  app.get<{ Params: { id: string } }>('/vessels/:id', {
    schema: {
      params: IdParams,
      response: {
        200: VesselDTO,
        404: Type.Object({ error: Type.String() }),
      },
    },
  }, async (req, reply) => {
    const id = Number(req.params.id)
    const vessel = vesselService.findById(id)
    if (!vessel) return reply.status(404).send({ error: 'Vessel not found' })
    return vessel
  })

  app.post<{ Body: CreateVesselDTO }>('/vessels', {
    schema: {
      body: CreateVesselDTO,
      response: { 201: VesselDTO },
    },
  }, async (req, reply) => {
    const vessel = vesselService.create(req.body)
    return reply.status(201).send(vessel)
  })

  app.delete<{ Params: { id: string } }>('/vessels/:id', {
    schema: { params: IdParams },
  }, async (req, reply) => {
    vesselService.remove(Number(req.params.id))
    return reply.status(204).send()
  })

  /** Full physics configuration — used by simulation engine */
  app.get<{ Params: { id: string } }>('/vessels/:id/config', {
    schema: {
      params: IdParams,
      response: {
        200: VesselConfigDTO,
        404: Type.Object({ error: Type.String() }),
      },
    },
  }, async (req, reply) => {
    const id = Number(req.params.id)
    const config = vesselService.findConfig(id)
    if (!config) {
      return reply.status(404).send({ error: 'Vessel config not found' })
    }
    return config
  })
}
