import { Marker, Tooltip } from 'react-leaflet'
import L from 'leaflet'

interface VesselMarkerProps {
  vesselId: number
  name: string
  latLng: [number, number]
  headingDeg: number
  draggable: boolean
  selected?: boolean
  onDragEnd: (latLng: [number, number]) => void
  onClick?: (vesselId: number) => void
}

function createVesselIcon(headingDeg: number, selected: boolean): L.DivIcon {
  const fill = selected ? '#f97316' : '#1a73e8'
  const ring = selected ? `<circle cx="12" cy="12" r="14" fill="none" stroke="#f97316" stroke-width="2" opacity="0.7"/>` : ''
  // headingDeg is math angle (0=East, CCW). SVG arrow tip points North.
  // CSS rotate is CW from North, so: cssRotation = 90 - headingDeg.
  const cssRotation = (90 - headingDeg + 360) % 360
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="28" height="28" viewBox="0 0 28 28"
    style="transform:rotate(${cssRotation}deg);transform-origin:center">
    ${ring}
    <polygon points="14,3 22,25 14,21 6,25" fill="${fill}" stroke="white" stroke-width="1.2"/>
  </svg>`
  return L.divIcon({
    html: svg,
    className: '',
    iconSize: [28, 28],
    iconAnchor: [14, 14],
  })
}

export function VesselMarker({ vesselId, name, latLng, headingDeg, draggable, selected = false, onDragEnd, onClick }: VesselMarkerProps) {
  const icon = createVesselIcon(headingDeg, selected)
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
        },
        click: () => onClick?.(vesselId),
      }}
    >
      <Tooltip>{name}</Tooltip>
    </Marker>
  )
}
