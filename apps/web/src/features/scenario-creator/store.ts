import { create } from 'zustand'
import type { AreaDTO, CreateScenarioDTO } from '@ymir/types'

export interface ScenarioDraftVessel {
  instanceId: number   // unique per scenario instance — used by physics engine and React keys
  vesselId: number     // vessel type ID — used for config lookup only
  name: string
  x: number        // meters, relative to area origin
  y: number        // meters
  headingDeg: number  // 0-360
}

interface ScenarioStore {
  name: string
  areaId: number | null
  area: AreaDTO | null
  vessels: ScenarioDraftVessel[]
  _nextInstanceId: number
  setName: (name: string) => void
  setArea: (area: AreaDTO) => void
  addVessel: (v: Pick<ScenarioDraftVessel, 'vesselId' | 'name'>) => void
  removeVessel: (instanceId: number) => void
  updateVesselPosition: (instanceId: number, x: number, y: number) => void
  updateVesselHeading: (instanceId: number, headingDeg: number) => void
  reset: () => void
  toCreateScenarioDTO: () => CreateScenarioDTO
}

export const useScenarioStore = create<ScenarioStore>((set, get) => ({
  name: new Date().toLocaleDateString('pt-BR', { year: 'numeric', month: '2-digit', day: '2-digit' }),
  // NOTE: do NOT use Date.now() or new Date() inside actions — only in initial state or outside
  areaId: null,
  area: null,
  vessels: [],
  _nextInstanceId: 1,
  setName: (name) => set({ name }),
  setArea: (area) => set({ area, areaId: area.id }),
  addVessel: (v) => set((s) => ({
    _nextInstanceId: s._nextInstanceId + 1,
    vessels: [...s.vessels, { instanceId: s._nextInstanceId, ...v, x: 0, y: 0, headingDeg: 0 }]
  })),
  removeVessel: (instanceId) => set((s) => ({
    vessels: s.vessels.filter(v => v.instanceId !== instanceId)
  })),
  updateVesselPosition: (instanceId, x, y) => set((s) => ({
    vessels: s.vessels.map(v => v.instanceId === instanceId ? { ...v, x, y } : v)
  })),
  updateVesselHeading: (instanceId, headingDeg) => set((s) => ({
    vessels: s.vessels.map(v => v.instanceId === instanceId ? { ...v, headingDeg: ((headingDeg % 360) + 360) % 360 } : v)
  })),
  reset: () => set({ name: 'Cenário', areaId: null, area: null, vessels: [], _nextInstanceId: 1 }),
  toCreateScenarioDTO: () => {
    const { name, areaId, vessels } = get()
    return {
      name,
      areaId: areaId ?? undefined,
      duration: 3600,
      dt: 0.05,
      initialConditions: vessels.map(v => ({
        vesselId: v.instanceId,
        x: v.x,
        y: v.y,
        psi: v.headingDeg * Math.PI / 180,
        u: 0, v: 0, r: 0,
      })),
    }
  },
}))
