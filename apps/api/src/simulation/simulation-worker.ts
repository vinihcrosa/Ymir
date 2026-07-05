import type { SimulationStateDTO } from '@ymir/types'
import type { SimulationRunner } from './simulation-runner.js'
import type { WorkerCommand, WorkerEvent } from './worker-protocol.js'

/** Minimal port surface shared by worker_threads' parentPort and MessageChannel ports. */
export interface MessagePortLike {
  postMessage(value: unknown): void
  on(event: 'message', listener: (value: WorkerCommand) => void): void
}

/**
 * Bridge a SimulationRunner to a message port: translate inbound WorkerCommands
 * into runner calls and emit WorkerEvents (state/status/error) back. Kept free
 * of worker_threads specifics so it can be driven by a MessageChannel in tests.
 */
export function attachRunnerToPort(port: MessagePortLike, runner: SimulationRunner): void {
  const emit = (event: WorkerEvent): void => port.postMessage(event)
  const emitState = (payload: SimulationStateDTO): void => emit({ type: 'state', payload })

  port.on('message', (cmd: WorkerCommand) => {
    void handleCommand(cmd)
  })

  async function handleCommand(cmd: WorkerCommand): Promise<void> {
    try {
      switch (cmd.type) {
        case 'loadScenario':
          await runner.loadScenario(cmd.vessels)
          break
        case 'play':
          runner.play(cmd.dt, (state) => emitState(state))
          emit({ type: 'status', status: 'running' })
          break
        case 'pause':
          runner.stop()
          emit({ type: 'status', status: 'paused' })
          break
        case 'reset':
          runner.reset()
          break
        case 'setActuator':
          runner.setActuator(cmd.vesselId, cmd.deviceType, cmd.deviceId, cmd.value, cmd.value2)
          break
        case 'loadEnvironment':
          runner.loadEnvironment(cmd.json)
          break
      }
    } catch (err) {
      emit({ type: 'error', message: err instanceof Error ? err.message : String(err) })
    }
  }
}
