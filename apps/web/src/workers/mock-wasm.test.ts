import { describe, it, expect } from 'vitest'
import createMockModule from './mock-wasm.js'

describe('MockYmirSimulation', () => {
  it('addVessel places vessel at origin', async () => {
    const { YmirSimulation } = await createMockModule()
    const sim = new YmirSimulation()
    sim.addVessel(1)
    const state = sim.getState()
    expect(state.vessels[0]).toMatchObject({ id: 1, x: 0, y: 0, psi: 0 })
  })

  it('addVesselAt places vessel at given x, y, psi', async () => {
    const { YmirSimulation } = await createMockModule()
    const sim = new YmirSimulation()
    sim.addVesselAt(1, 100, 200, Math.PI / 4)
    const state = sim.getState()
    expect(state.vessels[0]).toMatchObject({ id: 1, x: 100, y: 200, psi: Math.PI / 4 })
  })

  it('vessel stays at initial coords when no actuator commanded', async () => {
    const { YmirSimulation } = await createMockModule()
    const sim = new YmirSimulation()
    sim.addVesselAt(1, 500, 300, 0)
    sim.step(1.0)
    const state = sim.getState()
    // no thrust → u=0, no motion
    expect(state.vessels[0].x).toBeCloseTo(500, 2)
    expect(state.vessels[0].y).toBeCloseTo(300, 2)
  })

  it('thruster command drives vessel forward', async () => {
    const { YmirSimulation } = await createMockModule()
    const sim = new YmirSimulation()
    sim.addVesselAt(1, 0, 0, 0)
    sim.setThrusterCommand(1, 1, 100, 0) // full ahead, no azimuth
    sim.step(1.0)
    const state = sim.getState()
    // should have moved forward (x > 0 with heading psi=0)
    expect(state.vessels[0].x).toBeGreaterThan(0)
  })
})
