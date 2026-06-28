import type { FastifyPluginAsync } from 'fastify'
import { Type } from '@sinclair/typebox'
import { areaService } from '../services/area-service.js'
import { AreaDTO, CreateAreaDTO } from '@ymir/types'

const IdParams = Type.Object({ id: Type.String() })

export const areaRoutes: FastifyPluginAsync = async (app) => {
  app.get('/areas', {
    schema: { response: { 200: Type.Array(AreaDTO) } },
  }, async () => {
    return areaService.findAll()
  })

  app.get<{ Params: { id: string } }>('/areas/:id', {
    schema: {
      params: IdParams,
      response: {
        200: AreaDTO,
        404: Type.Object({ error: Type.String() }),
      },
    },
  }, async (req, reply) => {
    const area = areaService.findById(Number(req.params.id))
    if (!area) return reply.status(404).send({ error: 'Area not found' })
    return area
  })

  app.post<{ Body: CreateAreaDTO }>('/areas', {
    schema: {
      body: CreateAreaDTO,
      response: { 201: AreaDTO },
    },
  }, async (req, reply) => {
    const area = areaService.create(req.body)
    return reply.status(201).send(area)
  })

  app.delete<{ Params: { id: string } }>('/areas/:id', {
    schema: { params: IdParams },
  }, async (req, reply) => {
    areaService.remove(Number(req.params.id))
    return reply.status(204).send()
  })
}
