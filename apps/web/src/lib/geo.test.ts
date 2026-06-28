import { describe, it, expect } from 'vitest'
import { latlngToMeters, metersToLatLng, isInsidePolygon } from './geo'

const origin = { latitude: -23.9, longitude: -46.3 }

describe('latlngToMeters', () => {
  it('maps origin to { x: 0, y: 0 }', () => {
    const result = latlngToMeters(origin.latitude, origin.longitude, origin)
    expect(result.x).toBeCloseTo(0, 5)
    expect(result.y).toBeCloseTo(0, 5)
  })

  it('north of origin has positive y', () => {
    const result = latlngToMeters(origin.latitude + 0.01, origin.longitude, origin)
    expect(result.y).toBeGreaterThan(0)
  })

  it('east of origin has positive x', () => {
    const result = latlngToMeters(origin.latitude, origin.longitude + 0.01, origin)
    expect(result.x).toBeGreaterThan(0)
  })
})

describe('metersToLatLng', () => {
  it('round-trip within 0.01 m', () => {
    const lat = -23.85
    const lng = -46.25
    const { x, y } = latlngToMeters(lat, lng, origin)
    const [lat2, lng2] = metersToLatLng(x, y, origin)
    const R = 6_371_000
    const dyM = Math.abs(lat2 - lat) * R * (Math.PI / 180)
    const dxM =
      Math.abs(lng2 - lng) *
      Math.cos((origin.latitude * Math.PI) / 180) *
      R *
      (Math.PI / 180)
    expect(dyM).toBeLessThan(0.01)
    expect(dxM).toBeLessThan(0.01)
  })
})

describe('isInsidePolygon', () => {
  const square = [
    { x: -100, y: -100 },
    { x: 100, y: -100 },
    { x: 100, y: 100 },
    { x: -100, y: 100 },
  ]

  it('returns true for point inside square', () => {
    expect(isInsidePolygon(0, 0, square)).toBe(true)
  })

  it('returns false for point outside square', () => {
    expect(isInsidePolygon(200, 200, square)).toBe(false)
  })
})
