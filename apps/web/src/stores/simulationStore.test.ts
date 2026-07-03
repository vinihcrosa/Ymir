import { describe, it, expect, beforeEach, afterEach } from 'vitest'
import type { ClientMessage, ServerMessage } from '@ymir/types'
import { SimulationSocket } from '../lib/simulation-socket'
import { useSimulationStore, __setSocketFactory } from './simulationStore'
import { useEnvironmentStore } from './environmentStore'
import { useVesselPanelStore } from './vesselPanelStore'

class FakeWebSocket {
  static CONNECTING = 0
  static OPEN = 1
  static CLOSED = 3
  static instances: FakeWebSocket[] = []
  readyState = FakeWebSocket.CONNECTING
  onopen: (() => void) | null = null
  onmessage: ((e: { data: string }) => void) | null = null
  onclose: (() => void) | null = null
  sent: string[] = []
  constructor(public url: string) { FakeWebSocket.instances.push(this) }
  send(data: string) { this.sent.push(data) }
  close() { this.readyState = FakeWebSocket.CLOSED; this.onclose?.() }
  open() { this.readyState = FakeWebSocket.OPEN; this.onopen?.() }
  emit(msg: ServerMessage) { this.onmessage?.({ data: JSON.stringify(msg) }) }
  get sentMessages(): ClientMessage[] { return this.sent.map((s) => JSON.parse(s) as ClientMessage) }
}

function makeStorage() {
  const m = new Map<string, string>()
  return {
    getItem: (k: string) => m.get(k) ?? null,
    setItem: (k: string, v: string) => { m.set(k, v) },
    removeItem: (k: string) => { m.delete(k) },
  }
}

let storage = makeStorage()
const store = () => useSimulationStore.getState()
const lastWs = () => FakeWebSocket.instances[FakeWebSocket.instances.length - 1]

beforeEach(() => {
  FakeWebSocket.instances = []
  storage = makeStorage()
  __setSocketFactory(() => new SimulationSocket({
    url: 'ws://test/ws',
    WebSocketImpl: FakeWebSocket as unknown as typeof WebSocket,
    storage,
  }))
  store().reset()
  useEnvironmentStore.getState().reset()
  useVesselPanelStore.setState({ selectedVesselId: null, rudderAngles: {}, thrusterPowers: {}, thrusterAzimuths: {} })
})

afterEach(() => { __setSocketFactory(null) })

/** Boot a session up to a confirmed simulation and return its id. */
function createSession(dt = 0.05): string {
  store().play(dt)
  lastWs().open()
  lastWs().emit({ type: 'SimulationCreated', simId: 'sim-1' })
  return 'sim-1'
}

describe('simulationStore — initial state', () => {
  it('starts idle with no socket or session', () => {
    expect(store().status).toBe('idle')
    expect(store().socket).toBeNull()
    expect(store().simId).toBeNull()
    expect(store().scenarioVessels).toEqual([])
    expect(store().hasSession()).toBe(false)
  })
})

describe('simulationStore — play/create flow', () => {
  it('creates a socket and enters loading on first play', () => {
    store().play(0.05)
    expect(store().status).toBe('loading')
    expect(store().socket).not.toBeNull()
    expect(FakeWebSocket.instances).toHaveLength(1)
  })

  it('sends CreateSimulation on open and Play after SimulationCreated', () => {
    store().loadScenario([{ instanceId: 1, vesselId: 1, name: 'A', x: 0, y: 0, headingDeg: 0 }])
    store().play(0.1)
    lastWs().open()
    expect(lastWs().sentMessages.some((m) => m.type === 'CreateSimulation')).toBe(true)
    lastWs().emit({ type: 'SimulationCreated', simId: 'sim-9' })
    expect(store().simId).toBe('sim-9')
    const play = lastWs().sentMessages.find((m) => m.type === 'Play')
    expect(play).toEqual({ type: 'Play', simId: 'sim-9', dt: 0.1 })
    expect(store().status).toBe('running')
  })

  it('resumes with a Play when a sim id already exists (reconnect)', () => {
    storage.setItem('ymir.simId', 'sim-existing')
    store().play(0.05)
    lastWs().open()
    const msgs = lastWs().sentMessages
    expect(msgs).toContainEqual({ type: 'AttachSimulation', simId: 'sim-existing' })
    expect(msgs.some((m) => m.type === 'Play' && m.simId === 'sim-existing')).toBe(true)
    expect(store().status).toBe('running')
  })
})

describe('simulationStore — server messages', () => {
  it('State updates the store state', () => {
    createSession()
    lastWs().emit({ type: 'State', payload: { t: 3, vessels: [] } })
    expect(store().state).toEqual({ t: 3, vessels: [] })
  })

  it('Status paused sets status; ended maps to idle', () => {
    createSession()
    lastWs().emit({ type: 'Status', status: 'paused' })
    expect(store().status).toBe('paused')
    lastWs().emit({ type: 'Status', status: 'ended' })
    expect(store().status).toBe('idle')
  })

  it('Error sets status error and message', () => {
    createSession()
    lastWs().emit({ type: 'Error', message: 'boom' })
    expect(store().status).toBe('error')
    expect(store().error).toBe('boom')
  })

  it('re-syncs actuator state after SimulationCreated', () => {
    useVesselPanelStore.setState({ selectedVesselId: 1, thrusterPowers: { 0: 80 }, thrusterAzimuths: { 0: 10 }, rudderAngles: { 0: 15 } })
    createSession()
    const acts = lastWs().sentMessages.filter((m) => m.type === 'SetActuator')
    expect(acts.some((m) => m.type === 'SetActuator' && m.deviceType === 'thruster' && m.value === 80)).toBe(true)
    expect(acts.some((m) => m.type === 'SetActuator' && m.deviceType === 'rudder' && m.value === 15)).toBe(true)
  })

  it('tracks connection state changes', () => {
    store().play()
    expect(store().connection).toBe('connecting')
    lastWs().open()
    expect(store().connection).toBe('open')
  })
})

describe('simulationStore — commands', () => {
  it('pause sends Pause and sets status paused', () => {
    const simId = createSession()
    store().pause()
    expect(lastWs().sentMessages.some((m) => m.type === 'Pause' && m.simId === simId)).toBe(true)
    expect(store().status).toBe('paused')
  })

  it('pause sets paused even without a session', () => {
    store().pause()
    expect(store().status).toBe('paused')
  })

  it('loadScenario forwards to an active session', () => {
    createSession()
    const vessels = [{ instanceId: 2, vesselId: 2, name: 'B', x: 1, y: 2, headingDeg: 90 }]
    store().loadScenario(vessels)
    expect(lastWs().sentMessages.some((m) => m.type === 'LoadScenario')).toBe(true)
    expect(store().scenarioVessels).toEqual(vessels)
  })

  it('loadScenario without a session only stores vessels', () => {
    const vessels = [{ instanceId: 3, vesselId: 3, name: 'C', x: 0, y: 0, headingDeg: 0 }]
    store().loadScenario(vessels)
    expect(store().scenarioVessels).toEqual(vessels)
  })

  it('applyEnvironment sends LoadEnvironment when conditions exist', () => {
    createSession()
    useEnvironmentStore.setState({ currentSeries: [[{ t: 0, speed: 2, dirNaut: 90 }]] })
    store().applyEnvironment()
    const env = lastWs().sentMessages.find((m) => m.type === 'LoadEnvironment')
    expect(env).toBeTruthy()
  })

  it('applyEnvironment is a no-op with an empty environment', () => {
    createSession()
    store().applyEnvironment()
    expect(lastWs().sentMessages.some((m) => m.type === 'LoadEnvironment')).toBe(false)
  })

  it('sendActuator sends SetActuator with the sim id', () => {
    const simId = createSession()
    store().sendActuator(1, 'thruster', 0, 75, 20)
    const act = lastWs().sentMessages.find((m) => m.type === 'SetActuator')
    expect(act).toEqual({ type: 'SetActuator', simId, vesselId: 1, deviceType: 'thruster', deviceId: 0, value: 75, value2: 20 })
  })

  it('sendActuator is a no-op without a session', () => {
    store().sendActuator(1, 'rudder', 0, 10)
    expect(FakeWebSocket.instances).toHaveLength(0)
  })
})

describe('simulationStore — reset', () => {
  it('sends Reset, closes the socket, and clears state', () => {
    const simId = createSession()
    store().reset()
    expect(lastWs().sentMessages.some((m) => m.type === 'Reset' && m.simId === simId)).toBe(true)
    expect(store().status).toBe('idle')
    expect(store().socket).toBeNull()
    expect(store().simId).toBeNull()
    expect(store().scenarioVessels).toEqual([])
  })
})
