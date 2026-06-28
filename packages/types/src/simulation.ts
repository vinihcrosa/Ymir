import { Type, Static } from '@sinclair/typebox'
import { VesselConfigDTO } from './vessel.js'

export const VesselStateDTO = Type.Object({
  id: Type.Number(),
  x: Type.Number(),
  y: Type.Number(),
  z: Type.Number(),
  phi: Type.Number(),
  theta: Type.Number(),
  psi: Type.Number(),
  u: Type.Number(),
  v: Type.Number(),
  r: Type.Number(),
})
export type VesselStateDTO = Static<typeof VesselStateDTO>

export const SimulationStateDTO = Type.Object({
  t: Type.Number({ description: 'Simulation time [s]' }),
  vessels: Type.Array(VesselStateDTO),
})
export type SimulationStateDTO = Static<typeof SimulationStateDTO>

export const WorkerMessageDTO = Type.Union([
  Type.Object({ type: Type.Literal('ready') }),
  Type.Object({ type: Type.Literal('state'), payload: SimulationStateDTO }),
  Type.Object({ type: Type.Literal('error'), message: Type.String() }),
  Type.Object({ type: Type.Literal('vessel_config'), payload: VesselConfigDTO }),
])
export type WorkerMessageDTO = Static<typeof WorkerMessageDTO>
