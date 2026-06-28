import { eq } from 'drizzle-orm'
import { db } from '../db/index.js'
import { areas } from '../db/schema.js'
import type { AreaDTO } from '@ymir/types'

export type AreaRow = typeof areas.$inferSelect
export type CreateAreaRow = typeof areas.$inferInsert

function rowToDTO(row: AreaRow): AreaDTO {
  const polygon = JSON.parse(row.polygon)
  return {
    id: row.id,
    name: row.name,
    description: row.description ?? undefined,
    origin: { latitude: row.originLat, longitude: row.originLng },
    polygon,
    gravity: row.gravity,
    magneticCorrection: row.magneticCorrection,
    waterDensity: row.waterDensity,
    airDensity: row.airDensity,
    createdAt: row.createdAt,
  }
}

export const areaRepository = {
  findAll(): AreaDTO[] {
    return db.select().from(areas).all().map(rowToDTO)
  },

  findById(id: number): AreaDTO | undefined {
    const row = db.select().from(areas).where(eq(areas.id, id)).get()
    return row ? rowToDTO(row) : undefined
  },

  create(data: Omit<CreateAreaRow, 'id'>): AreaDTO {
    const row = db.insert(areas).values(data).returning().get()
    return rowToDTO(row)
  },

  remove(id: number): void {
    db.delete(areas).where(eq(areas.id, id)).run()
  },
}
