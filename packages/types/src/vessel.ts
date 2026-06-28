import { Type, Static } from '@sinclair/typebox'

// ── Basic vessel (list / create) ─────────────────────────────────────────────

export const VesselDTO = Type.Object({
  id: Type.Number(),
  name: Type.String(),
  length: Type.Number({ description: 'Length between perpendiculars [m]' }),
  beam: Type.Number({ description: 'Beam [m]' }),
  draft: Type.Number({ description: 'Draft [m]' }),
  mass: Type.Number({ description: 'Mass [t] (from mass matrix diagonal)' }),
  createdAt: Type.String({ format: 'date-time' }),
})

export type VesselDTO = Static<typeof VesselDTO>

export const CreateVesselDTO = Type.Omit(VesselDTO, ['id', 'createdAt'])
export type CreateVesselDTO = Static<typeof CreateVesselDTO>

// ── Sub-components ───────────────────────────────────────────────────────────

export const VesselConnectionPointDTO = Type.Object({
  pointId: Type.Number({ description: 'Original id from JSON' }),
  name: Type.String(),
  type: Type.Union([Type.Literal('TUG'), Type.Literal('ANCHOR')]),
  position: Type.Tuple([Type.Number(), Type.Number(), Type.Number()]),
})
export type VesselConnectionPointDTO = Static<typeof VesselConnectionPointDTO>

export const EOTEntryDTO = Type.Object({
  range: Type.Tuple([Type.Number(), Type.Number()]),
  value: Type.Number(),
  label: Type.String(),
})
export type EOTEntryDTO = Static<typeof EOTEntryDTO>

export const VesselRudderDTO = Type.Object({
  rudderId: Type.Number(),
  name: Type.String(),
  angleMaximum: Type.Number({ description: 'Max deflection angle [deg]' }),
  area: Type.Number({ description: 'Rudder area [m²]' }),
  speed: Type.Number({ description: 'Turning speed [deg/s]' }),
  associatedThruster: Type.Number(),
  position: Type.Array(Type.Number(), { description: '[x, y, z, azimuth]' }),
  forceTable: Type.Array(Type.Array(Type.Number()), {
    description: 'Rows: [angle_deg, Cy, Cx]',
  }),
})
export type VesselRudderDTO = Static<typeof VesselRudderDTO>

export const VesselThrusterDTO = Type.Object({
  thrusterId: Type.Number(),
  name: Type.String(),
  position: Type.Array(Type.Number(), { description: '[x, y, z, azimuth]' }),
  azimuthSpeed: Type.Number({ description: '[deg/s]' }),
  diameter: Type.Number({ description: '[m]' }),
  rotationSpeed: Type.Number(),
  rotationSpeedMax: Type.Number({ description: '[rev/s]' }),
  rotationTime: Type.Number({ description: '[s] — time to reach max RPM' }),
  maximumPower: Type.Number({ description: '[kW]' }),
  mechanicalEfficiency: Type.Number(),
  asternEfficiency: Type.Number(),
  hullEfficiency: Type.Number(),
  pitchDiameterRelation: Type.Number(),
  formFactor: Type.Number(),
  maximumTransversalSpeed: Type.Number({ description: '[m/s]' }),
  paddleEffect: Type.Array(Type.Number()),
  forceTable: Type.Array(Type.Array(Type.Number()), {
    description: 'Rows: [J, Kt, Kq]',
  }),
  eot: Type.Array(EOTEntryDTO),
})
export type VesselThrusterDTO = Static<typeof VesselThrusterDTO>

export const VesselCurrentDTO = Type.Object({
  frontalArea: Type.Number({ description: '[m²]' }),
  frontalHeight: Type.Number({ description: '[m]' }),
  lateralArea: Type.Number({ description: '[m²]' }),
  lateralHeight: Type.Number({ description: '[m]' }),
  midshipDistance: Type.Number({ description: '[m]' }),
  coefficients: Type.Array(Type.Array(Type.Number()), {
    description: 'Rows: [angle_deg, Cx, Cy, Cn]',
  }),
})
export type VesselCurrentDTO = Static<typeof VesselCurrentDTO>

// Matrix6x6: TypeBox validates as number[][] — shape enforced at seed time.
const Matrix6x6 = Type.Array(Type.Array(Type.Number()))

export const VesselPhysicalPropsDTO = Type.Object({
  displacementVolumetric: Type.Number({ description: '[m³]' }),
  displacementWeight: Type.Number({ description: '[kN]' }),
  floatCenter: Type.Tuple([Type.Number(), Type.Number(), Type.Number()], {
    description: 'Centre of buoyancy [m] — body frame',
  }),
  blockCoefficient: Type.Number(),
  massCenter: Type.Tuple([Type.Number(), Type.Number(), Type.Number()], {
    description: 'Centre of gravity [m] — body frame',
  }),
  massMatrix: Matrix6x6,
  addedMass: Matrix6x6,
  hydrostaticRestoring: Matrix6x6,
  dampingLinearCoeff: Type.Number(),
  dampingLinear: Matrix6x6,
  dampingPotential: Matrix6x6,
  dampingQuadratic: Matrix6x6,
})
export type VesselPhysicalPropsDTO = Static<typeof VesselPhysicalPropsDTO>

export const VesselWavesDTO = Type.Object({
  omega: Type.Array(Type.Number(), { description: 'Wave frequencies [rad/s]' }),
  angle: Type.Array(Type.Number(), { description: 'Wave headings [deg]' }),
  originPosition: Type.Array(Type.Number(), { description: '[x, y, z]' }),
  // 3D array [6_dof][n_freq][n_angle] — opaque blob
  forcesAmplitude: Type.Array(Type.Any()),
})
export type VesselWavesDTO = Static<typeof VesselWavesDTO>

// ── Full config (physics engine input) ──────────────────────────────────────

export const VesselConfigDTO = Type.Object({
  id: Type.Number(),
  name: Type.String(),
  lpp: Type.Number({ description: 'Length between perpendiculars [m]' }),
  beam: Type.Number({ description: '[m]' }),
  draft: Type.Number({ description: '[m]' }),
  physicalProps: VesselPhysicalPropsDTO,
  connectionPoints: Type.Array(VesselConnectionPointDTO),
  rudders: Type.Array(VesselRudderDTO),
  thrusters: Type.Array(VesselThrusterDTO),
  current: VesselCurrentDTO,
  waves: VesselWavesDTO,
})
export type VesselConfigDTO = Static<typeof VesselConfigDTO>
