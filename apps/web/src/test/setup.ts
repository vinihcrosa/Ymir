import '@testing-library/jest-dom'
import { vi } from 'vitest'

// jsdom has no usable WebSocket for our client. Stub a no-op implementation so
// components that open a SimulationSocket in tests don't hit the network.
// Tests that need to drive frames inject their own WebSocketImpl into
// SimulationSocket instead of relying on this global.
class StubWebSocket {
  static CONNECTING = 0
  static OPEN = 1
  static CLOSING = 2
  static CLOSED = 3
  readyState = StubWebSocket.CONNECTING
  onopen: (() => void) | null = null
  onmessage: ((e: { data: string }) => void) | null = null
  onclose: (() => void) | null = null
  onerror: (() => void) | null = null
  constructor(public url: string) {}
  send(): void {}
  close(): void {}
}

vi.stubGlobal('WebSocket', StubWebSocket)
