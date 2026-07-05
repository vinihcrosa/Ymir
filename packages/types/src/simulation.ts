import { Type, Static } from '@sinclair/typebox'

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

