import { describe, it, expect, afterEach } from 'vitest'
import { createSimulationEngine, type SimulationEngine } from './index.js'

// These tests exercise the REAL WASM engine (no mock — see ADR-005).
// They require `pnpm build:wasm` to have produced runtime/ymir.js first.

let engine: SimulationEngine | null = null

afterEach(() => {
  if (engine) {
    engine.delete()
    engine = null
  }
})

function step(e: SimulationEngine, dt: number, n: number): void {
  for (let i = 0; i < n; i++) e.step(dt)
}

describe('createSimulationEngine', () => {
  it('resolves an engine exposing the full SimulationEngine surface', async () => {
    engine = await createSimulationEngine()
    for (const method of [
      'addVessel', 'addVesselAt', 'setRudderAngle', 'setThrusterCommand',
      'step', 'getState', 'loadEnvironment', 'setWaveConditions', 'reset', 'delete',
    ]) {
      expect(typeof (engine as unknown as Record<string, unknown>)[method]).toBe('function')
    }
  })

  it('places a vessel at the given position at rest', async () => {
    engine = await createSimulationEngine()
    engine.addVesselAt(1, 100, -50, 0)
    const state = engine.getState()
    expect(state.t).toBe(0)
    expect(state.vessels).toHaveLength(1)
    const v = state.vessels[0]
    expect(v.id).toBe(1)
    expect(v.x).toBeCloseTo(100, 3)
    expect(v.y).toBeCloseTo(-50, 3)
    expect(v.psi).toBeCloseTo(0, 3)
    expect(v.u).toBeCloseTo(0, 3)
    expect(v.v).toBeCloseTo(0, 3)
    expect(v.r).toBeCloseTo(0, 3)
  })

  it('keeps a vessel essentially at rest with no thrust', async () => {
    engine = await createSimulationEngine()
    engine.addVesselAt(1, 0, 0, 0)
    step(engine, 0.05, 40)
    const v = engine.getState().vessels[0]
    // No propulsion → surge/sway speed stays negligible.
    expect(Math.abs(v.u)).toBeLessThan(0.05)
    expect(Math.abs(v.v)).toBeLessThan(0.05)
  })

  it('accelerates forward in surge under full thrust', async () => {
    engine = await createSimulationEngine()
    engine.addVesselAt(1, 0, 0, 0)
    engine.setThrusterCommand(1, 0, 100, 0)
    step(engine, 0.1, 100) // ~10 s simulated
    const v = engine.getState().vessels[0]
    expect(v.u).toBeGreaterThan(0) // moving ahead
    expect(v.x).toBeGreaterThan(0) // advanced along heading (psi = 0 → +x)
    expect(engine.getState().t).toBeGreaterThan(9)
  })

  it('develops a yaw rate when the rudder is deflected while making way', async () => {
    engine = await createSimulationEngine()
    engine.addVesselAt(1, 0, 0, 0)
    engine.setThrusterCommand(1, 0, 100, 0)
    step(engine, 0.1, 50) // build up speed first
    engine.setRudderAngle(1, 0, 35)
    step(engine, 0.1, 50)
    const v = engine.getState().vessels[0]
    expect(Math.abs(v.r)).toBeGreaterThan(0) // turning
  })

  it('reset runs without error and leaves the engine steppable', async () => {
    engine = await createSimulationEngine()
    engine.addVesselAt(1, 0, 0, 0)
    engine.setThrusterCommand(1, 0, 100, 0)
    step(engine, 0.1, 30)
    // reset() resets the per-body integrator clock and force-model state; it
    // does NOT rewind physical pose/velocity (real-engine behavior — ADR-005).
    expect(() => engine!.reset()).not.toThrow()
    step(engine, 0.1, 5)
    const v = engine.getState().vessels[0]
    expect(Number.isFinite(v.x)).toBe(true)
    expect(Number.isFinite(v.u)).toBe(true)
  })

  it('delete frees the instance without throwing', async () => {
    const e = await createSimulationEngine()
    e.addVesselAt(1, 0, 0, 0)
    expect(() => e.delete()).not.toThrow()
  })
})

describe('integration — real WASM in Node', () => {
  it('runs a short two-vessel scenario end to end without error', async () => {
    engine = await createSimulationEngine()
    engine.addVesselAt(1, 0, 0, 0)
    engine.addVesselAt(2, 500, 0, Math.PI / 2)
    engine.setThrusterCommand(1, 0, 60, 0)
    engine.loadEnvironment('{}') // no-op environment is accepted
    step(engine, 0.05, 100)
    const state = engine.getState()
    expect(state.vessels).toHaveLength(2)
    expect(Number.isFinite(state.vessels[0].x)).toBe(true)
    expect(Number.isFinite(state.vessels[1].y)).toBe(true)
  })
})
