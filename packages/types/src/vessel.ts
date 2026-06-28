import { Type, Static } from '@sinclair/typebox'

export const VesselDTO = Type.Object({
  id: Type.Number(),
  name: Type.String(),
  length: Type.Number({ description: 'Length between perpendiculars [m]' }),
  beam: Type.Number({ description: 'Beam [m]' }),
  draft: Type.Number({ description: 'Draft [m]' }),
  mass: Type.Number({ description: 'Mass [kg]' }),
  createdAt: Type.String({ format: 'date-time' }),
})

export type VesselDTO = Static<typeof VesselDTO>

export const CreateVesselDTO = Type.Omit(VesselDTO, ['id', 'createdAt'])
export type CreateVesselDTO = Static<typeof CreateVesselDTO>
