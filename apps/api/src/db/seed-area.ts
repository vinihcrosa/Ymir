import { readFileSync } from 'node:fs'
import { resolve, dirname } from 'node:path'
import { fileURLToPath } from 'node:url'
import { db } from './index.js'
import { areas } from './schema.js'

const __dirname = dirname(fileURLToPath(import.meta.url))

const AREA_JSON_PATH =
  process.env.AREA_JSON_PATH ?? resolve(__dirname, '../../../../../area.json')

interface AreaJson {
  name: string
  description?: string
  coverage: {
    origin: { latitude: number; longitude: number }
    type: 'Polygon'
    coordinates: number[][][]
  }
  gravity: number
  magneticCorrection: number
  waterDensity: number
  airDensity: number
}

export function seedArea(): void {
  let raw: AreaJson
  try {
    raw = JSON.parse(readFileSync(AREA_JSON_PATH, 'utf-8')) as AreaJson
  } catch (err) {
    console.warn(`[seed] area.json not found at ${AREA_JSON_PATH}: ${String(err)}`)
    return
  }

  const existing = db.select().from(areas).all()
  if (existing.some(a => a.name === raw.name)) return

  const polygon = JSON.stringify({
    type: raw.coverage.type,
    coordinates: raw.coverage.coordinates,
  })

  const [area] = db
    .insert(areas)
    .values({
      name: raw.name,
      description: raw.description ?? null,
      originLat: raw.coverage.origin.latitude,
      originLng: raw.coverage.origin.longitude,
      polygon,
      gravity: raw.gravity,
      magneticCorrection: raw.magneticCorrection,
      waterDensity: raw.waterDensity,
      airDensity: raw.airDensity,
      createdAt: new Date().toISOString(),
    })
    .returning()
    .all()

  console.info(`[seed] Imported area.json → area id=${area.id} (${raw.name})`)
}
