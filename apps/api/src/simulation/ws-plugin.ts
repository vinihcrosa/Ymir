import type { FastifyPluginAsync } from 'fastify'
import websocket from '@fastify/websocket'
import { SimulationManager } from './simulation-manager.js'
import { MessageRouter, WsClient } from './message-router.js'

export interface SimulationWsOptions {
  /** Inject a manager (tests); defaults to a manager spawning real worker_threads. */
  manager?: SimulationManager
}

/**
 * Register the real-time simulation channel: a `GET /ws` WebSocket endpoint that
 * routes ClientMessages to the SimulationManager and streams ServerMessages back.
 * REST routes on the same Fastify instance are unaffected.
 */
export const simulationWsPlugin: FastifyPluginAsync<SimulationWsOptions> = async (app, opts) => {
  const manager = opts.manager ?? new SimulationManager()
  const router = new MessageRouter(manager)

  await app.register(websocket)

  app.get('/ws', { websocket: true }, (connection) => {
    // @fastify/websocket v8 passes a SocketStream whose `.socket` is the ws.
    const socket = (connection as { socket?: WsLike }).socket ?? (connection as unknown as WsLike)
    const client = new WsClient({ send: (data) => socket.send(data) })
    socket.on('message', (raw: unknown) => router.handleRaw(client, String(raw)))
    socket.on('close', () => router.handleClose(client))
  })
}

interface WsLike {
  send(data: string): void
  on(event: 'message' | 'close', listener: (arg: unknown) => void): void
}
