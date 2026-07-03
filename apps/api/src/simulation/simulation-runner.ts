import { createSimulationEngine, type SimulationEngine } from '@ymir/wasm'
import type { SimulationStateDTO, ScenarioDraftVesselDTO } from '@ymir/types'

const DEG_TO_RAD = Math.PI / 180
const DEFAULT_DT = 0.05 // 20 Hz
const LOOP_INTERVAL_MS = 50

/** Maps a scenario spectrum name to the engine's integer id (mirrors YmirBindings). */
function spectrumId(spectrum: string | undefined): number {
  if (spectrum === 'REGULAR') return 2
  if (spectrum === 'PIERSON') return 1
  return 0 // JONSWAP / default
}

export interface SimulationRunnerOptions {
  /** Injectable factory (defaults to the real @ymir/wasm engine). */
  engineFactory?: () => Promise<SimulationEngine>
}

/**
 * Owns one isolated physics engine and its real-time tick loop. Applies
 * commands and produces state snapshots. Pure runtime logic — knows nothing
 * about worker_threads or WebSocket; the SimulationWorker bridges it to a port.
 */
export class SimulationRunner {
  private engine: SimulationEngine
  private readonly engineFactory: () => Promise<SimulationEngine>
  private timer: ReturnType<typeof setInterval> | null = null
  private disposed = false

  private constructor(engine: SimulationEngine, factory: () => Promise<SimulationEngine>) {
    this.engine = engine
    this.engineFactory = factory
  }

  static async create(opts: SimulationRunnerOptions = {}): Promise<SimulationRunner> {
    const factory = opts.engineFactory ?? createSimulationEngine
    const engine = await factory()
    return new SimulationRunner(engine, factory)
  }

  /**
   * Replace the current simulation with a fresh engine holding the given
   * vessels. Rebuilding from scratch is the only safe path — adding a vessel
   * after stepping is undefined behaviour in the CVODE-based engine.
   */
  async loadScenario(vessels: ScenarioDraftVesselDTO[]): Promise<void> {
    this.stop()
    this.engine.delete()
    this.engine = await this.engineFactory()
    for (const v of vessels) {
      this.engine.addVesselAt(v.instanceId, v.x, v.y, v.headingDeg * DEG_TO_RAD)
    }
  }

  setActuator(
    vesselId: number,
    deviceType: 'rudder' | 'thruster',
    deviceId: number,
    value: number,
    value2 = 0,
  ): void {
    if (deviceType === 'rudder') {
      this.engine.setRudderAngle(vesselId, deviceId, value)
    } else {
      this.engine.setThrusterCommand(vesselId, deviceId, value, value2)
    }
  }

  /**
   * Load the environment timeline and apply the first sea state to the wave
   * model. The JSON is validated here before reaching the engine — the native
   * parser aborts on malformed input, so we never hand it invalid JSON.
   */
  loadEnvironment(json: string): void {
    if (!json || json === '{}') return
    let env: {
      waveSeries?: Array<{ Hs?: number; Tp?: number; dirNaut?: number; gamma?: number; spectrum?: string }>
    }
    try {
      env = JSON.parse(json)
    } catch {
      return // malformed env — leave the sea calm, don't touch the engine
    }
    this.engine.loadEnvironment(json)
    const kf = env.waveSeries?.[0]
    if (kf) {
      this.engine.setWaveConditions(
        kf.Hs ?? 0,
        kf.Tp ?? 8,
        kf.dirNaut ?? 0,
        kf.gamma ?? 3.3,
        spectrumId(kf.spectrum),
      )
    }
  }

  /** Advance one step and return the resulting state. */
  tick(dt: number = DEFAULT_DT): SimulationStateDTO {
    this.engine.step(dt)
    return this.engine.getState()
  }

  getState(): SimulationStateDTO {
    return this.engine.getState()
  }

  reset(): void {
    this.engine.reset()
  }

  /** Whether the tick loop is currently running. */
  get running(): boolean {
    return this.timer !== null
  }

  /** Start the real-time loop; `onState` is invoked with each tick's snapshot. */
  play(dt: number = DEFAULT_DT, onState: (state: SimulationStateDTO) => void): void {
    if (this.timer !== null) return
    this.timer = setInterval(() => {
      try {
        onState(this.tick(dt))
      } catch {
        this.stop()
      }
    }, LOOP_INTERVAL_MS)
  }

  /** Freeze the loop without discarding state. */
  stop(): void {
    if (this.timer !== null) {
      clearInterval(this.timer)
      this.timer = null
    }
  }

  /** Stop the loop and free the engine. Idempotent. */
  dispose(): void {
    if (this.disposed) return
    this.disposed = true
    this.stop()
    this.engine.delete()
  }
}
