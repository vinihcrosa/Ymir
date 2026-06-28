/// <reference lib="webworker" />
import type { SimulationStateDTO, WorkerMessageDTO } from '@ymir/types'
import type { VesselConfigDTO } from '@ymir/types'
import createMockModule from './mock-wasm.js'

const API_BASE = (self as unknown as { VITE_API_URL?: string }).VITE_API_URL
  ?? 'http://localhost:3000'

interface ScenarioDraftVessel { vesselId: number; name: string; x: number; y: number; headingDeg: number }

// eslint-disable-next-line @typescript-eslint/no-explicit-any
let simulation: any = null
// eslint-disable-next-line @typescript-eslint/no-explicit-any
let SimulationClass: (new () => any) | null = null
let loopHandle: ReturnType<typeof setInterval> | null = null
let pendingVessels: ScenarioDraftVessel[] = []

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

function stopLoop() {
  if (loopHandle !== null) {
    clearInterval(loopHandle)
    loopHandle = null
  }
}

function applyScenarioVessels(vessels: ScenarioDraftVessel[]) {
  stopLoop()
  // addVessel after step() is undefined behaviour in the C++ engine — the CVODE
  // integrator is already initialised for existing bodies. The only safe path is
  // to reconstruct the YmirSimulation from scratch (per the C++ comment in
  // YmirBindings.cpp). Emscripten Embind objects must be .delete()d to free heap.
  if (simulation && typeof simulation.delete === 'function') simulation.delete()
  if (!SimulationClass) return
  simulation = new SimulationClass()

  for (const v of vessels) {
    simulation.addVessel(v.vesselId)
  }
}

async function initWasm() {
  // Try real WASM first; fall back to JS mock when ymir.js is not built yet.
  try {
    // eslint-disable-next-line @typescript-eslint/no-implied-eval, @typescript-eslint/no-explicit-any
    const { default: createYmirModule } = await (new Function('u', 'return import(u)'))('/wasm/ymir.js') as any
    const Module = await createYmirModule()
    SimulationClass = Module.YmirSimulation
  } catch {
    const Module = await createMockModule()
    SimulationClass = Module.YmirSimulation
  }

  simulation = new SimulationClass!()

  const config = await fetchVesselConfig(1)
  if (config) {
    self.postMessage({ type: 'vessel_config', payload: config })
  }

  if (pendingVessels.length > 0) {
    applyScenarioVessels(pendingVessels)
  } else {
    // Fallback: start with a single default vessel (ID=1)
    simulation.addVessel(1)
  }

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

self.addEventListener('message', (e: MessageEvent) => {
  const msg = e.data as { type: string; dt?: number }
  switch (msg.type) {
    case 'start':
      if (simulation) startLoop(msg.dt ?? 0.05)
      break
    case 'stop':
      stopLoop()
      break
    case 'loadScenario': {
      const vessels = (e.data as { type: string; vessels: ScenarioDraftVessel[] }).vessels
      pendingVessels = vessels
      if (simulation) {
        applyScenarioVessels(vessels)
      }
      break
    }
  }
})

// Initialize on worker load
initWasm()
