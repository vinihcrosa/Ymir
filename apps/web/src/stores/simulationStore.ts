import { create } from 'zustand'
import type { SimulationStateDTO } from '@ymir/types'

type Status = 'idle' | 'loading' | 'ready' | 'running' | 'error'

interface SimulationStore {
  status: Status
  error: string | null
  state: SimulationStateDTO | null
  worker: Worker | null
  start: (dt?: number) => void
  stop: () => void
  reset: () => void
}

export const useSimulationStore = create<SimulationStore>((set, get) => ({
  status: 'idle',
  error: null,
  state: null,
  worker: null,

  start(dt = 0.05) {
    let { worker } = get()

    if (!worker) {
      set({ status: 'loading', error: null })
      worker = new Worker(
        new URL('../workers/simulation.worker.ts', import.meta.url),
        { type: 'module' },
      )

      worker.onmessage = (e: MessageEvent) => {
        const msg = e.data as { type: string; payload?: SimulationStateDTO; message?: string }
        switch (msg.type) {
          case 'ready':
            set({ status: 'ready' })
            // Auto-start once ready
            get().worker!.postMessage({ type: 'start', dt })
            set({ status: 'running' })
            break
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
      worker.postMessage({ type: 'start', dt })
      set({ status: 'running' })
    }
  },

  stop() {
    const { worker } = get()
    worker?.postMessage({ type: 'stop' })
    set({ status: 'ready' })
  },

  reset() {
    const { worker } = get()
    if (worker) {
      worker.terminate()
    }
    set({ status: 'idle', error: null, state: null, worker: null })
  },
}))
