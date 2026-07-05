import { describe, it, expect, afterEach } from 'vitest'
import Fastify, { type FastifyInstance } from 'fastify'
import { MessageChannel } from 'node:worker_threads'
import { simulationWsPlugin } from './ws-plugin.js'
import { SimulationManager, type SimWorkerHandle } from './simulation-manager.js'
import { SimulationRunner } from './simulation-runner.js'
import { attachRunnerToPort, type MessagePortLike } from './simulation-worker.js'
import type { ServerMessage } from '@ymir/types'

const disposers: Array<() => void | Promise<void>> = []
afterEach(async () => { for (const d of disposers.splice(0)) await d() })

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

async function startServer(): Promise<{ app: FastifyInstance; port: number }> {
  const app = Fastify()
  app.get('/ping', async () => ({ ok: true }))
  await app.register(simulationWsPlugin, {
    manager: new SimulationManager({ workerFactory: realEngineWorkerFactory }),
  })
  await app.listen({ port: 0, host: '127.0.0.1' })
  const addr = app.server.address()
  const port = typeof addr === 'object' && addr ? addr.port : 0
  disposers.push(() => app.close())
  return { app, port }
}

/** Resolve when a message satisfying `pred` arrives. */
function waitFor(ws: WebSocket, pred: (m: ServerMessage) => boolean, ms = 8000): Promise<ServerMessage> {
  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error('timeout')), ms)
    ws.addEventListener('message', (ev) => {
      const msg = JSON.parse(String((ev as MessageEvent).data)) as ServerMessage
      if (pred(msg)) { clearTimeout(timer); resolve(msg) }
    })
  })
}

describe('simulationWsPlugin (real Fastify + WebSocket)', () => {
  it('CreateSimulation -> Play -> streams State over the socket', async () => {
    const { port } = await startServer()
    const ws = new WebSocket(`ws://127.0.0.1:${port}/ws`)
    disposers.push(() => ws.close())
    await new Promise<void>((r) => ws.addEventListener('open', () => r()))

    ws.send(JSON.stringify({ type: 'CreateSimulation', scenario: { vessels: [{ instanceId: 1, vesselId: 1, name: 'v', x: 0, y: 0, headingDeg: 0 }] } }))
    const created = await waitFor(ws, (m) => m.type === 'SimulationCreated')
    const simId = created.type === 'SimulationCreated' ? created.simId : ''
    expect(simId).toBeTruthy()

    ws.send(JSON.stringify({ type: 'Play', simId }))
    const state = await waitFor(ws, (m) => m.type === 'State')
    expect(state.type).toBe('State')
    if (state.type === 'State') expect(state.payload.vessels).toHaveLength(1)
  })

  it('rejects a malformed message with an Error frame without closing', async () => {
    const { port } = await startServer()
    const ws = new WebSocket(`ws://127.0.0.1:${port}/ws`)
    disposers.push(() => ws.close())
    await new Promise<void>((r) => ws.addEventListener('open', () => r()))

    ws.send(JSON.stringify({ type: 'Bogus' }))
    const err = await waitFor(ws, (m) => m.type === 'Error')
    expect(err.type).toBe('Error')
  })

  it('serves REST on the same instance as the WebSocket', async () => {
    const { port } = await startServer()
    const res = await fetch(`http://127.0.0.1:${port}/ping`)
    expect(res.status).toBe(200)
    expect(await res.json()).toEqual({ ok: true })
  })
})
