import { MapContainer, TileLayer, Polygon, useMap } from 'react-leaflet'
import L from 'leaflet'
import { useEffect } from 'react'
import { useScenarioStore } from '../store'
import { useSimulationStore } from '../../../stores/simulationStore'
import { VesselMarker } from './VesselMarker'
import { latlngToMeters, metersToLatLng } from '../../../lib/geo'

// Inner component that adjusts map bounds when area changes
function AreaBoundsFitter({ coordinates }: { coordinates: [number, number][][] }) {
  const map = useMap()
  useEffect(() => {
    if (coordinates.length > 0 && coordinates[0].length > 0) {
      const leafletCoords = coordinates[0].map(([lng, lat]) => [lat, lng] as [number, number])
      const bounds = L.latLngBounds(leafletCoords)
      map.fitBounds(bounds, { padding: [40, 40] })
    }
  }, [map, coordinates])
  return null
}

export function AreaMapView() {
  const { area, vessels, updateVesselPosition } = useScenarioStore()
  const { status, state } = useSimulationStore()
  const isRunning = status === 'running'

  // Default center: Rio de Janeiro
  const defaultCenter: [number, number] = [-22.9, -43.2]
  const defaultZoom = 12

  // GeoJSON polygon coordinates are [lng, lat] — convert to Leaflet [lat, lng]
  const polygonPositions: [number, number][][] = area
    ? area.polygon.coordinates.map(ring =>
        ring.map(([lng, lat]) => [lat, lng] as [number, number])
      )
    : []

  return (
    <MapContainer
      center={area ? [area.origin.latitude, area.origin.longitude] : defaultCenter}
      zoom={defaultZoom}
      style={{ height: '100%', width: '100%' }}
    >
      <TileLayer
        attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>'
        url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
      />
      {area && (
        <>
          <AreaBoundsFitter coordinates={area.polygon.coordinates as [number, number][][]} />
          <Polygon
            positions={polygonPositions[0] ?? []}
            pathOptions={{ color: '#f97316', fillOpacity: 0.05, weight: 2 }}
          />
        </>
      )}
      {vessels.map(vessel => {
        // During simulation, use live state; otherwise use draft position
        let latLng: [number, number]
        if (isRunning && state && area) {
          const liveVessel = state.vessels.find(v => v.id === vessel.vesselId)
          if (liveVessel) {
            latLng = metersToLatLng(liveVessel.x, liveVessel.y, area.origin)
          } else {
            latLng = area
              ? metersToLatLng(vessel.x, vessel.y, area.origin)
              : defaultCenter
          }
        } else {
          latLng = area
            ? metersToLatLng(vessel.x, vessel.y, area.origin)
            : defaultCenter
        }

        const headingDeg = isRunning && state
          ? ((state.vessels.find(v => v.id === vessel.vesselId)?.psi ?? 0) * 180 / Math.PI)
          : vessel.headingDeg

        return (
          <VesselMarker
            key={vessel.vesselId}
            vesselId={vessel.vesselId}
            name={vessel.name}
            latLng={latLng}
            headingDeg={headingDeg}
            draggable={!isRunning}
            onDragEnd={([lat, lng]) => {
              if (area) {
                const { x, y } = latlngToMeters(lat, lng, area.origin)
                updateVesselPosition(vessel.vesselId, x, y)
              }
            }}
          />
        )
      })}
    </MapContainer>
  )
}
