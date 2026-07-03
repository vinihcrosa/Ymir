import type { ClientMessage, ServerMessage } from '@ymir/types'

export type ConnectionState = 'connecting' | 'open' | 'reconnecting' | 'closed'

type MessageListener = (msg: ServerMessage) => void
type ConnectionListener = (state: ConnectionState) => void

interface StorageLike {
  getItem(key: string): string | null
  setItem(key: string, value: string): void
  removeItem(key: string): void
}

export interface SimulationSocketOptions {
  url?: string
  /** WebSocket implementation (defaults to the global). Injectable for tests. */
  WebSocketImpl?: typeof WebSocket
  /** Persists the sim id across reloads (defaults to localStorage). */
  storage?: StorageLike
  /** Delay before an automatic reconnect attempt. */
  reconnectDelayMs?: number
}

const SIM_ID_KEY = 'ymir.simId'
const DEFAULT_URL =
  (import.meta.env?.VITE_SIM_WS_URL as string | undefined) ?? 'ws://localhost:3000/ws'
const DEFAULT_RECONNECT_MS = 1000

/**
 * Client transport to the server-side simulation. Owns the WebSocket lifecycle
 * (connect, auto-reconnect), (de)serializes ClientMessage/ServerMessage, and
 * persists the sim_id so a reopened client re-attaches to its running sim.
 */
export class SimulationSocket {
  private ws: WebSocket | null = null
  private readonly url: string
  private readonly WebSocketImpl: typeof WebSocket
  private readonly storage: StorageLike
  private readonly reconnectDelayMs: number
  private readonly messageListeners = new Set<MessageListener>()
  private readonly connectionListeners = new Set<ConnectionListener>()
  private queue: ClientMessage[] = []
  private intentionalClose = false
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null

  constructor(opts: SimulationSocketOptions = {}) {
    this.url = opts.url ?? DEFAULT_URL
    this.WebSocketImpl = opts.WebSocketImpl ?? globalThis.WebSocket
    this.storage = opts.storage ?? globalThis.localStorage
    this.reconnectDelayMs = opts.reconnectDelayMs ?? DEFAULT_RECONNECT_MS
  }

  get simId(): string | null {
    return this.storage.getItem(SIM_ID_KEY)
  }

  onMessage(listener: MessageListener): () => void {
    this.messageListeners.add(listener)
    return () => this.messageListeners.delete(listener)
  }

  onConnectionChange(listener: ConnectionListener): () => void {
    this.connectionListeners.add(listener)
    return () => this.connectionListeners.delete(listener)
  }

  /** Open the connection. On (re)open, auto-attaches to a stored sim id. */
  connect(): void {
    this.intentionalClose = false
    this.emitConnection('connecting')
    const ws = new this.WebSocketImpl(this.url)
    this.ws = ws
    ws.onopen = () => {
      this.emitConnection('open')
      const stored = this.simId
      if (stored) this.rawSend({ type: 'AttachSimulation', simId: stored })
      this.flushQueue()
    }
    ws.onmessage = (ev: MessageEvent) => this.handleMessage(String(ev.data))
    ws.onclose = () => {
      this.ws = null
      if (this.intentionalClose) {
        this.emitConnection('closed')
      } else {
        this.emitConnection('reconnecting')
        this.reconnectTimer = setTimeout(() => this.connect(), this.reconnectDelayMs)
      }
    }
  }

  /** Create a new simulation; the sim id is persisted when the server confirms it. */
  createSimulation(scenario: Extract<ClientMessage, { type: 'CreateSimulation' }>['scenario']): void {
    this.send({ type: 'CreateSimulation', scenario })
  }

  /** Send a command, queuing it until the socket is open. */
  send(msg: ClientMessage): void {
    if (this.ws && this.ws.readyState === this.WebSocketImpl.OPEN) {
      this.rawSend(msg)
    } else {
      this.queue.push(msg)
    }
  }

  /** Close intentionally — no reconnect. */
  close(): void {
    this.intentionalClose = true
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer)
      this.reconnectTimer = null
    }
    this.ws?.close()
    this.ws = null
  }

  /** Forget the persisted sim id (e.g. the sim no longer exists server-side). */
  clearSimId(): void {
    this.storage.removeItem(SIM_ID_KEY)
  }

  private handleMessage(raw: string): void {
    let msg: ServerMessage
    try {
      msg = JSON.parse(raw) as ServerMessage
    } catch {
      return
    }
    if (msg.type === 'SimulationCreated') {
      this.storage.setItem(SIM_ID_KEY, msg.simId)
    } else if (msg.type === 'Error' && msg.message === 'unknown simulation') {
      this.clearSimId()
    }
    for (const l of this.messageListeners) l(msg)
  }

  private rawSend(msg: ClientMessage): void {
    this.ws?.send(JSON.stringify(msg))
  }

  private flushQueue(): void {
    const pending = this.queue
    this.queue = []
    for (const msg of pending) this.rawSend(msg)
  }

  private emitConnection(state: ConnectionState): void {
    for (const l of this.connectionListeners) l(state)
  }
}
