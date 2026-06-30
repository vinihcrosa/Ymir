import { create } from 'zustand'

export type ViewMode = 'map' | '3d'

interface ViewStore {
  mode: ViewMode
  setMode: (m: ViewMode) => void
  toggle: () => void
}

/** Which background the scenario workspace shows — 2D map or the 3D scene.
 *  Only the background swaps; all panels are shared. */
export const useViewStore = create<ViewStore>((set, get) => ({
  mode: 'map',
  setMode: (mode) => set({ mode }),
  toggle: () => set({ mode: get().mode === 'map' ? '3d' : 'map' }),
}))
