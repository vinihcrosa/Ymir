import { Type, Static } from '@sinclair/typebox'
import { SimulationStateDTO } from './simulation.js'

// ── Shared payloads ──────────────────────────────────────────────────────────

/**
 * A vessel placed on a scenario draft (client-side scenario builder shape).
 * Previously declared ad-hoc in the web worker/store; now the shared contract.
 */
export const ScenarioDraftVesselDTO = Type.Object({
  /** Per-scenario body identifier used by the physics engine. */
  instanceId: Type.Number(),
  /** Vessel TYPE id — used to load the vessel config. */
  vesselId: Type.Number(),
  name: Type.String(),
  x: Type.Number({ description: 'Initial surge position [m]' }),
  y: Type.Number({ description: 'Initial sway position [m]' }),
  headingDeg: Type.Number({ description: 'Initial heading [deg]' }),
})
export type ScenarioDraftVesselDTO = Static<typeof ScenarioDraftVesselDTO>

/** Minimal scenario input to boot a simulation. */
export const ScenarioInputDTO = Type.Object({
  vessels: Type.Array(ScenarioDraftVesselDTO),
  environmentJson: Type.Optional(Type.String()),
})
export type ScenarioInputDTO = Static<typeof ScenarioInputDTO>

// ── Client → Server commands ─────────────────────────────────────────────────

export const ClientMessage = Type.Union([
  Type.Object({ type: Type.Literal('CreateSimulation'), scenario: ScenarioInputDTO }),
  Type.Object({ type: Type.Literal('AttachSimulation'), simId: Type.String() }),
  Type.Object({ type: Type.Literal('Play'), simId: Type.String(), dt: Type.Optional(Type.Number()) }),
  Type.Object({ type: Type.Literal('Pause'), simId: Type.String() }),
  Type.Object({ type: Type.Literal('Reset'), simId: Type.String() }),
  Type.Object({
    type: Type.Literal('LoadScenario'),
    simId: Type.String(),
    vessels: Type.Array(ScenarioDraftVesselDTO),
  }),
  Type.Object({
    type: Type.Literal('SetActuator'),
    simId: Type.String(),
    vesselId: Type.Number(),
    deviceType: Type.Union([Type.Literal('rudder'), Type.Literal('thruster')]),
    deviceId: Type.Number(),
    value: Type.Number(),
    value2: Type.Optional(Type.Number()),
  }),
  Type.Object({ type: Type.Literal('LoadEnvironment'), simId: Type.String(), json: Type.String() }),
])
export type ClientMessage = Static<typeof ClientMessage>

// ── Server → Client events ───────────────────────────────────────────────────

export const ServerMessage = Type.Union([
  Type.Object({ type: Type.Literal('SimulationCreated'), simId: Type.String() }),
  Type.Object({ type: Type.Literal('State'), payload: SimulationStateDTO }),
  Type.Object({
    type: Type.Literal('Status'),
    status: Type.Union([Type.Literal('running'), Type.Literal('paused'), Type.Literal('ended')]),
  }),
  Type.Object({ type: Type.Literal('Error'), message: Type.String() }),
])
export type ServerMessage = Static<typeof ServerMessage>
