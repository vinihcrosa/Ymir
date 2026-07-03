import { describe, it, expect } from 'vitest'
import { Value } from '@sinclair/typebox/value'
import { ClientMessage, ServerMessage, ScenarioDraftVesselDTO, ScenarioInputDTO } from './protocol.js'

const draftVessel = { instanceId: 1, vesselId: 1, name: 'Ship A', x: 0, y: 0, headingDeg: 90 }

const validState = {
  t: 1.5,
  vessels: [
    { id: 1, x: 0, y: 0, z: 0, phi: 0, theta: 0, psi: 0, u: 1, v: 0, r: 0 },
  ],
}

describe('ScenarioDraftVesselDTO', () => {
  it('accepts a valid draft vessel', () => {
    expect(Value.Check(ScenarioDraftVesselDTO, draftVessel)).toBe(true)
  })

  it('rejects a draft vessel missing headingDeg', () => {
    const { headingDeg, ...rest } = draftVessel
    void headingDeg
    expect(Value.Check(ScenarioDraftVesselDTO, rest)).toBe(false)
  })
})

describe('ScenarioInputDTO', () => {
  it('accepts vessels with optional environmentJson', () => {
    expect(Value.Check(ScenarioInputDTO, { vessels: [draftVessel], environmentJson: '{}' })).toBe(true)
  })

  it('accepts vessels without environmentJson (optional)', () => {
    expect(Value.Check(ScenarioInputDTO, { vessels: [] })).toBe(true)
  })
})

describe('ClientMessage', () => {
  it('accepts a valid SetActuator (rudder)', () => {
    const msg = {
      type: 'SetActuator',
      simId: 'sim-1',
      vesselId: 1,
      deviceType: 'rudder',
      deviceId: 0,
      value: 15,
    }
    expect(Value.Check(ClientMessage, msg)).toBe(true)
  })

  it('accepts SetActuator (thruster) with value2', () => {
    const msg = {
      type: 'SetActuator',
      simId: 'sim-1',
      vesselId: 1,
      deviceType: 'thruster',
      deviceId: 0,
      value: 0.8,
      value2: 45,
    }
    expect(Value.Check(ClientMessage, msg)).toBe(true)
  })

  it("rejects SetActuator with deviceType 'invalid'", () => {
    const msg = {
      type: 'SetActuator',
      simId: 'sim-1',
      vesselId: 1,
      deviceType: 'invalid',
      deviceId: 0,
      value: 15,
    }
    expect(Value.Check(ClientMessage, msg)).toBe(false)
  })

  it('rejects an unknown type', () => {
    expect(Value.Check(ClientMessage, { type: 'Nonsense', simId: 'sim-1' })).toBe(false)
  })

  it('rejects Play without simId', () => {
    expect(Value.Check(ClientMessage, { type: 'Play' })).toBe(false)
  })

  it('accepts Play with simId and optional dt', () => {
    expect(Value.Check(ClientMessage, { type: 'Play', simId: 'sim-1', dt: 0.1 })).toBe(true)
    expect(Value.Check(ClientMessage, { type: 'Play', simId: 'sim-1' })).toBe(true)
  })

  it('accepts CreateSimulation with a scenario', () => {
    const msg = { type: 'CreateSimulation', scenario: { vessels: [draftVessel] } }
    expect(Value.Check(ClientMessage, msg)).toBe(true)
  })

  it('accepts AttachSimulation / Pause / Reset with simId', () => {
    expect(Value.Check(ClientMessage, { type: 'AttachSimulation', simId: 'sim-1' })).toBe(true)
    expect(Value.Check(ClientMessage, { type: 'Pause', simId: 'sim-1' })).toBe(true)
    expect(Value.Check(ClientMessage, { type: 'Reset', simId: 'sim-1' })).toBe(true)
  })

  it('accepts LoadScenario with vessels', () => {
    const msg = { type: 'LoadScenario', simId: 'sim-1', vessels: [draftVessel] }
    expect(Value.Check(ClientMessage, msg)).toBe(true)
  })

  it('accepts LoadEnvironment with json', () => {
    expect(Value.Check(ClientMessage, { type: 'LoadEnvironment', simId: 'sim-1', json: '{}' })).toBe(true)
  })
})

describe('ServerMessage', () => {
  it('accepts { type: "SimulationCreated", simId }', () => {
    expect(Value.Check(ServerMessage, { type: 'SimulationCreated', simId: 'sim-1' })).toBe(true)
  })

  it('accepts { type: "State", payload: <valid SimulationStateDTO> }', () => {
    expect(Value.Check(ServerMessage, { type: 'State', payload: validState })).toBe(true)
  })

  it('rejects State with a malformed payload', () => {
    const bad = { type: 'State', payload: { t: 'not-a-number', vessels: [] } }
    expect(Value.Check(ServerMessage, bad)).toBe(false)
  })

  it('accepts Status with running/paused/ended', () => {
    expect(Value.Check(ServerMessage, { type: 'Status', status: 'running' })).toBe(true)
    expect(Value.Check(ServerMessage, { type: 'Status', status: 'paused' })).toBe(true)
    expect(Value.Check(ServerMessage, { type: 'Status', status: 'ended' })).toBe(true)
  })

  it('rejects Status with an invalid status', () => {
    expect(Value.Check(ServerMessage, { type: 'Status', status: 'sleeping' })).toBe(false)
  })

  it('accepts Error with a message', () => {
    expect(Value.Check(ServerMessage, { type: 'Error', message: 'boom' })).toBe(true)
  })

  it('rejects an unknown server type', () => {
    expect(Value.Check(ServerMessage, { type: 'Whoops' })).toBe(false)
  })
})

describe('round-trip serialization', () => {
  it('serializes → JSON → parses → validates a ClientMessage without loss', () => {
    const msg = {
      type: 'SetActuator',
      simId: 'sim-1',
      vesselId: 1,
      deviceType: 'rudder',
      deviceId: 0,
      value: 15,
    }
    const restored = JSON.parse(JSON.stringify(msg))
    expect(Value.Check(ClientMessage, restored)).toBe(true)
    expect(restored).toEqual(msg)
  })

  it('serializes → JSON → parses → validates a ServerMessage without loss', () => {
    const msg = { type: 'State', payload: validState }
    const restored = JSON.parse(JSON.stringify(msg))
    expect(Value.Check(ServerMessage, restored)).toBe(true)
    expect(restored).toEqual(msg)
  })
})
