import { create } from 'zustand'
import type { VesselConfigDTO } from '@ymir/types'

interface VesselPanelStore {
  selectedVesselId: number | null
  config: VesselConfigDTO | null
  configLoading: boolean
  rudderAngles: Record<number, number>
  thrusterPowers: Record<number, number>
  thrusterAzimuths: Record<number, number>

  open: (vesselId: number) => void
  close: () => void
  setConfig: (config: VesselConfigDTO | null, loading: boolean) => void
  setRudderAngle: (rudderId: number, deg: number) => void
  setThrusterPower: (thrusterId: number, pct: number) => void
  setThrusterAzimuth: (thrusterId: number, deg: number) => void
}

export const useVesselPanelStore = create<VesselPanelStore>((set) => ({
  selectedVesselId: null,
  config: null,
  configLoading: false,
  rudderAngles: {},
  thrusterPowers: {},
  thrusterAzimuths: {},

  open: (vesselId) => set({
    selectedVesselId: vesselId,
    config: null,
    configLoading: true,
    rudderAngles: {},
    thrusterPowers: {},
    thrusterAzimuths: {},
  }),
  close: () => set({ selectedVesselId: null, config: null, configLoading: false }),
  setConfig: (config, loading) => set({ config, configLoading: loading }),
  setRudderAngle: (rudderId, deg) => set((s) => ({
    rudderAngles: { ...s.rudderAngles, [rudderId]: deg },
  })),
  setThrusterPower: (thrusterId, pct) => set((s) => ({
    thrusterPowers: { ...s.thrusterPowers, [thrusterId]: pct },
  })),
  setThrusterAzimuth: (thrusterId, deg) => set((s) => ({
    thrusterAzimuths: { ...s.thrusterAzimuths, [thrusterId]: deg },
  })),
}))
