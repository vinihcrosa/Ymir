import { fileURLToPath } from 'node:url'
import type { SimulationStateDTO } from '@ymir/types'

/**
 * Facade over the Ymir naval physics engine (C++ compiled to WASM via Embind).
 * Mirrors the `YmirSimulation` class exposed in `core/src/wasm/YmirBindings.cpp`.
 * Every method maps 1:1 to a binding so the server-side worker uses the exact
 * same engine the browser used to run locally.
 */
export interface SimulationEngine {
  /** Add a vessel at the origin, heading 0. */
  addVessel(id: number): void
  /** Add a vessel at world position (x, y) with heading psi (radians). */
  addVesselAt(id: number, x: number, y: number, psi: number): void
  /** Set rudder angle in degrees (positive = starboard). */
  setRudderAngle(vesselId: number, rudderId: number, angleDeg: number): void
  /** Set thruster command: power 0–100% of nominal RPM, azimuth in degrees. */
  setThrusterCommand(vesselId: number, thrusterId: number, powerPct: number, azimuthDeg: number): void
  /** Advance the simulation by dt seconds. */
  step(dt: number): void
  /** Current state: simulation time and per-vessel 6-DOF state. */
  getState(): SimulationStateDTO
  /** Load the environment timeline from a serialized EnvironmentProfileDTO JSON string. */
  loadEnvironment(json: string): void
  /** Apply a sea state to every vessel's wave model. spectrum: 0=JONSWAP, 1=PIERSON, 2=REGULAR. */
  setWaveConditions(hs: number, tp: number, dirDeg: number, gamma: number, spectrum: number): void
  /** Reset time and all vessel state to initial. */
  reset(): void
  /** Free the underlying Embind heap object. Call when the engine is discarded. */
  delete(): void
}

// The Emscripten-generated module (runtime/ymir.js) is produced by
// `pnpm build:wasm` and is not committed. It is an ES6 module (MODULARIZE +
// EXPORT_ES6, ENVIRONMENT=node) whose default export is an async factory.
interface EmscriptenModule {
  YmirSimulation: new () => SimulationEngine
}
type YmirModuleFactory = () => Promise<EmscriptenModule>

let modulePromise: Promise<EmscriptenModule> | null = null

async function loadModule(): Promise<EmscriptenModule> {
  if (modulePromise) return modulePromise
  // Resolve runtime/ymir.js relative to this file (works from src/ and dist/).
  const runtimeUrl = new URL('../runtime/ymir.js', import.meta.url)
  modulePromise = import(fileURLToPath(runtimeUrl))
    .then((m: { default: YmirModuleFactory }) => m.default())
  return modulePromise
}

/**
 * Load the WASM engine and instantiate a fresh, isolated simulation.
 * Each call returns an independent engine — no shared global state.
 * @throws if the WASM runtime has not been built (`pnpm build:wasm`).
 */
export async function createSimulationEngine(): Promise<SimulationEngine> {
  const mod = await loadModule()
  return new mod.YmirSimulation()
}
