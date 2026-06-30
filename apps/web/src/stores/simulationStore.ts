import { create } from 'zustand'
import type { SimulationStateDTO, SimulationEngine } from '@ymir/types'
import { useVesselPanelStore } from './vesselPanelStore'
import { useEnvironmentStore } from './environmentStore'

type Status = 'idle' | 'loading' | 'running' | 'paused' | 'error'

interface ScenarioDraftVessel {
  instanceId: number
  vesselId: number
  name: string
  x: number
  y: number
  headingDeg: number
}

interface SimulationStore {
  status: Status
  error: string | null
  state: SimulationStateDTO | null
  engine: SimulationEngine | null
  worker: Worker | null
  scenarioVessels: ScenarioDraftVessel[]
  /** Start (first time) or resume (after pause) the simulation loop. */
  play: (dt?: number) => void
  /** Freeze the loop at the current state — does NOT reset position/time. */
  pause: () => void
  reset: () => void
  loadScenario: (vessels: ScenarioDraftVessel[]) => void
  /** Push the current environment profile to a running/paused worker. */
  applyEnvironment: () => void
}

export const useSimulationStore = create<SimulationStore>((set, get) => ({
  status: 'idle',
  error: null,
  state: null,
  engine: null,
  worker: null,
  scenarioVessels: [],

  play(dt = 0.05) {
    let { worker } = get()

    if (!worker) {
      set({ status: 'loading', error: null })
      worker = new Worker(
        new URL('../workers/simulation.worker.ts', import.meta.url),
        { type: 'module' },
      )

      worker.onmessage = (e: MessageEvent) => {
        const msg = e.data as { type: string; payload?: SimulationStateDTO; message?: string; engine?: SimulationEngine }
        switch (msg.type) {
          case 'ready': {
            // Worker booted; engine known. We go straight to running below.
            set({ engine: msg.engine ?? null })
            const { scenarioVessels } = get()
            if (scenarioVessels.length > 0) {
              get().worker!.postMessage({ type: 'loadScenario', vessels: scenarioVessels })
            }
            // Re-sync any actuator state that was set before the worker was created.
            const panel = useVesselPanelStore.getState()
            if (panel.selectedVesselId !== null) {
              const vesselId = panel.selectedVesselId
              const w = get().worker!
              Object.entries(panel.thrusterPowers).forEach(([id, pct]) => {
                const thrusterId = Number(id)
                w.postMessage({
                  type: 'setActuator', vesselId, deviceType: 'thruster',
                  deviceId: thrusterId, value: pct,
                  value2: panel.thrusterAzimuths[thrusterId] ?? 0,
                })
              })
              Object.entries(panel.rudderAngles).forEach(([id, deg]) => {
                w.postMessage({
                  type: 'setActuator', vesselId, deviceType: 'rudder',
                  deviceId: Number(id), value: deg,
                })
              })
            }
            const envStore = useEnvironmentStore.getState()
            if (envStore.hasConditions()) {
              get().worker!.postMessage({ type: 'loadEnvironment', json: envStore.toJson() })
            }
            get().worker!.postMessage({ type: 'start', dt })
            set({ status: 'running' })
            break
          }
          case 'state':
            set({ state: msg.payload ?? null })
            break
          case 'error':
            set({ status: 'error', error: msg.message ?? 'Unknown error' })
            break
        }
      }

      worker.onerror = (e) => {
        set({ status: 'error', error: e.message })
      }

      set({ worker })
    } else {
      // Resume from the frozen state — the worker keeps the simulation object,
      // so position and elapsed time carry over.
      worker.postMessage({ type: 'start', dt })
      set({ status: 'running' })
    }
  },

  pause() {
    const { worker } = get()
    worker?.postMessage({ type: 'stop' })
    set({ status: 'paused' })
  },

  applyEnvironment() {
    const { worker } = get()
    const envStore = useEnvironmentStore.getState()
    if (worker && envStore.hasConditions()) {
      worker.postMessage({ type: 'loadEnvironment', json: envStore.toJson() })
    }
  },

  reset() {
    const { worker } = get()
    if (worker) {
      worker.terminate()
    }
    set({ status: 'idle', error: null, state: null, engine: null, worker: null, scenarioVessels: [] })
  },

  loadScenario(vessels: ScenarioDraftVessel[]) {
    set({ scenarioVessels: vessels })
    const { worker } = get()
    if (worker) {
      worker.postMessage({ type: 'loadScenario', vessels })
    }
  },
}))
