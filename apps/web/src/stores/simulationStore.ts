import { create } from 'zustand'
import type { SimulationStateDTO, ScenarioDraftVesselDTO, ServerMessage } from '@ymir/types'
import { SimulationSocket, type ConnectionState } from '../lib/simulation-socket'
import { useVesselPanelStore } from './vesselPanelStore'
import { useEnvironmentStore } from './environmentStore'

type Status = 'idle' | 'loading' | 'running' | 'paused' | 'error'

type ScenarioDraftVessel = ScenarioDraftVesselDTO

/** Socket factory — overridable in tests to inject a fake transport. */
let socketFactory: () => SimulationSocket = () => new SimulationSocket()
export function __setSocketFactory(factory: (() => SimulationSocket) | null): void {
  socketFactory = factory ?? (() => new SimulationSocket())
}

// dt to send with Play once the server confirms a freshly created simulation.
let pendingPlayDt: number | null = null

interface SimulationStore {
  status: Status
  error: string | null
  state: SimulationStateDTO | null
  connection: ConnectionState | 'idle'
  socket: SimulationSocket | null
  simId: string | null
  scenarioVessels: ScenarioDraftVessel[]
  /** True once a session (socket) exists. */
  hasSession: () => boolean
  /** Start (first time: create the sim) or resume the simulation. */
  play: (dt?: number) => void
  /** Pause the simulation on the server; state is retained. */
  pause: () => void
  /** End the simulation, close the socket, and clear local state. */
  reset: () => void
  loadScenario: (vessels: ScenarioDraftVessel[]) => void
  /** Push the current environment profile to the running simulation. */
  applyEnvironment: () => void
  /** Send an actuator command (rudder/thruster) to a vessel. */
  sendActuator: (
    vesselId: number,
    deviceType: 'rudder' | 'thruster',
    deviceId: number,
    value: number,
    value2?: number,
  ) => void
}

export const useSimulationStore = create<SimulationStore>((set, get) => {
  function resyncActuators(simId: string): void {
    const panel = useVesselPanelStore.getState()
    if (panel.selectedVesselId === null) return
    const vesselId = panel.selectedVesselId
    const { socket } = get()
    if (!socket) return
    for (const [id, pct] of Object.entries(panel.thrusterPowers)) {
      const thrusterId = Number(id)
      socket.send({
        type: 'SetActuator', simId, vesselId, deviceType: 'thruster',
        deviceId: thrusterId, value: pct, value2: panel.thrusterAzimuths[thrusterId] ?? 0,
      })
    }
    for (const [id, deg] of Object.entries(panel.rudderAngles)) {
      socket.send({ type: 'SetActuator', simId, vesselId, deviceType: 'rudder', deviceId: Number(id), value: deg })
    }
  }

  function handleServer(msg: ServerMessage): void {
    const { socket } = get()
    switch (msg.type) {
      case 'SimulationCreated': {
        set({ simId: msg.simId })
        if (socket && pendingPlayDt !== null) {
          socket.send({ type: 'Play', simId: msg.simId, dt: pendingPlayDt })
          pendingPlayDt = null
          set({ status: 'running' })
        }
        resyncActuators(msg.simId)
        break
      }
      case 'State':
        set({ state: msg.payload })
        break
      case 'Status':
        set({ status: msg.status === 'ended' ? 'idle' : msg.status })
        break
      case 'Error':
        set({ status: 'error', error: msg.message })
        break
    }
  }

  return {
    status: 'idle',
    error: null,
    state: null,
    connection: 'idle',
    socket: null,
    simId: null,
    scenarioVessels: [],

    hasSession: () => get().socket !== null,

    play(dt = 0.05) {
      let { socket } = get()
      if (!socket) {
        socket = socketFactory()
        socket.onMessage(handleServer)
        socket.onConnectionChange((c) => set({ connection: c }))
        set({ socket, status: 'loading', error: null, simId: socket.simId })
        socket.connect()
      }
      const { simId, scenarioVessels } = get()
      if (simId) {
        socket.send({ type: 'Play', simId, dt })
        set({ status: 'running' })
      } else {
        pendingPlayDt = dt
        const env = useEnvironmentStore.getState()
        socket.createSimulation({
          vessels: scenarioVessels,
          environmentJson: env.hasConditions() ? env.toJson() : undefined,
        })
      }
    },

    pause() {
      const { socket, simId } = get()
      if (socket && simId) socket.send({ type: 'Pause', simId })
      set({ status: 'paused' })
    },

    applyEnvironment() {
      const { socket, simId } = get()
      const env = useEnvironmentStore.getState()
      if (socket && simId && env.hasConditions()) {
        socket.send({ type: 'LoadEnvironment', simId, json: env.toJson() })
      }
    },

    reset() {
      const { socket, simId } = get()
      if (socket && simId) socket.send({ type: 'Reset', simId })
      socket?.close()
      pendingPlayDt = null
      set({ status: 'idle', error: null, state: null, connection: 'idle', socket: null, simId: null, scenarioVessels: [] })
    },

    loadScenario(vessels) {
      set({ scenarioVessels: vessels })
      const { socket, simId } = get()
      if (socket && simId) socket.send({ type: 'LoadScenario', simId, vessels })
    },

    sendActuator(vesselId, deviceType, deviceId, value, value2) {
      const { socket, simId } = get()
      if (socket && simId) {
        socket.send({ type: 'SetActuator', simId, vesselId, deviceType, deviceId, value, value2 })
      }
    },
  }
})
