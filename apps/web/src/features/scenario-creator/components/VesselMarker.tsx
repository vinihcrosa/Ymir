import { useRef, useMemo } from 'react'
import { Marker, Tooltip } from 'react-leaflet'
import L from 'leaflet'

interface VesselMarkerProps {
  vesselId: number      // instanceId — unique physics identifier
  vesselTypeId: number  // vessel type — for config lookup
  name: string
  latLng: [number, number]
  headingDeg: number
  draggable: boolean
  selected?: boolean
  onDragEnd: (latLng: [number, number]) => void
  onClick?: (instanceId: number, typeId: number) => void
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

export function VesselMarker({ vesselId, vesselTypeId, name, latLng, headingDeg, draggable, selected = false, onDragEnd, onClick }: VesselMarkerProps) {
  // Refs hold the latest callbacks without causing eventHandlers to be recreated.
  // Direct assignment in render is the correct pattern for "always-current" refs.
  const onDragEndRef = useRef(onDragEnd)
  const onClickRef = useRef(onClick)
  onDragEndRef.current = onDragEnd
  onClickRef.current = onClick

  // Stable eventHandlers object — created once per vessel mount (vesselId/vesselTypeId never
  // change for a given mounted instance). react-leaflet only re-registers Leaflet listeners
  // when this object reference changes, so click is never unregistered during simulation.
  const eventHandlers = useMemo(() => ({
    dragend: (e: L.DragEndEvent) => {
      const { lat, lng } = (e.target as L.Marker).getLatLng()
      onDragEndRef.current([lat, lng])
    },
    click: () => onClickRef.current?.(vesselId, vesselTypeId),
  }), [vesselId, vesselTypeId])

  // Memoize the icon so it is NOT rebuilt on every position tick. During a running
  // simulation the map re-renders ~20 Hz; recreating the DivIcon each time makes
  // Leaflet replace the marker's DOM element, which swallows clicks (mousedown and
  // mouseup land on different elements). Rebuild only when the rounded heading or
  // selection changes — keeps the marker clickable at any time during the run.
  const roundedHeading = Math.round(headingDeg)
  const icon = useMemo(
    () => createVesselIcon(roundedHeading, selected),
    [roundedHeading, selected],
  )

  return (
    <Marker
      key={vesselId}
      position={latLng}
      icon={icon}
      draggable={draggable}
      eventHandlers={eventHandlers}
    >
      <Tooltip>{name}</Tooltip>
    </Marker>
  )
}
