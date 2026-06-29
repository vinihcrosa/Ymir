import { create } from 'zustand'
import type { Map as LeafletMap } from 'leaflet'

/**
 * Holds the live Leaflet map instance so chrome rendered outside the
 * `MapContainer` (e.g. the floating `MapActions` zoom pill) can drive it.
 */
interface MapStore {
  map: LeafletMap | null
  setMap: (map: LeafletMap | null) => void
  zoomIn: () => void
  zoomOut: () => void
}

export const useMapStore = create<MapStore>((set, get) => ({
  map: null,
  setMap: (map) => set({ map }),
  zoomIn: () => get().map?.zoomIn(),
  zoomOut: () => get().map?.zoomOut(),
}))
