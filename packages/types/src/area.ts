import { Type, Static } from '@sinclair/typebox'

/** Geographic origin — maps to simulation coordinate (0, 0). */
const OriginDTO = Type.Object({
  latitude: Type.Number({ description: 'Latitude [deg]' }),
  longitude: Type.Number({ description: 'Longitude [deg]' }),
})

/**
 * Geographic polygon bounding the simulation domain.
 * Stored as GeoJSON ring: [[lng, lat], ...] (first = last vertex).
 * Simulation coordinates are in meters relative to origin.
 */
const PolygonDTO = Type.Object({
  type: Type.Literal('Polygon'),
  /** Outer ring only; GeoJSON [lng, lat] pairs */
  coordinates: Type.Array(Type.Array(Type.Tuple([Type.Number(), Type.Number()]))),
})

export const AreaDTO = Type.Object({
  id: Type.Number(),
  name: Type.String(),
  description: Type.Optional(Type.String()),
  origin: OriginDTO,
  polygon: PolygonDTO,
  gravity: Type.Number({ description: 'Local gravitational acceleration [m/s²]', default: 9.81 }),
  magneticCorrection: Type.Number({ description: 'Magnetic declination [deg]' }),
  waterDensity: Type.Number({ description: 'Seawater density [t/m³]', default: 1.025 }),
  airDensity: Type.Number({ description: 'Air density [t/m³]', default: 0.001275 }),
  createdAt: Type.String({ format: 'date-time' }),
})
export type AreaDTO = Static<typeof AreaDTO>

export const CreateAreaDTO = Type.Omit(AreaDTO, ['id', 'createdAt'])
export type CreateAreaDTO = Static<typeof CreateAreaDTO>
