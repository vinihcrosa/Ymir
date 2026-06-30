import { create } from 'zustand'

export type ViewMode = 'map' | '3d'
/** Onboard camera ids from vessel_config (celso_furtado). */
export type CameraId = 'Front' | 'Back' | 'Bridge' | 'Starboard' | 'Portside'

interface ViewStore {
  mode: ViewMode
  cameraId: CameraId
  setMode: (m: ViewMode) => void
  toggle: () => void
  setCamera: (c: CameraId) => void
}

/** Which background the scenario workspace shows — 2D map or the 3D scene — and,
 *  in 3D, which onboard vessel camera is active. Only the background swaps; all
 *  panels are shared. */
export const useViewStore = create<ViewStore>((set, get) => ({
  mode: 'map',
  cameraId: 'Front',
  setMode: (mode) => set({ mode }),
  toggle: () => set({ mode: get().mode === 'map' ? '3d' : 'map' }),
  setCamera: (cameraId) => set({ cameraId }),
}))
