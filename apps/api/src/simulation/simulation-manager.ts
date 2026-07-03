import { Worker } from 'node:worker_threads'
import { randomUUID } from 'node:crypto'
import type { WorkerCommand, WorkerEvent } from './worker-protocol.js'

export type SimId = string

/** Abstract handle over a simulation worker (real worker_thread or a test double). */
export interface SimWorkerHandle {
  postMessage(cmd: WorkerCommand): void
  on(event: 'message', listener: (event: WorkerEvent) => void): void
  terminate(): void | Promise<void>
}

export type WorkerFactory = () => SimWorkerHandle

/** A connected client that receives a simulation's events (e.g. a WebSocket). */
export interface Connection {
  send(event: WorkerEvent): void
}

export interface SimulationManagerOptions {
  /** Spawns a worker; defaults to a real worker_thread from the entry file. */
  workerFactory?: WorkerFactory
  /** Max concurrent simulations (default 2). */
  maxSimulations?: number
  /** Grace period after the last client leaves before the sim is torn down (default 30 min). */
  orphanTtlMs?: number
  /** Sim id generator (default crypto.randomUUID — non-guessable). */
  generateId?: () => SimId
}

interface SimEntry {
  handle: SimWorkerHandle
  connections: Set<Connection>
  ttlTimer: ReturnType<typeof setTimeout> | null
}

const DEFAULT_MAX = 2
const DEFAULT_TTL_MS = 30 * 60 * 1000

function defaultWorkerFactory(): SimWorkerHandle {
  // dist runtime: simulation-worker.entry.js sits next to this file; in dev,
  // tsx maps the .js specifier to the .ts source.
  const worker = new Worker(new URL('./simulation-worker.entry.js', import.meta.url))
  return {
    postMessage: (cmd) => worker.postMessage(cmd),
    on: (event, listener) => worker.on(event, listener),
    terminate: async () => { await worker.terminate() },
  }
}

/**
 * Owns the lifecycle of every live simulation: spawns one worker per sim
 * (ADR-002), maps sim_id → worker + connections, fans out worker events to the
 * connected clients, and tears down orphaned sims after a TTL (ADR-006).
 * Runs on the Fastify main thread — it never executes physics itself.
 */
export class SimulationManager {
  private readonly entries = new Map<SimId, SimEntry>()
  private readonly workerFactory: WorkerFactory
  private readonly maxSimulations: number
  private readonly orphanTtlMs: number
  private readonly generateId: () => SimId

  constructor(opts: SimulationManagerOptions = {}) {
    this.workerFactory = opts.workerFactory ?? defaultWorkerFactory
    this.maxSimulations = opts.maxSimulations ?? DEFAULT_MAX
    this.orphanTtlMs = opts.orphanTtlMs ?? DEFAULT_TTL_MS
    this.generateId = opts.generateId ?? randomUUID
  }

  /** Number of live simulations. */
  get size(): number {
    return this.entries.size
  }

  /** Create a new simulation and return its id. Throws when at capacity. */
  create(): SimId {
    if (this.entries.size >= this.maxSimulations) {
      throw new Error(`simulation limit reached (${this.maxSimulations})`)
    }
    const simId = this.generateId()
    const handle = this.workerFactory()
    const entry: SimEntry = { handle, connections: new Set(), ttlTimer: null }
    handle.on('message', (event) => this.fanOut(simId, event))
    this.entries.set(simId, entry)
    return simId
  }

  /** Attach a client to a sim. Returns false if the sim does not exist. */
  attach(simId: SimId, conn: Connection): boolean {
    const entry = this.entries.get(simId)
    if (!entry) return false
    entry.connections.add(conn)
    if (entry.ttlTimer) {
      clearTimeout(entry.ttlTimer)
      entry.ttlTimer = null
    }
    return true
  }

  /** Detach a client. When the last client leaves, arm the orphan TTL. */
  detach(simId: SimId, conn: Connection): void {
    const entry = this.entries.get(simId)
    if (!entry) return
    entry.connections.delete(conn)
    if (entry.connections.size === 0 && entry.ttlTimer === null) {
      entry.ttlTimer = setTimeout(() => void this.stop(simId), this.orphanTtlMs)
    }
  }

  /** Forward a command to a sim's worker. No-op if the sim does not exist. */
  command(simId: SimId, cmd: WorkerCommand): void {
    this.entries.get(simId)?.handle.postMessage(cmd)
  }

  /** Stop a sim immediately and free its worker. Idempotent. */
  async stop(simId: SimId): Promise<void> {
    const entry = this.entries.get(simId)
    if (!entry) return
    this.entries.delete(simId)
    if (entry.ttlTimer) clearTimeout(entry.ttlTimer)
    await entry.handle.terminate()
  }

  private fanOut(simId: SimId, event: WorkerEvent): void {
    const entry = this.entries.get(simId)
    if (!entry) return
    for (const conn of entry.connections) conn.send(event)
  }
}
