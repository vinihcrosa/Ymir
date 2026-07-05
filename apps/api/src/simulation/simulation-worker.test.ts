import { describe, it, expect, afterEach } from 'vitest'
import { MessageChannel, type MessagePort } from 'node:worker_threads'
import { attachRunnerToPort, type MessagePortLike } from './simulation-worker.js'
import { SimulationRunner } from './simulation-runner.js'
import type { WorkerCommand, WorkerEvent } from './worker-protocol.js'

// Bridge is exercised over a real MessageChannel with the real engine (ADR-005).
// The error-path test injects a throwing runner stub (runner logic, not engine).

const cleanups: Array<() => void> = []
afterEach(() => {
  for (const c of cleanups.splice(0)) c()
})

/** Resolve with the first event matching `pred`, or reject on timeout. */
function nextEvent(port: MessagePort, pred: (e: WorkerEvent) => boolean, ms = 5000): Promise<WorkerEvent> {
  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error('timeout waiting for event')), ms)
    port.on('message', (e: WorkerEvent) => {
      if (pred(e)) {
        clearTimeout(timer)
        resolve(e)
      }
    })
  })
}

function send(port: MessagePort, cmd: WorkerCommand): void {
  port.postMessage(cmd)
}

describe('attachRunnerToPort', () => {
  it('runs a scenario: play emits state, pause emits paused status', async () => {
    const runner = await SimulationRunner.create()
    const { port1, port2 } = new MessageChannel()
    cleanups.push(() => { runner.dispose(); port1.close(); port2.close() })
    attachRunnerToPort(port1 as unknown as MessagePortLike, runner)

    send(port2, { type: 'loadScenario', vessels: [{ instanceId: 1, vesselId: 1, name: 'v', x: 0, y: 0, headingDeg: 0 }] })
    send(port2, { type: 'setActuator', vesselId: 1, deviceType: 'thruster', deviceId: 0, value: 100, value2: 0 })
    send(port2, { type: 'play' })

    const running = await nextEvent(port2, (e) => e.type === 'status' && e.status === 'running')
    expect(running.type).toBe('status')
    const state = await nextEvent(port2, (e) => e.type === 'state')
    expect(state.type).toBe('state')
    if (state.type === 'state') {
      expect(state.payload.vessels).toHaveLength(1)
    }

    send(port2, { type: 'pause' })
    const paused = await nextEvent(port2, (e) => e.type === 'status' && e.status === 'paused')
    expect(paused).toBeTruthy()
  })

  it('forwards reset without emitting an error', async () => {
    const runner = await SimulationRunner.create()
    const { port1, port2 } = new MessageChannel()
    cleanups.push(() => { runner.dispose(); port1.close(); port2.close() })
    attachRunnerToPort(port1 as unknown as MessagePortLike, runner)

    send(port2, { type: 'loadScenario', vessels: [{ instanceId: 1, vesselId: 1, name: 'v', x: 0, y: 0, headingDeg: 0 }] })
    send(port2, { type: 'loadEnvironment', json: '{}' })
    send(port2, { type: 'reset' })

    let errored = false
    port2.on('message', (e: WorkerEvent) => { if (e.type === 'error') errored = true })
    await new Promise((r) => setTimeout(r, 200))
    expect(errored).toBe(false)
  })

  it('emits an error event when a command handler throws (Error and non-Error)', async () => {
    // Runner stub whose method throws — exercises the bridge catch branch.
    const throwingRunner = {
      loadScenario: () => { throw new Error('boom') },
      reset: () => { throw 'plain-string' },
    } as unknown as SimulationRunner
    const { port1, port2 } = new MessageChannel()
    cleanups.push(() => { port1.close(); port2.close() })
    attachRunnerToPort(port1 as unknown as MessagePortLike, throwingRunner)

    send(port2, { type: 'loadScenario', vessels: [] })
    const e1 = await nextEvent(port2, (e) => e.type === 'error')
    expect(e1.type === 'error' && e1.message).toBe('boom')

    send(port2, { type: 'reset' })
    const e2 = await nextEvent(port2, (e) => e.type === 'error' && e.message === 'plain-string')
    expect(e2).toBeTruthy()
  })
})
