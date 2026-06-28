/// <reference lib="webworker" />
import type { SimulationStateDTO, WorkerMessageDTO } from '@ymir/types'

// eslint-disable-next-line @typescript-eslint/no-explicit-any
let simulation: any = null
let loopHandle: ReturnType<typeof setInterval> | null = null

function post(msg: WorkerMessageDTO) {
  self.postMessage(msg)
}

async function initWasm() {
  try {
    // Dynamic import via Function constructor — avoids TS module resolution for a
    // runtime-only public/ path that has no type declarations at build time.
    // Vite will NOT bundle this import (intended: the WASM loader lives in public/).
    // eslint-disable-next-line @typescript-eslint/no-implied-eval
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    const { default: createYmirModule } = await (new Function('u', 'return import(u)'))('/wasm/ymir.js') as any as { default: () => Promise<{ YmirSimulation: new () => unknown }> }
    const Module = await createYmirModule()
    simulation = new Module.YmirSimulation()
    // Add a default vessel so there's something to simulate
    simulation.addVessel(1)
    post({ type: 'ready' })
  } catch (err) {
    post({ type: 'error', message: String(err) })
  }
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
