import { areaRepository } from '../repositories/area-repository.js'
import type { AreaDTO, CreateAreaDTO } from '@ymir/types'

export const areaService = {
  findAll(): AreaDTO[] {
    return areaRepository.findAll()
  },

  findById(id: number): AreaDTO | null {
    return areaRepository.findById(id) ?? null
  },

  create(data: CreateAreaDTO): AreaDTO {
    return areaRepository.create({
      name: data.name,
      description: data.description ?? null,
      originLat: data.origin.latitude,
      originLng: data.origin.longitude,
      polygon: JSON.stringify(data.polygon),
      gravity: data.gravity,
      magneticCorrection: data.magneticCorrection,
      waterDensity: data.waterDensity,
      airDensity: data.airDensity,
      createdAt: new Date().toISOString(),
    })
  },

  remove(id: number): void {
    areaRepository.remove(id)
  },
}
