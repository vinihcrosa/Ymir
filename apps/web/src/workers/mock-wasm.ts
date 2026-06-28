import type { SimulationStateDTO } from '@ymir/types'

class MockYmirSimulation {
  private t = 0
  private vessels: SimulationStateDTO['vessels'] = []

  addVessel(id: number) {
    this.vessels.push({ id, x: 0, y: 0, z: 0, phi: 0, theta: 0, psi: 0, u: 0.5, v: 0, r: 0.01 })
  }

  step(dt: number) {
    this.t += dt
    for (const v of this.vessels) {
      v.x += v.u * dt
      v.y += v.v * dt
      v.psi += v.r * dt
    }
  }

  getState(): SimulationStateDTO {
    return { t: this.t, vessels: this.vessels.map(v => ({ ...v })) }
  }

  reset() {
    this.t = 0
    for (const v of this.vessels) {
      v.x = 0; v.y = 0; v.psi = 0
    }
  }

  getTime() { return this.t }
}

export default async function createMockModule() {
  return { YmirSimulation: MockYmirSimulation }
}
