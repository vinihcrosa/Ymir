import { describe, it, expect, beforeEach, vi } from 'vitest'
import { SimulationSocket } from './simulation-socket'
import type { ClientMessage, ServerMessage } from '@ymir/types'

class FakeWebSocket {
  static CONNECTING = 0
  static OPEN = 1
  static CLOSED = 3
  static instances: FakeWebSocket[] = []
  readyState = FakeWebSocket.CONNECTING
  onopen: (() => void) | null = null
  onmessage: ((e: { data: string }) => void) | null = null
  onclose: (() => void) | null = null
  sent: string[] = []
  constructor(public url: string) { FakeWebSocket.instances.push(this) }
  send(data: string) { this.sent.push(data) }
  close() { this.readyState = FakeWebSocket.CLOSED; this.onclose?.() }
  // --- test drivers ---
  open() { this.readyState = FakeWebSocket.OPEN; this.onopen?.() }
  emit(msg: ServerMessage) { this.onmessage?.({ data: JSON.stringify(msg) }) }
  serverClose() { this.readyState = FakeWebSocket.CLOSED; this.onclose?.() }
  get sentMessages(): ClientMessage[] { return this.sent.map((s) => JSON.parse(s) as ClientMessage) }
}

function makeStorage() {
  const m = new Map<string, string>()
  return {
    getItem: (k: string) => m.get(k) ?? null,
    setItem: (k: string, v: string) => { m.set(k, v) },
    removeItem: (k: string) => { m.delete(k) },
    map: m,
  }
}

function makeSocket(storage = makeStorage()) {
  const socket = new SimulationSocket({
    url: 'ws://test/ws',
    WebSocketImpl: FakeWebSocket as unknown as typeof WebSocket,
    storage,
    reconnectDelayMs: 50,
  })
  return { socket, storage }
}

const last = () => FakeWebSocket.instances[FakeWebSocket.instances.length - 1]

beforeEach(() => { FakeWebSocket.instances = [] })

describe('SimulationSocket', () => {
  it('persists sim id on SimulationCreated', () => {
    const { socket, storage } = makeSocket()
    socket.connect()
    last().open()
    last().emit({ type: 'SimulationCreated', simId: 'sim-42' })
    expect(storage.getItem('ymir.simId')).toBe('sim-42')
    expect(socket.simId).toBe('sim-42')
  })

  it('auto-attaches to a stored sim id when the socket opens', () => {
    const storage = makeStorage()
    storage.setItem('ymir.simId', 'sim-7')
    const { socket } = makeSocket(storage)
    socket.connect()
    last().open()
    expect(last().sentMessages).toContainEqual({ type: 'AttachSimulation', simId: 'sim-7' })
  })

  it('delivers State messages to listeners', () => {
    const { socket } = makeSocket()
    const seen: ServerMessage[] = []
    socket.onMessage((m) => seen.push(m))
    socket.connect()
    last().open()
    const state: ServerMessage = { type: 'State', payload: { t: 1, vessels: [] } }
    last().emit(state)
    expect(seen).toContainEqual(state)
  })

  it('queues commands sent before open and flushes them on open', () => {
    const { socket } = makeSocket()
    socket.connect()
    socket.send({ type: 'Play', simId: 'x' })
    expect(last().sent).toHaveLength(0) // not open yet
    last().open()
    expect(last().sentMessages).toContainEqual({ type: 'Play', simId: 'x' })
  })

  it('clears the stored sim id when the server reports unknown simulation', () => {
    const storage = makeStorage()
    storage.setItem('ymir.simId', 'gone')
    const { socket } = makeSocket(storage)
    socket.connect()
    last().open()
    last().emit({ type: 'Error', message: 'unknown simulation' })
    expect(storage.getItem('ymir.simId')).toBeNull()
  })

  it('reconnects after an unexpected close and re-attaches', () => {
    vi.useFakeTimers()
    try {
      const storage = makeStorage()
      const { socket } = makeSocket(storage)
      const states: string[] = []
      socket.onConnectionChange((s) => states.push(s))
      socket.connect()
      last().open()
      last().emit({ type: 'SimulationCreated', simId: 'sim-9' })
      const firstWs = last()
      firstWs.serverClose() // unexpected drop
      expect(states).toContain('reconnecting')
      vi.advanceTimersByTime(60) // past reconnectDelayMs
      expect(FakeWebSocket.instances.length).toBe(2) // reconnected
      last().open()
      expect(last().sentMessages).toContainEqual({ type: 'AttachSimulation', simId: 'sim-9' })
    } finally {
      vi.useRealTimers()
    }
  })

  it('does not reconnect after an intentional close', () => {
    vi.useFakeTimers()
    try {
      const { socket } = makeSocket()
      const states: string[] = []
      socket.onConnectionChange((s) => states.push(s))
      socket.connect()
      last().open()
      socket.close()
      expect(states).toContain('closed')
      vi.advanceTimersByTime(200)
      expect(FakeWebSocket.instances.length).toBe(1) // no reconnect
    } finally {
      vi.useRealTimers()
    }
  })

  it('createSimulation sends a CreateSimulation frame', () => {
    const { socket } = makeSocket()
    socket.connect()
    last().open()
    socket.createSimulation({ vessels: [] })
    expect(last().sentMessages).toContainEqual({ type: 'CreateSimulation', scenario: { vessels: [] } })
  })

  it('onMessage returns an unsubscribe function', () => {
    const { socket } = makeSocket()
    const seen: ServerMessage[] = []
    const off = socket.onMessage((m) => seen.push(m))
    socket.connect()
    last().open()
    off()
    last().emit({ type: 'Status', status: 'running' })
    expect(seen).toHaveLength(0)
  })
})
