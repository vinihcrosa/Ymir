import { readFileSync } from 'node:fs'
import { resolve, dirname } from 'node:path'
import { fileURLToPath } from 'node:url'
import { db } from './index.js'
import {
  vessels,
  vesselPhysicalProps,
  vesselCurrent,
  vesselConnectionPoints,
  vesselRudders,
  vesselThrusters,
  vesselWaves,
} from './schema.js'

const __dirname = dirname(fileURLToPath(import.meta.url))

// Default: vessel1.json at repo root, two directories above apps/api/
const VESSEL1_PATH =
  process.env.VESSEL1_JSON_PATH ?? resolve(__dirname, '../../../../vessel1.json')

interface Vessel1Json {
  name: string
  connectionPoints: Array<{ id: number; type: string; name: string; position: [number, number, number] }>
  rudder: Array<{
    id: number
    name: string
    angleMaximum: number
    area: number
    speed: number
    associatedThruster: number
    position: number[]
    table: number[][]
  }>
  thruster: Array<{
    id: number
    name: string
    position: number[]
    azimuthSpeed: number
    diameter: number
    rotationSpeed: number
    rotationSpeedMax: number
    rotationTime: number
    maximumPower: number
    mechanicalEfficiency: number
    asternEfficiency: number
    hullEfficiency: number
    paddleEffect: number[]
    pitchDiameterRelation: number
    table: number[][]
    transversalSpeed: { formFactor: number; maximumSpeed: number }
    EOT: Array<{ range: [number, number]; value: number; label: string }>
  }>
  current: {
    area: { frontal: number; frontalHeight: number; lateral: number; lateralHeight: number }
    midshipDistance: number
    coefficients: number[][]
  }
  damping: {
    linearDampingCoefficient: number
    linear: number[][]
    potential: number[][]
    quadratic: number[][]
  }
  dimensions: {
    lengthPerpendiculars: number
    beam: number
    draft: number
    bouiancy: {
      floatCenter: [number, number, number]
      displacement: { volumetric: number; weight: number }
      hydrostaticRestoring: number[][]
      blockCoefficient: number
    }
    mass: {
      centerOfGravity: [number, number, number]
      massMatrix: number[][]
      addedMass: number[][]
    }
  }
  waves: {
    omega: number[]
    angle: number[]
    originPosition: number[]
    waveForcesAmplitude: number[][][]
  }
}

export function seedVessel1(): void {
  // Skip if vessel with this name already exists
  const existing = db.select().from(vessels).all()
  const alreadySeeded = existing.some(v => v.name === 'VLCC_L350_B63_T230')
  if (alreadySeeded) return

  let raw: Vessel1Json
  try {
    const content = readFileSync(VESSEL1_PATH, 'utf-8')
    // JSON allows duplicate keys — last wins, which gives us 'VLCC_L350_B63_T230'
    raw = JSON.parse(content) as Vessel1Json
  } catch (err) {
    // Non-fatal: API starts without full vessel data
    console.warn(`[seed] vessel1.json not found at ${VESSEL1_PATH}: ${String(err)}`)
    return
  }

  const d = raw.dimensions

  const now = new Date().toISOString()

  const [vessel] = db
    .insert(vessels)
    .values({
      name: raw.name,
      length: d.lengthPerpendiculars,
      beam: d.beam,
      draft: d.draft,
      mass: d.mass.massMatrix[0][0], // surge inertia ≈ ship mass [t]
      createdAt: now,
    })
    .returning()
    .all()

  const vid = vessel.id

  db.insert(vesselPhysicalProps).values({
    vesselId: vid,
    displacementVolumetric: d.bouiancy.displacement.volumetric,
    displacementWeight: d.bouiancy.displacement.weight,
    floatCenter: JSON.stringify(d.bouiancy.floatCenter),
    blockCoefficient: d.bouiancy.blockCoefficient,
    massCenter: JSON.stringify(d.mass.centerOfGravity),
    massMatrix: JSON.stringify(d.mass.massMatrix),
    addedMass: JSON.stringify(d.mass.addedMass),
    hydrostaticRestoring: JSON.stringify(d.bouiancy.hydrostaticRestoring),
    dampingLinearCoeff: raw.damping.linearDampingCoefficient,
    dampingLinear: JSON.stringify(raw.damping.linear),
    dampingPotential: JSON.stringify(raw.damping.potential),
    dampingQuadratic: JSON.stringify(raw.damping.quadratic),
  }).run()

  db.insert(vesselCurrent).values({
    vesselId: vid,
    frontalArea: raw.current.area.frontal,
    frontalHeight: raw.current.area.frontalHeight,
    lateralArea: raw.current.area.lateral,
    lateralHeight: raw.current.area.lateralHeight,
    midshipDistance: raw.current.midshipDistance,
    coefficients: JSON.stringify(raw.current.coefficients),
  }).run()

  if (raw.connectionPoints.length > 0) {
    db.insert(vesselConnectionPoints)
      .values(
        raw.connectionPoints.map(cp => ({
          vesselId: vid,
          pointId: cp.id,
          name: cp.name,
          type: cp.type,
          x: cp.position[0],
          y: cp.position[1],
          z: cp.position[2],
        })),
      )
      .run()
  }

  for (const r of raw.rudder) {
    db.insert(vesselRudders).values({
      vesselId: vid,
      rudderId: r.id,
      name: r.name,
      angleMaximum: r.angleMaximum,
      area: r.area,
      speed: r.speed,
      associatedThruster: r.associatedThruster,
      position: JSON.stringify(r.position),
      forceTable: JSON.stringify(r.table),
    }).run()
  }

  for (const t of raw.thruster) {
    db.insert(vesselThrusters).values({
      vesselId: vid,
      thrusterId: t.id,
      name: t.name,
      position: JSON.stringify(t.position),
      azimuthSpeed: t.azimuthSpeed,
      diameter: t.diameter,
      rotationSpeed: t.rotationSpeed,
      rotationSpeedMax: t.rotationSpeedMax,
      rotationTime: t.rotationTime,
      maximumPower: t.maximumPower,
      mechanicalEfficiency: t.mechanicalEfficiency,
      asternEfficiency: t.asternEfficiency,
      hullEfficiency: t.hullEfficiency,
      pitchDiameterRelation: t.pitchDiameterRelation,
      formFactor: t.transversalSpeed.formFactor,
      maximumTransversalSpeed: t.transversalSpeed.maximumSpeed,
      paddleEffect: JSON.stringify(t.paddleEffect),
      forceTable: JSON.stringify(t.table),
      eotTable: JSON.stringify(t.EOT),
    }).run()
  }

  db.insert(vesselWaves).values({
    vesselId: vid,
    omega: JSON.stringify(raw.waves.omega),
    angle: JSON.stringify(raw.waves.angle),
    originPosition: JSON.stringify(raw.waves.originPosition),
    forcesAmplitude: JSON.stringify(raw.waves.waveForcesAmplitude),
  }).run()

  console.info(`[seed] Imported vessel1.json → vessel id=${vid} (${raw.name})`)
}

/** Legacy seed: two placeholder vessels for scenarios demo */
export function seedPlaceholdersIfEmpty(): void {
  const count = db.select().from(vessels).all().length
  if (count > 0) return

  const now = new Date().toISOString()
  db.insert(vessels).values([
    { name: 'Rebocador RB-01', length: 32.0, beam: 10.5, draft: 4.2, mass: 850_000, createdAt: now },
    { name: 'AHTS Ymir-I', length: 75.0, beam: 18.0, draft: 6.5, mass: 4_200_000, createdAt: now },
  ]).run()
}
