import { describe, it, expect, afterEach, vi } from 'vitest'
import { MessageChannel, Worker } from 'node:worker_threads'
import { SimulationManager, type SimWorkerHandle, type Connection } from './simulation-manager.js'
import { SimulationRunner } from './simulation-runner.js'
import { attachRunnerToPort, type MessagePortLike } from './simulation-worker.js'
import type { WorkerCommand, WorkerEvent } from './worker-protocol.js'

const disposers: Array<() => void | Promise<void>> = []
afterEach(async () => {
  for (const d of disposers.splice(0)) await d()
})

/** A worker double backed by the real engine over a MessageChannel (no thread, no mock). */
function realEngineWorkerFactory(): SimWorkerHandle {
  const { port1, port2 } = new MessageChannel()
  let runner: SimulationRunner | null = null
  void SimulationRunner.create().then((r) => {
    runner = r
    attachRunnerToPort(port1 as unknown as MessagePortLike, r)
  })
  disposers.push(() => { runner?.dispose(); port1.close(); port2.close() })
  return {
    postMessage: (cmd: WorkerCommand) => port2.postMessage(cmd),
    on: (_e, listener) => port2.on('message', listener as (v: unknown) => void),
    terminate: () => { runner?.dispose(); port1.close(); port2.close() },
  }
}

function collector(): Connection & { events: WorkerEvent[] } {
  const events: WorkerEvent[] = []
  return { events, send: (e) => events.push(e) }
}

const delay = (ms: number) => new Promise((r) => setTimeout(r, ms))

describe('SimulationManager', () => {
  it('creates simulations up to the limit, then rejects', () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory, maxSimulations: 2 })
    const a = mgr.create()
    const b = mgr.create()
    expect(a).not.toBe(b)
    expect(mgr.size).toBe(2)
    expect(() => mgr.create()).toThrow(/limit reached/)
  })

  it('attach to an unknown sim returns false', () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory })
    expect(mgr.attach('does-not-exist', collector())).toBe(false)
  })

  it('fans out worker events to all connected clients', async () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory })
    const id = mgr.create()
    const c1 = collector()
    const c2 = collector()
    expect(mgr.attach(id, c1)).toBe(true)
    expect(mgr.attach(id, c2)).toBe(true)

    mgr.command(id, { type: 'loadScenario', vessels: [{ instanceId: 1, vesselId: 1, name: 'v', x: 0, y: 0, headingDeg: 0 }] })
    mgr.command(id, { type: 'play' })
    await vi.waitFor(() => {
      expect(c1.events.some((e) => e.type === 'state')).toBe(true)
      expect(c2.events.some((e) => e.type === 'state')).toBe(true)
    }, { timeout: 5000, interval: 25 })
    mgr.command(id, { type: 'pause' })

    for (const c of [c1, c2]) {
      expect(c.events.some((e) => e.type === 'status' && e.status === 'running')).toBe(true)
    }
    await mgr.stop(id)
  })

  it('arms a TTL when the last client detaches and tears the sim down', async () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory, orphanTtlMs: 60 })
    const id = mgr.create()
    const c = collector()
    mgr.attach(id, c)
    expect(mgr.size).toBe(1)
    mgr.detach(id, c)
    expect(mgr.size).toBe(1) // still alive within TTL
    await delay(120)
    expect(mgr.size).toBe(0) // torn down after TTL
  })

  it('reconnecting within the TTL cancels the teardown', async () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory, orphanTtlMs: 80 })
    const id = mgr.create()
    const c1 = collector()
    mgr.attach(id, c1)
    mgr.detach(id, c1)
    await delay(30)
    expect(mgr.attach(id, collector())).toBe(true) // reconnect cancels timer
    await delay(90)
    expect(mgr.size).toBe(1) // survived past the original TTL
    await mgr.stop(id)
  })

  it('stop is idempotent', async () => {
    const mgr = new SimulationManager({ workerFactory: realEngineWorkerFactory })
    const id = mgr.create()
    await mgr.stop(id)
    await expect(mgr.stop(id)).resolves.toBeUndefined()
    expect(mgr.size).toBe(0)
  })
})

describe('integration — real worker_thread', () => {
  it('spawns the entry file in a real thread, runs the engine, streams state', async () => {
    // Boot a real worker_thread that registers the tsx ESM loader, then imports
    // the .ts entrypoint — proving the entry boots the engine on a live thread.
    const entryUrl = new URL('./simulation-worker.entry.ts', import.meta.url).href
    const boot = `const { register } = require('tsx/esm/api'); register(); import(${JSON.stringify(entryUrl)});`
    const worker = new Worker(boot, { eval: true })
    const events: WorkerEvent[] = []
    const done = new Promise<void>((resolve, reject) => {
      const timer = setTimeout(() => reject(new Error('timeout')), 20000)
      worker.on('message', (e: WorkerEvent) => {
        events.push(e)
        if (e.type === 'state') {
          clearTimeout(timer)
          resolve()
        }
      })
      worker.on('error', (err) => { clearTimeout(timer); reject(err) })
    })
    worker.postMessage({ type: 'loadScenario', vessels: [{ instanceId: 1, vesselId: 1, name: 'v', x: 0, y: 0, headingDeg: 0 }] } satisfies WorkerCommand)
    worker.postMessage({ type: 'play' } satisfies WorkerCommand)
    await done
    await worker.terminate()
    expect(events.some((e) => e.type === 'ready')).toBe(true)
    expect(events.some((e) => e.type === 'state')).toBe(true)
  })
})
