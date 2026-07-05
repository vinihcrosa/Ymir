import type { SimulationStateDTO, ScenarioDraftVesselDTO } from '@ymir/types'

/**
 * Internal message protocol between the main thread (SimulationManager) and a
 * SimulationWorker running in a worker_thread. Distinct from the network
 * ClientMessage/ServerMessage: these carry no sim_id (the worker IS one sim).
 */
export type WorkerCommand =
  | { type: 'loadScenario'; vessels: ScenarioDraftVesselDTO[] }
  | { type: 'play'; dt?: number }
  | { type: 'pause' }
  | { type: 'reset' }
  | {
      type: 'setActuator'
      vesselId: number
      deviceType: 'rudder' | 'thruster'
      deviceId: number
      value: number
      value2?: number
    }
  | { type: 'loadEnvironment'; json: string }

export type WorkerEvent =
  | { type: 'ready' }
  | { type: 'state'; payload: SimulationStateDTO }
  | { type: 'status'; status: 'running' | 'paused' | 'ended' }
  | { type: 'error'; message: string }
