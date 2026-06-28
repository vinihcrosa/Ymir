import { sqliteTable, integer, real, text } from 'drizzle-orm/sqlite-core'

export const vessels = sqliteTable('vessels', {
  id: integer('id').primaryKey({ autoIncrement: true }),
  name: text('name').notNull(),
  length: real('length').notNull(),
  beam: real('beam').notNull(),
  draft: real('draft').notNull(),
  mass: real('mass').notNull(),
  createdAt: text('created_at').notNull(),
})

export const scenarios = sqliteTable('scenarios', {
  id: integer('id').primaryKey({ autoIncrement: true }),
  name: text('name').notNull(),
  description: text('description'),
  duration: real('duration').notNull(),
  dt: real('dt').notNull().default(0.1),
  initialConditions: text('initial_conditions').notNull(), // JSON string
  createdAt: text('created_at').notNull(),
})

// ── Supplementary vessel tables ───────────────────────────────────────────────

/** 6×6 matrices and displacement data */
export const vesselPhysicalProps = sqliteTable('vessel_physical_props', {
  vesselId: integer('vessel_id').primaryKey().references(() => vessels.id),
  displacementVolumetric: real('displacement_volumetric').notNull(),
  displacementWeight: real('displacement_weight').notNull(),
  floatCenter: text('float_center').notNull(),      // JSON [x, y, z]
  blockCoefficient: real('block_coefficient').notNull(),
  massCenter: text('mass_center').notNull(),         // JSON [x, y, z]
  massMatrix: text('mass_matrix').notNull(),         // JSON 6×6
  addedMass: text('added_mass').notNull(),           // JSON 6×6
  hydrostaticRestoring: text('hydrostatic_restoring').notNull(), // JSON 6×6
  dampingLinearCoeff: real('damping_linear_coeff').notNull(),
  dampingLinear: text('damping_linear').notNull(),   // JSON 6×6
  dampingPotential: text('damping_potential').notNull(),         // JSON 6×6
  dampingQuadratic: text('damping_quadratic').notNull(),         // JSON 6×6
})

/** Current force areas and coefficient table */
export const vesselCurrent = sqliteTable('vessel_current', {
  vesselId: integer('vessel_id').primaryKey().references(() => vessels.id),
  frontalArea: real('frontal_area').notNull(),
  frontalHeight: real('frontal_height').notNull(),
  lateralArea: real('lateral_area').notNull(),
  lateralHeight: real('lateral_height').notNull(),
  midshipDistance: real('midship_distance').notNull(),
  coefficients: text('coefficients').notNull(), // JSON [[angle, Cx, Cy, Cn]]
})

/** Tug and anchor connection points */
export const vesselConnectionPoints = sqliteTable('vessel_connection_points', {
  id: integer('id').primaryKey({ autoIncrement: true }),
  vesselId: integer('vessel_id').notNull().references(() => vessels.id),
  pointId: integer('point_id').notNull(),
  name: text('name').notNull(),
  type: text('type').notNull(), // 'TUG' | 'ANCHOR'
  x: real('x').notNull(),
  y: real('y').notNull(),
  z: real('z').notNull(),
})

/** Rudder config with polar force table */
export const vesselRudders = sqliteTable('vessel_rudders', {
  id: integer('id').primaryKey({ autoIncrement: true }),
  vesselId: integer('vessel_id').notNull().references(() => vessels.id),
  rudderId: integer('rudder_id').notNull(),
  name: text('name').notNull(),
  angleMaximum: real('angle_maximum').notNull(),
  area: real('area').notNull(),
  speed: real('speed').notNull(),
  associatedThruster: integer('associated_thruster').notNull(),
  position: text('position').notNull(), // JSON [x, y, z, azimuth]
  forceTable: text('force_table').notNull(), // JSON [[angle, Cy, Cx]]
})

/** Thruster / propeller config with Kt/Kq table and EOT */
export const vesselThrusters = sqliteTable('vessel_thrusters', {
  id: integer('id').primaryKey({ autoIncrement: true }),
  vesselId: integer('vessel_id').notNull().references(() => vessels.id),
  thrusterId: integer('thruster_id').notNull(),
  name: text('name').notNull(),
  position: text('position').notNull(),        // JSON [x, y, z, azimuth]
  azimuthSpeed: real('azimuth_speed').notNull(),
  diameter: real('diameter').notNull(),
  rotationSpeed: real('rotation_speed').notNull(),
  rotationSpeedMax: real('rotation_speed_max').notNull(),
  rotationTime: real('rotation_time').notNull(),
  maximumPower: real('maximum_power').notNull(),
  mechanicalEfficiency: real('mechanical_efficiency').notNull(),
  asternEfficiency: real('astern_efficiency').notNull(),
  hullEfficiency: real('hull_efficiency').notNull(),
  pitchDiameterRelation: real('pitch_diameter_relation').notNull(),
  formFactor: real('form_factor').notNull(),
  maximumTransversalSpeed: real('maximum_transversal_speed').notNull(),
  paddleEffect: text('paddle_effect').notNull(), // JSON [threshold, max_force, dir]
  forceTable: text('force_table').notNull(),     // JSON [[J, Kt, Kq]]
  eotTable: text('eot_table').notNull(),         // JSON [{ range, value, label }]
})

/** Wave RAO data: frequencies, headings, and 3D force amplitude table */
export const vesselWaves = sqliteTable('vessel_waves', {
  vesselId: integer('vessel_id').primaryKey().references(() => vessels.id),
  omega: text('omega').notNull(),           // JSON number[]
  angle: text('angle').notNull(),           // JSON number[]
  originPosition: text('origin_position').notNull(), // JSON [x, y, z]
  forcesAmplitude: text('forces_amplitude').notNull(), // JSON [6][n_freq][n_angle]
})
