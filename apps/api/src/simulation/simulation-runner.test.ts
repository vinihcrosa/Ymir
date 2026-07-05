import { describe, it, expect, afterEach, vi } from 'vitest'
import { SimulationRunner } from './simulation-runner.js'
import type { ScenarioDraftVesselDTO } from '@ymir/types'

// Real WASM engine — no engine mock (ADR-005). Requires `pnpm build:wasm`.

let runner: SimulationRunner | null = null

afterEach(() => {
  runner?.dispose()
  runner = null
})

function vessel(over: Partial<ScenarioDraftVesselDTO> = {}): ScenarioDraftVesselDTO {
  return { instanceId: 1, vesselId: 1, name: 'v', x: 0, y: 0, headingDeg: 0, ...over }
}

function tickN(r: SimulationRunner, dt: number, n: number): void {
  for (let i = 0; i < n; i++) r.tick(dt)
}

describe('SimulationRunner', () => {
  it('creates with an empty simulation at t=0', async () => {
    runner = await SimulationRunner.create()
    const s = runner.getState()
    expect(s.t).toBe(0)
    expect(s.vessels).toHaveLength(0)
  })

  it('loadScenario builds a fresh engine with the given vessels', async () => {
    runner = await SimulationRunner.create()
    await runner.loadScenario([
      vessel({ instanceId: 1, x: 10, y: 20, headingDeg: 0 }),
      vessel({ instanceId: 2, x: -30, y: 5, headingDeg: 90 }),
    ])
    const s = runner.getState()
    expect(s.vessels).toHaveLength(2)
    expect(s.vessels[0].x).toBeCloseTo(10, 3)
    expect(s.vessels[1].psi).toBeCloseTo(Math.PI / 2, 3)
  })

  it('loadScenario twice rebuilds from scratch (no leftover vessels)', async () => {
    runner = await SimulationRunner.create()
    await runner.loadScenario([vessel({ instanceId: 1 }), vessel({ instanceId: 2 })])
    await runner.loadScenario([vessel({ instanceId: 5, x: 1 })])
    const s = runner.getState()
    expect(s.vessels).toHaveLength(1)
    expect(s.vessels[0].id).toBe(5)
  })

  it('thruster command drives the vessel forward in surge', async () => {
    runner = await SimulationRunner.create()
    await runner.loadScenario([vessel({ instanceId: 1 })])
    runner.setActuator(1, 'thruster', 0, 100, 0)
    tickN(runner, 0.1, 100)
    const v = runner.getState().vessels[0]
    expect(v.u).toBeGreaterThan(0)
    expect(v.x).toBeGreaterThan(0)
  })

  it('rudder deflection while making way produces a yaw rate', async () => {
    runner = await SimulationRunner.create()
    await runner.loadScenario([vessel({ instanceId: 1 })])
    runner.setActuator(1, 'thruster', 0, 100, 0)
    tickN(runner, 0.1, 50)
    runner.setActuator(1, 'rudder', 0, 35)
    tickN(runner, 0.1, 50)
    expect(Math.abs(runner.getState().vessels[0].r)).toBeGreaterThan(0)
  })

  it('loadEnvironment with a current changes vessel motion vs calm water', async () => {
    const calm = await SimulationRunner.create()
    const flow = await SimulationRunner.create()
    try {
      await calm.loadScenario([vessel({ instanceId: 1 })])
      await flow.loadScenario([vessel({ instanceId: 1 })])
      const env = JSON.stringify({
        currentSeries: [[{ t: 0, speed: 5, dirNaut: 90 }]],
        windSeries: [],
        waveSeries: [],
      })
      flow.loadEnvironment(env)
      tickN(calm, 0.1, 100)
      tickN(flow, 0.1, 100)
      const a = calm.getState().vessels[0]
      const b = flow.getState().vessels[0]
      // Any measurable divergence proves the environment reaches the physics.
      const diff = Math.abs(a.x - b.x) + Math.abs(a.y - b.y) + Math.abs(a.v - b.v)
      expect(diff).toBeGreaterThan(1e-6)
    } finally {
      calm.dispose()
      flow.dispose()
    }
  })

  it('loadEnvironment applies each spectrum type and tolerates malformed JSON', async () => {
    runner = await SimulationRunner.create()
    await runner.loadScenario([vessel({ instanceId: 1 })])
    for (const spectrum of ['REGULAR', 'PIERSON', 'JONSWAP']) {
      const env = JSON.stringify({
        currentSeries: [],
        windSeries: [],
        waveSeries: [{ t: 0, Hs: 2, Tp: 8, dirNaut: 180, spectrum, gamma: 3.3 }],
      })
      expect(() => runner!.loadEnvironment(env)).not.toThrow()
    }
    expect(() => runner!.loadEnvironment('not-json')).not.toThrow()
    expect(() => runner!.loadEnvironment('{}')).not.toThrow()
    tickN(runner, 0.05, 5)
    expect(Number.isFinite(runner.getState().vessels[0].z)).toBe(true)
  })

  it('reset does not throw and leaves the engine steppable', async () => {
    runner = await SimulationRunner.create()
    await runner.loadScenario([vessel({ instanceId: 1 })])
    runner.setActuator(1, 'thruster', 0, 100, 0)
    tickN(runner, 0.1, 20)
    expect(() => runner!.reset()).not.toThrow()
    tickN(runner, 0.1, 5)
    expect(Number.isFinite(runner.getState().vessels[0].x)).toBe(true)
  })

  it('play runs the loop on a timer and stop halts it', async () => {
    vi.useFakeTimers()
    try {
      runner = await SimulationRunner.create()
      await runner.loadScenario([vessel({ instanceId: 1 })])
      const states: number[] = []
      expect(runner.running).toBe(false)
      runner.play(undefined, (s) => states.push(s.t))
      expect(runner.running).toBe(true)
      runner.play(0.05, () => { /* second call is a no-op while running */ })
      vi.advanceTimersByTime(160) // ~3 ticks at 50 ms
      const countAfterRun = states.length
      expect(countAfterRun).toBeGreaterThanOrEqual(2)
      runner.stop()
      expect(runner.running).toBe(false)
      vi.advanceTimersByTime(200)
      expect(states.length).toBe(countAfterRun) // no more ticks after stop
    } finally {
      vi.useRealTimers()
    }
  })
})
