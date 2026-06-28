import { Marker, Tooltip } from 'react-leaflet'
import L from 'leaflet'

interface VesselMarkerProps {
  vesselId: number
  name: string
  latLng: [number, number]
  headingDeg: number
  draggable: boolean
  onDragEnd: (latLng: [number, number]) => void
}

// Create a divIcon with an SVG ship shape rotated by heading
function createVesselIcon(headingDeg: number): L.DivIcon {
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"
    style="transform:rotate(${headingDeg}deg);transform-origin:center">
    <polygon points="12,2 20,22 12,18 4,22" fill="#1a73e8" stroke="white" stroke-width="1"/>
  </svg>`
  return L.divIcon({
    html: svg,
    className: '',
    iconSize: [24, 24],
    iconAnchor: [12, 12],
  })
}

export function VesselMarker({ vesselId, name, latLng, headingDeg, draggable, onDragEnd }: VesselMarkerProps) {
  const icon = createVesselIcon(headingDeg)
  return (
    <Marker
      key={vesselId}
      position={latLng}
      icon={icon}
      draggable={draggable}
      eventHandlers={{
        dragend: (e) => {
          const { lat, lng } = (e.target as L.Marker).getLatLng()
          onDragEnd([lat, lng])
        }
      }}
    >
      <Tooltip>{name}</Tooltip>
    </Marker>
  )
}
