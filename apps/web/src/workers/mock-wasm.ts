import type { SimulationStateDTO } from '@ymir/types'

interface ActuatorStore {
  rudderAngles: Record<number, number>     // rudderId → deg
  thrusterPowers: Record<number, number>   // thrusterId → pct (-100..100)
  thrusterAzimuths: Record<number, number> // thrusterId → deg
}

class MockYmirSimulation {
  private t = 0
  private vessels: SimulationStateDTO['vessels'] = []
  // per-vessel actuator state: vesselId → ActuatorStore
  private actuators: Record<number, ActuatorStore> = {}

  addVessel(id: number) {
    this.addVesselAt(id, 0, 0, 0)
  }

  addVesselAt(id: number, x: number, y: number, psi: number) {
    this.vessels.push({ id, x, y, z: 0, phi: 0, theta: 0, psi, u: 0, v: 0, r: 0 })
    this.actuators[id] = { rudderAngles: {}, thrusterPowers: {}, thrusterAzimuths: {} }
  }

  setRudderAngle(vesselId: number, rudderId: number, angleDeg: number) {
    if (!this.actuators[vesselId]) return
    this.actuators[vesselId].rudderAngles[rudderId] = angleDeg
  }

  setThrusterCommand(vesselId: number, thrusterId: number, powerPct: number, azimuthDeg: number) {
    if (!this.actuators[vesselId]) return
    this.actuators[vesselId].thrusterPowers[thrusterId] = powerPct
    this.actuators[vesselId].thrusterAzimuths[thrusterId] = azimuthDeg
  }

  step(dt: number) {
    this.t += dt
    for (const v of this.vessels) {
      const act = this.actuators[v.id] ?? { rudderAngles: {}, thrusterPowers: {}, thrusterAzimuths: {} }

      // Sum thruster forces in body frame
      let fx = 0 // surge force contribution
      let fy = 0 // sway force contribution
      for (const [, pct] of Object.entries(act.thrusterPowers)) {
        const azRad = ((act.thrusterAzimuths[Number(Object.keys(act.thrusterPowers)[0])] ?? 0) * Math.PI) / 180
        const f = (Number(pct) / 100) * 5.0  // simplified: 5 m/s² at 100%
        fx += f * Math.cos(azRad)
        fy += f * Math.sin(azRad)
      }

      // Rudder → yaw moment (simplified: proportional to average angle)
      const rudderAngles = Object.values(act.rudderAngles)
      const avgRudder = rudderAngles.length > 0
        ? rudderAngles.reduce((s, a) => s + a, 0) / rudderAngles.length
        : 0
      // yaw rate target proportional to rudder angle and surge speed
      const rTarget = (avgRudder / 45) * 0.08 * Math.sign(v.u + 0.01)

      // Simple first-order dynamics
      v.u += (fx - 0.5 * v.u) * dt      // damping + thrust
      v.v += (fy - 0.5 * v.v) * dt
      v.r += (rTarget - v.r) * 2.0 * dt  // fast yaw rate response

      // Integrate position in inertial frame
      const cosP = Math.cos(v.psi)
      const sinP = Math.sin(v.psi)
      v.x += (v.u * cosP - v.v * sinP) * dt
      v.y += (v.u * sinP + v.v * cosP) * dt
      v.psi += v.r * dt
    }
  }

  getState(): SimulationStateDTO {
    return { t: this.t, vessels: this.vessels.map(v => ({ ...v })) }
  }

  reset() {
    this.t = 0
    for (const v of this.vessels) {
      v.x = 0; v.y = 0; v.psi = 0; v.u = 0; v.v = 0; v.r = 0
    }
  }

  getTime() { return this.t }
}

export default async function createMockModule() {
  return { YmirSimulation: MockYmirSimulation }
}
