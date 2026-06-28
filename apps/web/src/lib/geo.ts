const R = 6_371_000 // Earth radius [m]

export function latlngToMeters(
  lat: number,
  lng: number,
  origin: { latitude: number; longitude: number }
): { x: number; y: number } {
  const dy = lat - origin.latitude
  const dx = (lng - origin.longitude) * Math.cos((origin.latitude * Math.PI) / 180)
  return {
    x: dx * R * (Math.PI / 180),
    y: dy * R * (Math.PI / 180),
  }
}

export function metersToLatLng(
  x: number,
  y: number,
  origin: { latitude: number; longitude: number }
): [number, number] {
  const dy = y / (R * (Math.PI / 180))
  const dx = x / (R * (Math.PI / 180))
  const lat = origin.latitude + dy
  const lng = origin.longitude + dx / Math.cos((origin.latitude * Math.PI) / 180)
  return [lat, lng]
}

export function isInsidePolygon(
  x: number,
  y: number,
  polygonMeters: Array<{ x: number; y: number }>
): boolean {
  let inside = false
  const n = polygonMeters.length
  for (let i = 0, j = n - 1; i < n; j = i++) {
    const xi = polygonMeters[i].x
    const yi = polygonMeters[i].y
    const xj = polygonMeters[j].x
    const yj = polygonMeters[j].y
    const intersect =
      yi > y !== yj > y && x < ((xj - xi) * (y - yi)) / (yj - yi) + xi
    if (intersect) inside = !inside
  }
  return inside
}
