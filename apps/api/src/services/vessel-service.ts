import { vesselRepository } from '../repositories/vessel-repository.js'
import type { VesselDTO, CreateVesselDTO, VesselConfigDTO } from '@ymir/types'

function rowToDTO(row: ReturnType<typeof vesselRepository.findById> & {}): VesselDTO {
  return {
    id: row.id,
    name: row.name,
    length: row.length,
    beam: row.beam,
    draft: row.draft,
    mass: row.mass,
    createdAt: row.createdAt,
  }
}

export const vesselService = {
  findAll(): VesselDTO[] {
    return vesselRepository.findAll().map(rowToDTO)
  },

  findById(id: number): VesselDTO | null {
    const row = vesselRepository.findById(id)
    return row ? rowToDTO(row) : null
  },

  create(data: CreateVesselDTO): VesselDTO {
    const now = new Date().toISOString()
    const row = vesselRepository.create({ ...data, createdAt: now })
    return rowToDTO(row)
  },

  remove(id: number): void {
    vesselRepository.remove(id)
  },

  findConfig(id: number): VesselConfigDTO | null {
    return vesselRepository.findConfig(id) ?? null
  },
}
