import { eq } from 'drizzle-orm'
import { db } from '../db/index.js'
import {
  vessels,
  vesselPhysicalProps,
  vesselCurrent,
  vesselConnectionPoints,
  vesselRudders,
  vesselThrusters,
  vesselWaves,
} from '../db/schema.js'
import type { VesselConfigDTO } from '@ymir/types'

export type VesselRow = typeof vessels.$inferSelect
export type CreateVesselRow = typeof vessels.$inferInsert

export const vesselRepository = {
  findAll(): VesselRow[] {
    return db.select().from(vessels).all()
  },

  findById(id: number): VesselRow | undefined {
    return db.select().from(vessels).where(eq(vessels.id, id)).get()
  },

  create(data: Omit<CreateVesselRow, 'id'>): VesselRow {
    return db.insert(vessels).values(data).returning().get()
  },

  remove(id: number): void {
    db.delete(vessels).where(eq(vessels.id, id)).run()
  },

  findConfig(id: number): VesselConfigDTO | undefined {
    const vessel = db.select().from(vessels).where(eq(vessels.id, id)).get()
    if (!vessel) return undefined

    const props = db
      .select()
      .from(vesselPhysicalProps)
      .where(eq(vesselPhysicalProps.vesselId, id))
      .get()

    const current = db
      .select()
      .from(vesselCurrent)
      .where(eq(vesselCurrent.vesselId, id))
      .get()

    const points = db
      .select()
      .from(vesselConnectionPoints)
      .where(eq(vesselConnectionPoints.vesselId, id))
      .all()

    const rudders = db
      .select()
      .from(vesselRudders)
      .where(eq(vesselRudders.vesselId, id))
      .all()

    const thrusters = db
      .select()
      .from(vesselThrusters)
      .where(eq(vesselThrusters.vesselId, id))
      .all()

    const waves = db
      .select()
      .from(vesselWaves)
      .where(eq(vesselWaves.vesselId, id))
      .get()

    if (!props || !current || !waves) return undefined

    return {
      id: vessel.id,
      name: vessel.name,
      lpp: vessel.length,
      beam: vessel.beam,
      draft: vessel.draft,
      physicalProps: {
        displacementVolumetric: props.displacementVolumetric,
        displacementWeight: props.displacementWeight,
        floatCenter: JSON.parse(props.floatCenter),
        blockCoefficient: props.blockCoefficient,
        massCenter: JSON.parse(props.massCenter),
        massMatrix: JSON.parse(props.massMatrix),
        addedMass: JSON.parse(props.addedMass),
        hydrostaticRestoring: JSON.parse(props.hydrostaticRestoring),
        dampingLinearCoeff: props.dampingLinearCoeff,
        dampingLinear: JSON.parse(props.dampingLinear),
        dampingPotential: JSON.parse(props.dampingPotential),
        dampingQuadratic: JSON.parse(props.dampingQuadratic),
      },
      connectionPoints: points.map(p => ({
        pointId: p.pointId,
        name: p.name,
        type: p.type as 'TUG' | 'ANCHOR',
        position: [p.x, p.y, p.z],
      })),
      rudders: rudders.map(r => ({
        rudderId: r.rudderId,
        name: r.name,
        angleMaximum: r.angleMaximum,
        area: r.area,
        speed: r.speed,
        associatedThruster: r.associatedThruster,
        position: JSON.parse(r.position),
        forceTable: JSON.parse(r.forceTable),
      })),
      thrusters: thrusters.map(t => ({
        thrusterId: t.thrusterId,
        name: t.name,
        position: JSON.parse(t.position),
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
        formFactor: t.formFactor,
        maximumTransversalSpeed: t.maximumTransversalSpeed,
        paddleEffect: JSON.parse(t.paddleEffect),
        forceTable: JSON.parse(t.forceTable),
        eot: JSON.parse(t.eotTable),
      })),
      current: {
        frontalArea: current.frontalArea,
        frontalHeight: current.frontalHeight,
        lateralArea: current.lateralArea,
        lateralHeight: current.lateralHeight,
        midshipDistance: current.midshipDistance,
        coefficients: JSON.parse(current.coefficients),
      },
      waves: {
        omega: JSON.parse(waves.omega),
        angle: JSON.parse(waves.angle),
        originPosition: JSON.parse(waves.originPosition),
        forcesAmplitude: JSON.parse(waves.forcesAmplitude),
      },
    }
  },
}
