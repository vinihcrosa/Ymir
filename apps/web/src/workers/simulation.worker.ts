/// <reference lib="webworker" />
import type { SimulationStateDTO, WorkerMessageDTO } from '@ymir/types'
import createMockModule from './mock-wasm.js'

// eslint-disable-next-line @typescript-eslint/no-explicit-any
let simulation: any = null
let loopHandle: ReturnType<typeof setInterval> | null = null

function post(msg: WorkerMessageDTO) {
  self.postMessage(msg)
}

async function initWasm() {
  // Try real WASM first; fall back to JS mock when ymir.js is not built yet.
  try {
    // eslint-disable-next-line @typescript-eslint/no-implied-eval, @typescript-eslint/no-explicit-any
    const { default: createYmirModule } = await (new Function('u', 'return import(u)'))('/wasm/ymir.js') as any
    const Module = await createYmirModule()
    simulation = new Module.YmirSimulation()
  } catch {
    const Module = await createMockModule()
    simulation = new Module.YmirSimulation()
  }
  simulation.addVessel(1)
  post({ type: 'ready' })
}

function startLoop(dt: number) {
  if (loopHandle !== null) return
  loopHandle = setInterval(() => {
    try {
      simulation.step(dt)
      const state: SimulationStateDTO = simulation.getState()
      post({ type: 'state', payload: state })
    } catch (err) {
      post({ type: 'error', message: String(err) })
      stopLoop()
    }
  }, 50) // 20Hz
}

function stopLoop() {
  if (loopHandle !== null) {
    clearInterval(loopHandle)
    loopHandle = null
  }
}

self.addEventListener('message', (e: MessageEvent) => {
  const msg = e.data as { type: string; dt?: number }
  switch (msg.type) {
    case 'start':
      if (simulation) startLoop(msg.dt ?? 0.05)
      break
    case 'stop':
      stopLoop()
      break
  }
})

// Initialize on worker load
initWasm()
