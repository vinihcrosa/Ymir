import { Value } from '@sinclair/typebox/value'
import { ClientMessage } from '@ymir/types'
import type { ClientMessage as ClientMessageType, ServerMessage } from '@ymir/types'
import type { SimulationManager, Connection } from './simulation-manager.js'
import type { WorkerCommand, WorkerEvent } from './worker-protocol.js'

/** Transport that can push a serialized frame to one client (e.g. a WebSocket). */
export interface ClientTransport {
  send(data: string): void
}

/** Translate an internal worker event into the outbound network ServerMessage. */
export function toServerMessage(event: WorkerEvent): ServerMessage | null {
  switch (event.type) {
    case 'state':
      return { type: 'State', payload: event.payload }
    case 'status':
      return { type: 'Status', status: event.status }
    case 'error':
      return { type: 'Error', message: event.message }
    case 'ready':
      return null // internal readiness signal — nothing to forward
  }
}

/** Map a validated network command to the worker command (drops sim_id). */
function toWorkerCommand(msg: ClientMessageType): WorkerCommand | null {
  switch (msg.type) {
    case 'Play':
      return { type: 'play', dt: msg.dt }
    case 'Pause':
      return { type: 'pause' }
    case 'Reset':
      return { type: 'reset' }
    case 'LoadScenario':
      return { type: 'loadScenario', vessels: msg.vessels }
    case 'SetActuator':
      return {
        type: 'setActuator',
        vesselId: msg.vesselId,
        deviceType: msg.deviceType,
        deviceId: msg.deviceId,
        value: msg.value,
        value2: msg.value2,
      }
    case 'LoadEnvironment':
      return { type: 'loadEnvironment', json: msg.json }
    default:
      return null // create/attach are handled separately, not forwarded
  }
}

/** One connected client: bridges manager events (WorkerEvent) to ServerMessage frames. */
export class WsClient implements Connection {
  simId: string | null = null
  constructor(private readonly transport: ClientTransport) {}

  /** Connection contract — receives worker events fanned out by the manager. */
  send(event: WorkerEvent): void {
    const msg = toServerMessage(event)
    if (msg) this.sendServer(msg)
  }

  sendServer(msg: ServerMessage): void {
    this.transport.send(JSON.stringify(msg))
  }
}

/**
 * Validates inbound ClientMessages and dispatches them to the SimulationManager,
 * binding each connection to its sim_id on Create/Attach. Pure logic — driven by
 * a WsClient over any transport, so it is unit-testable without a real socket.
 */
export class MessageRouter {
  constructor(private readonly manager: SimulationManager) {}

  handleRaw(client: WsClient, raw: string): void {
    let parsed: unknown
    try {
      parsed = JSON.parse(raw)
    } catch {
      client.sendServer({ type: 'Error', message: 'invalid JSON' })
      return
    }
    if (!Value.Check(ClientMessage, parsed)) {
      client.sendServer({ type: 'Error', message: 'invalid message' })
      return
    }
    this.dispatch(client, parsed)
  }

  handleClose(client: WsClient): void {
    if (client.simId) this.manager.detach(client.simId, client)
  }

  private dispatch(client: WsClient, msg: ClientMessageType): void {
    if (msg.type === 'CreateSimulation') {
      let simId: string
      try {
        simId = this.manager.create()
      } catch (err) {
        client.sendServer({ type: 'Error', message: err instanceof Error ? err.message : String(err) })
        return
      }
      this.manager.attach(simId, client)
      client.simId = simId
      client.sendServer({ type: 'SimulationCreated', simId })
      if (msg.scenario.vessels.length > 0) {
        this.manager.command(simId, { type: 'loadScenario', vessels: msg.scenario.vessels })
      }
      if (msg.scenario.environmentJson) {
        this.manager.command(simId, { type: 'loadEnvironment', json: msg.scenario.environmentJson })
      }
      return
    }

    if (msg.type === 'AttachSimulation') {
      if (this.manager.attach(msg.simId, client)) {
        client.simId = msg.simId
      } else {
        client.sendServer({ type: 'Error', message: 'unknown simulation' })
      }
      return
    }

    const cmd = toWorkerCommand(msg)
    if (cmd) this.manager.command(msg.simId, cmd)
  }
}
