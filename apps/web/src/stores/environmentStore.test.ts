import { describe, it, expect, beforeEach } from 'vitest'
import { Value } from '@sinclair/typebox/value'
import { EnvironmentProfileDTO } from '@ymir/types'
import { useEnvironmentStore } from './environmentStore'

const store = () => useEnvironmentStore.getState()

beforeEach(() => {
  store().reset()
})

describe('initial state', () => {
  it('hasConditions returns false', () => {
    expect(store().hasConditions()).toBe(false)
  })

  it('all series are empty arrays', () => {
    const s = store()
    expect(s.currentSeries).toEqual([])
    expect(s.windSeries).toEqual([])
    expect(s.waveSeries).toEqual([])
  })
})

describe('currentSeries actions', () => {
  it('addCurrentSeries increases length by 1', () => {
    store().addCurrentSeries()
    expect(store().currentSeries).toHaveLength(1)
  })

  it('addCurrentSeries initializes series with one default keyframe', () => {
    store().addCurrentSeries()
    const series = store().currentSeries[0]
    expect(series).toHaveLength(1)
    expect(series[0]).toMatchObject({ t: 0, speed: 0, dirNaut: 0 })
  })

  it('removeCurrentSeries removes the series at given index', () => {
    store().addCurrentSeries()
    store().addCurrentSeries()
    store().removeCurrentSeries(0)
    expect(store().currentSeries).toHaveLength(1)
  })

  it('setCurrentKeyframe patches the keyframe', () => {
    store().addCurrentSeries()
    store().setCurrentKeyframe(0, 0, { speed: 2.5 })
    expect(store().currentSeries[0][0].speed).toBe(2.5)
  })

  it('addCurrentKeyframe appends keyframe at t = lastKeyframe.t + 300', () => {
    store().addCurrentSeries()
    store().addCurrentKeyframe(0)
    const series = store().currentSeries[0]
    expect(series).toHaveLength(2)
    expect(series[1].t).toBe(300)
  })

  it('addCurrentKeyframe second append is at t = 600', () => {
    store().addCurrentSeries()
    store().addCurrentKeyframe(0)
    store().addCurrentKeyframe(0)
    expect(store().currentSeries[0][2].t).toBe(600)
  })

  it('removeCurrentKeyframe removes keyframe at given index', () => {
    store().addCurrentSeries()
    store().addCurrentKeyframe(0)
    store().removeCurrentKeyframe(0, 1)
    expect(store().currentSeries[0]).toHaveLength(1)
  })

  it('does not mutate other series when editing one', () => {
    store().addCurrentSeries()
    store().addCurrentSeries()
    store().setCurrentKeyframe(0, 0, { speed: 5 })
    expect(store().currentSeries[1][0].speed).toBe(0)
  })

  it('addCurrentKeyframe only appends to target series when multiple series exist', () => {
    store().addCurrentSeries()
    store().addCurrentSeries()
    store().addCurrentKeyframe(0)
    expect(store().currentSeries[0]).toHaveLength(2)
    expect(store().currentSeries[1]).toHaveLength(1)
  })

  it('removeCurrentKeyframe only removes from target series', () => {
    store().addCurrentSeries()
    store().addCurrentKeyframe(0)
    store().addCurrentSeries()
    store().addCurrentKeyframe(1)
    store().removeCurrentKeyframe(0, 1)
    expect(store().currentSeries[0]).toHaveLength(1)
    expect(store().currentSeries[1]).toHaveLength(2)
  })

  it('addCurrentKeyframe on empty series starts at t=0', () => {
    store().addCurrentSeries()
    store().removeCurrentKeyframe(0, 0)
    store().addCurrentKeyframe(0)
    expect(store().currentSeries[0][0].t).toBe(0)
  })
})

describe('windSeries actions', () => {
  it('addWindSeries increases length by 1 with one default keyframe', () => {
    store().addWindSeries()
    expect(store().windSeries).toHaveLength(1)
    expect(store().windSeries[0]).toHaveLength(1)
  })

  it('setWindKeyframe patches the keyframe', () => {
    store().addWindSeries()
    store().setWindKeyframe(0, 0, { speed: 10 })
    expect(store().windSeries[0][0].speed).toBe(10)
  })

  it('addWindKeyframe appends at t = lastKeyframe.t + 300', () => {
    store().addWindSeries()
    store().addWindKeyframe(0)
    expect(store().windSeries[0][1].t).toBe(300)
  })

  it('removeWindKeyframe removes keyframe at given index', () => {
    store().addWindSeries()
    store().addWindKeyframe(0)
    store().removeWindKeyframe(0, 1)
    expect(store().windSeries[0]).toHaveLength(1)
  })

  it('removeWindSeries removes the series', () => {
    store().addWindSeries()
    store().removeWindSeries(0)
    expect(store().windSeries).toHaveLength(0)
  })

  it('setWindKeyframe does not mutate other wind series', () => {
    store().addWindSeries()
    store().addWindSeries()
    store().setWindKeyframe(0, 0, { speed: 8 })
    expect(store().windSeries[1][0].speed).toBe(0)
  })

  it('addWindKeyframe only appends to target wind series', () => {
    store().addWindSeries()
    store().addWindSeries()
    store().addWindKeyframe(0)
    expect(store().windSeries[0]).toHaveLength(2)
    expect(store().windSeries[1]).toHaveLength(1)
  })

  it('addWindKeyframe on empty series starts at t=0', () => {
    store().addWindSeries()
    store().removeWindKeyframe(0, 0)
    store().addWindKeyframe(0)
    expect(store().windSeries[0][0].t).toBe(0)
  })
})

describe('waveSeries actions', () => {
  it('addWaveKeyframe appends a default keyframe', () => {
    store().addWaveKeyframe()
    expect(store().waveSeries).toHaveLength(1)
    expect(store().waveSeries[0]).toMatchObject({ t: 0, Hs: 0, Tp: 1, dirNaut: 0, spectrum: 'JONSWAP', gamma: 3.3 })
  })

  it('addWaveKeyframe second time appends at t + 300', () => {
    store().addWaveKeyframe()
    store().addWaveKeyframe()
    expect(store().waveSeries[1].t).toBe(300)
  })

  it('setWaveKeyframe patches the keyframe', () => {
    store().addWaveKeyframe()
    store().setWaveKeyframe(0, { Hs: 2.0 })
    expect(store().waveSeries[0].Hs).toBe(2.0)
  })

  it('removeWaveKeyframe removes at given index', () => {
    store().addWaveKeyframe()
    store().addWaveKeyframe()
    store().removeWaveKeyframe(0)
    expect(store().waveSeries).toHaveLength(1)
  })

  it('setWaveKeyframe does not mutate other wave keyframes', () => {
    store().addWaveKeyframe()
    store().addWaveKeyframe()
    store().setWaveKeyframe(0, { Hs: 5.0 })
    expect(store().waveSeries[1].Hs).toBe(0)
  })
})

describe('hasConditions', () => {
  it('returns true after addCurrentSeries', () => {
    store().addCurrentSeries()
    expect(store().hasConditions()).toBe(true)
  })

  it('returns true after addWindSeries', () => {
    store().addWindSeries()
    expect(store().hasConditions()).toBe(true)
  })

  it('returns true after addWaveKeyframe', () => {
    store().addWaveKeyframe()
    expect(store().hasConditions()).toBe(true)
  })

  it('returns false after reset', () => {
    store().addCurrentSeries()
    store().addWindSeries()
    store().reset()
    expect(store().hasConditions()).toBe(false)
  })
})

describe('reset', () => {
  it('clears all series', () => {
    store().addCurrentSeries()
    store().addWindSeries()
    store().addWaveKeyframe()
    store().reset()
    const s = store()
    expect(s.currentSeries).toEqual([])
    expect(s.windSeries).toEqual([])
    expect(s.waveSeries).toEqual([])
  })
})

describe('toJson', () => {
  it('returns valid JSON parseable by JSON.parse', () => {
    store().addCurrentSeries()
    const json = store().toJson()
    expect(() => JSON.parse(json)).not.toThrow()
  })

  it('output validates against EnvironmentProfileDTO schema', () => {
    store().addCurrentSeries()
    store().addWindSeries()
    store().addWaveKeyframe()
    const parsed = JSON.parse(store().toJson())
    expect(Value.Check(EnvironmentProfileDTO, parsed)).toBe(true)
  })

  it('empty store produces valid empty arrays', () => {
    const parsed = JSON.parse(store().toJson())
    expect(Value.Check(EnvironmentProfileDTO, parsed)).toBe(true)
    expect(parsed.currentSeries).toEqual([])
    expect(parsed.windSeries).toEqual([])
    expect(parsed.waveSeries).toEqual([])
  })

  it('keyframes within a series are sorted by t ascending regardless of insertion order', () => {
    store().addCurrentSeries()
    // Manually patch keyframes out of order
    store().addCurrentKeyframe(0) // t=300
    store().setCurrentKeyframe(0, 0, { t: 900 }) // first kf now t=900
    store().setCurrentKeyframe(0, 1, { t: 0 })   // second kf now t=0

    const parsed = JSON.parse(store().toJson())
    const ts = parsed.currentSeries[0].map((kf: { t: number }) => kf.t)
    expect(ts).toEqual([...ts].sort((a: number, b: number) => a - b))
  })

  it('round-trip: parsed object matches original state structure', () => {
    store().addCurrentSeries()
    store().setCurrentKeyframe(0, 0, { speed: 1.5, dirNaut: 90 })
    store().addWindSeries()
    store().setWindKeyframe(0, 0, { speed: 5.0, dirNaut: 45 })
    store().addWaveKeyframe()
    store().setWaveKeyframe(0, { Hs: 2.0, Tp: 8.0, dirNaut: 270 })

    const parsed: { currentSeries: unknown[][], windSeries: unknown[][], waveSeries: unknown[] } = JSON.parse(store().toJson())

    expect(parsed.currentSeries).toHaveLength(1)
    expect(parsed.windSeries).toHaveLength(1)
    expect(parsed.waveSeries).toHaveLength(1)
    expect((parsed.currentSeries[0][0] as { speed: number }).speed).toBe(1.5)
    expect((parsed.windSeries[0][0] as { speed: number }).speed).toBe(5.0)
    expect((parsed.waveSeries[0] as { Hs: number }).Hs).toBe(2.0)
  })
})
