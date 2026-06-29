import { create } from 'zustand'
import type { EnvironmentConditionDTO, WaveConditionDTO, EnvironmentProfileDTO } from '@ymir/types'

const DEFAULT_CURRENT_KEYFRAME: EnvironmentConditionDTO = { t: 0, speed: 0, dirNaut: 0 }
const DEFAULT_WIND_KEYFRAME: EnvironmentConditionDTO = { t: 0, speed: 0, dirNaut: 0 }
const DEFAULT_WAVE_KEYFRAME: WaveConditionDTO = { t: 0, Hs: 0, Tp: 1, dirNaut: 0, spectrum: 'JONSWAP', gamma: 3.3 }

interface EnvironmentStore {
  currentSeries: EnvironmentConditionDTO[][]
  windSeries:    EnvironmentConditionDTO[][]
  waveSeries:    WaveConditionDTO[]

  addCurrentSeries:      () => void
  removeCurrentSeries:   (idx: number) => void
  setCurrentKeyframe:    (seriesIdx: number, kfIdx: number, patch: Partial<EnvironmentConditionDTO>) => void
  addCurrentKeyframe:    (seriesIdx: number) => void
  removeCurrentKeyframe: (seriesIdx: number, kfIdx: number) => void

  addWindSeries:      () => void
  removeWindSeries:   (idx: number) => void
  setWindKeyframe:    (seriesIdx: number, kfIdx: number, patch: Partial<EnvironmentConditionDTO>) => void
  addWindKeyframe:    (seriesIdx: number) => void
  removeWindKeyframe: (seriesIdx: number, kfIdx: number) => void

  addWaveKeyframe:    () => void
  removeWaveKeyframe: (idx: number) => void
  setWaveKeyframe:    (kfIdx: number, patch: Partial<WaveConditionDTO>) => void

  hasConditions: () => boolean
  toJson:        () => string
  reset:         () => void
}

export const useEnvironmentStore = create<EnvironmentStore>((set, get) => ({
  currentSeries: [],
  windSeries:    [],
  waveSeries:    [],

  addCurrentSeries: () => set((s) => ({
    currentSeries: [...s.currentSeries, [{ ...DEFAULT_CURRENT_KEYFRAME }]],
  })),

  removeCurrentSeries: (idx) => set((s) => ({
    currentSeries: s.currentSeries.filter((_, i) => i !== idx),
  })),

  setCurrentKeyframe: (seriesIdx, kfIdx, patch) => set((s) => {
    const next = s.currentSeries.map((series, si) => {
      if (si !== seriesIdx) return series
      return series.map((kf, ki) => ki === kfIdx ? { ...kf, ...patch } : kf)
    })
    return { currentSeries: next }
  }),

  addCurrentKeyframe: (seriesIdx) => set((s) => {
    const next = s.currentSeries.map((series, si) => {
      if (si !== seriesIdx) return series
      const last = series[series.length - 1]
      const t = last ? last.t + 300 : 0
      return [...series, { ...DEFAULT_CURRENT_KEYFRAME, t }]
    })
    return { currentSeries: next }
  }),

  removeCurrentKeyframe: (seriesIdx, kfIdx) => set((s) => {
    const next = s.currentSeries.map((series, si) => {
      if (si !== seriesIdx) return series
      return series.filter((_, ki) => ki !== kfIdx)
    })
    return { currentSeries: next }
  }),

  addWindSeries: () => set((s) => ({
    windSeries: [...s.windSeries, [{ ...DEFAULT_WIND_KEYFRAME }]],
  })),

  removeWindSeries: (idx) => set((s) => ({
    windSeries: s.windSeries.filter((_, i) => i !== idx),
  })),

  setWindKeyframe: (seriesIdx, kfIdx, patch) => set((s) => {
    const next = s.windSeries.map((series, si) => {
      if (si !== seriesIdx) return series
      return series.map((kf, ki) => ki === kfIdx ? { ...kf, ...patch } : kf)
    })
    return { windSeries: next }
  }),

  addWindKeyframe: (seriesIdx) => set((s) => {
    const next = s.windSeries.map((series, si) => {
      if (si !== seriesIdx) return series
      const last = series[series.length - 1]
      const t = last ? last.t + 300 : 0
      return [...series, { ...DEFAULT_WIND_KEYFRAME, t }]
    })
    return { windSeries: next }
  }),

  removeWindKeyframe: (seriesIdx, kfIdx) => set((s) => {
    const next = s.windSeries.map((series, si) => {
      if (si !== seriesIdx) return series
      return series.filter((_, ki) => ki !== kfIdx)
    })
    return { windSeries: next }
  }),

  addWaveKeyframe: () => set((s) => {
    const last = s.waveSeries[s.waveSeries.length - 1]
    const t = last ? last.t + 300 : 0
    return { waveSeries: [...s.waveSeries, { ...DEFAULT_WAVE_KEYFRAME, t }] }
  }),

  removeWaveKeyframe: (idx) => set((s) => ({
    waveSeries: s.waveSeries.filter((_, i) => i !== idx),
  })),

  setWaveKeyframe: (kfIdx, patch) => set((s) => ({
    waveSeries: s.waveSeries.map((kf, i) => i === kfIdx ? { ...kf, ...patch } : kf),
  })),

  hasConditions: () => {
    const s = get()
    return s.currentSeries.length > 0 || s.windSeries.length > 0 || s.waveSeries.length > 0
  },

  toJson: () => {
    const s = get()
    const sortByT = <T extends { t: number }>(arr: T[]): T[] =>
      [...arr].sort((a, b) => a.t - b.t)
    const payload: EnvironmentProfileDTO = {
      currentSeries: s.currentSeries.map(sortByT),
      windSeries:    s.windSeries.map(sortByT),
      waveSeries:    sortByT(s.waveSeries),
    }
    return JSON.stringify(payload)
  },

  reset: () => set({ currentSeries: [], windSeries: [], waveSeries: [] }),
}))
