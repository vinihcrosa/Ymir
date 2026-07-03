import { describe, it, expect, afterEach } from 'vitest'
import { MessageChannel } from 'node:worker_threads'
import { MessageRouter, WsClient, toServerMessage, type ClientTransport } from './message-router.js'
import { SimulationManager, type SimWorkerHandle } from './simulation-manager.js'
import { SimulationRunner } from './simulation-runner.js'
import { attachRunnerToPort, type MessagePortLike } from './simulation-worker.js'
import type { ServerMessage } from '@ymir/types'

const disposers: Array<() => void> = []
afterEach(() => { for (const d of disposers.splice(0)) d() })

function realEngineWorkerFactory(): SimWorkerHandle {
  const { port1, port2 } = new MessageChannel()
  let runner: SimulationRunner | null = null
  void SimulationRunner.create().then((r) => { runner = r; attachRunnerToPort(port1 as unknown as MessagePortLike, r) })
  disposers.push(() => { runner?.dispose(); port1.close(); port2.close() })
  return {
    postMessage: (cmd) => port2.postMessage(cmd),
    on: (_e, listener) => port2.on('message', listener as (v: unknown) => void),
    terminate: () => { runner?.dispose(); port1.close(); port2.close() },
  }
}

function fakeClient(): { client: WsClient; sent: ServerMessage[] } {
  const sent: ServerMessage[] = []
  const transport: ClientTransport = { send: (data) => sent.push(JSON.parse(data) as ServerMessage) }
  return { client: new WsClient(transport), sent }
}

const delay = (ms: number) => new Promise((r) => setTimeout(r, ms))
const V = { instanceId: 1, vesselId: 1, name: 'v', x: 0, y: 0, headingDeg: 0 }

describe('toServerMessage', () => {
  it('maps worker events to network messages and drops ready', () => {
    expect(toServerMessage({ type: 'status', status: 'paused' })).toEqual({ type: 'Status', status: 'paused' })
    expect(toServerMessage({ type: 'error', message: 'x' })).toEqual({ type: 'Error', message: 'x' })
    expect(toServerMessage({ type: 'ready' })).toBeNull()
  })
})

describe('MessageRouter', () => {
  it('rejects invalid JSON and malformed messages', () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory })
    const router = new MessageRouter(mgr)
    const { client, sent } = fakeClient()
    router.handleRaw(client, '{not json')
    router.handleRaw(client, JSON.stringify({ type: 'Nope' }))
    expect(sent.filter((m) => m.type === 'Error')).toHaveLength(2)
  })

  it('CreateSimulation returns SimulationCreated and binds the sim id', () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory })
    const router = new MessageRouter(mgr)
    const { client, sent } = fakeClient()
    router.handleRaw(client, JSON.stringify({ type: 'CreateSimulation', scenario: { vessels: [V] } }))
    const created = sent.find((m) => m.type === 'SimulationCreated')
    expect(created).toBeTruthy()
    expect(client.simId).toBe(created && created.type === 'SimulationCreated' ? created.simId : null)
    expect(mgr.size).toBe(1)
  })

  it('AttachSimulation to an unknown sim replies with Error', () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory })
    const router = new MessageRouter(mgr)
    const { client, sent } = fakeClient()
    router.handleRaw(client, JSON.stringify({ type: 'AttachSimulation', simId: 'ghost' }))
    expect(sent.some((m) => m.type === 'Error')).toBe(true)
    expect(client.simId).toBeNull()
  })

  it('forwards play/actuator commands and streams State back', async () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory })
    const router = new MessageRouter(mgr)
    const { client, sent } = fakeClient()
    router.handleRaw(client, JSON.stringify({ type: 'CreateSimulation', scenario: { vessels: [V] } }))
    const simId = client.simId!
    router.handleRaw(client, JSON.stringify({ type: 'SetActuator', simId, vesselId: 1, deviceType: 'thruster', deviceId: 0, value: 100 }))
    router.handleRaw(client, JSON.stringify({ type: 'Play', simId }))
    await delay(180)
    router.handleRaw(client, JSON.stringify({ type: 'Pause', simId }))
    expect(sent.some((m) => m.type === 'State')).toBe(true)
    expect(sent.some((m) => m.type === 'Status' && m.status === 'running')).toBe(true)
    await mgr.stop(simId)
  })

  it('replies with Error when create fails at capacity', () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory, maxSimulations: 0 })
    const router = new MessageRouter(mgr)
    const { client, sent } = fakeClient()
    router.handleRaw(client, JSON.stringify({ type: 'CreateSimulation', scenario: { vessels: [] } }))
    expect(sent.some((m) => m.type === 'Error')).toBe(true)
    expect(client.simId).toBeNull()
  })

  it('CreateSimulation forwards an environment when provided', () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory })
    const router = new MessageRouter(mgr)
    const { client, sent } = fakeClient()
    const environmentJson = JSON.stringify({ currentSeries: [], windSeries: [], waveSeries: [] })
    router.handleRaw(client, JSON.stringify({ type: 'CreateSimulation', scenario: { vessels: [V], environmentJson } }))
    expect(sent.some((m) => m.type === 'SimulationCreated')).toBe(true)
    expect(client.simId).toBeTruthy()
  })

  it('a second client can attach to an existing sim', () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory })
    const router = new MessageRouter(mgr)
    const a = fakeClient()
    router.handleRaw(a.client, JSON.stringify({ type: 'CreateSimulation', scenario: { vessels: [] } }))
    const simId = a.client.simId!
    const b = fakeClient()
    router.handleRaw(b.client, JSON.stringify({ type: 'AttachSimulation', simId }))
    expect(b.client.simId).toBe(simId)
    expect(b.sent.some((m) => m.type === 'Error')).toBe(false)
  })

  it('handleClose detaches the client from its sim', async () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory, orphanTtlMs: 50 })
    const router = new MessageRouter(mgr)
    const { client } = fakeClient()
    router.handleRaw(client, JSON.stringify({ type: 'CreateSimulation', scenario: { vessels: [] } }))
    expect(mgr.size).toBe(1)
    router.handleClose(client)
    await delay(110)
    expect(mgr.size).toBe(0) // detached → TTL expired → torn down
  })
})
