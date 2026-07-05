import { isMainThread, parentPort } from 'node:worker_threads'
import { SimulationRunner } from './simulation-runner.js'
import { attachRunnerToPort, type MessagePortLike } from './simulation-worker.js'
import type { WorkerEvent } from './worker-protocol.js'

/**
 * Worker_thread entrypoint. The SimulationManager spawns this file (one per
 * simulation). Boots a SimulationRunner and bridges it to the parent port.
 * Excluded from coverage — it requires a live worker_thread; the bridge logic
 * it wires (attachRunnerToPort) is unit-tested via MessageChannel.
 */
if (!isMainThread && parentPort) {
  const port = parentPort
  SimulationRunner.create()
    .then((runner) => {
      attachRunnerToPort(port as unknown as MessagePortLike, runner)
      port.postMessage({ type: 'ready' } satisfies WorkerEvent)
    })
    .catch((err: unknown) => {
      port.postMessage({
        type: 'error',
        message: err instanceof Error ? err.message : String(err),
      } satisfies WorkerEvent)
    })
}
