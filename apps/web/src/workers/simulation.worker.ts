/// <reference lib="webworker" />
import type { SimulationStateDTO, WorkerMessageDTO } from '@ymir/types'
import type { VesselConfigDTO } from '@ymir/types'
import createMockModule from './mock-wasm.js'

const API_BASE = (self as unknown as { VITE_API_URL?: string }).VITE_API_URL
  ?? 'http://localhost:3000'

// eslint-disable-next-line @typescript-eslint/no-explicit-any
let simulation: any = null
let loopHandle: ReturnType<typeof setInterval> | null = null

function post(msg: WorkerMessageDTO) {
  self.postMessage(msg)
}

async function fetchVesselConfig(id: number): Promise<VesselConfigDTO | null> {
  try {
    const res = await fetch(`${API_BASE}/vessels/${id}/config`)
    if (!res.ok) return null
    return res.json() as Promise<VesselConfigDTO>
  } catch {
    return null
  }
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

  const config = await fetchVesselConfig(1)
  if (config) {
    self.postMessage({ type: 'vessel_config', payload: config })
    // WASM mock ignores physics params; real Embind module will accept them via loadConfig().
    if (typeof simulation.loadConfig === 'function') {
      simulation.loadConfig(JSON.stringify(config))
    }
    simulation.addVessel(1)
    post({ type: 'ready' })
  } else {
    // Config not available (API down or vessel not seeded) — still start with default vessel
    simulation.addVessel(1)
    post({ type: 'ready' })
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
