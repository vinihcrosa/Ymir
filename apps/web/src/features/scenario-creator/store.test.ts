import { describe, it, expect, beforeEach } from 'vitest'
import { useScenarioStore } from './store'
import type { AreaDTO } from '@ymir/types'

const mockArea: AreaDTO = {
  id: 42,
  name: 'Test Area',
  origin: { latitude: -22.9, longitude: -43.2 },
  polygon: { type: 'Polygon', coordinates: [] },
  gravity: 9.81,
  magneticCorrection: -22.5,
  waterDensity: 1.025,
  airDensity: 0.001275,
  createdAt: '2024-01-01T00:00:00Z',
}

describe('useScenarioStore', () => {
  beforeEach(() => {
    useScenarioStore.getState().reset()
  })

  it('addVessel adds vessel with x=0, y=0, headingDeg=0', () => {
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'Ship A' })
    const vessels = useScenarioStore.getState().vessels
    expect(vessels).toHaveLength(1)
    expect(vessels[0]).toMatchObject({ vesselId: 1, name: 'Ship A', x: 0, y: 0, headingDeg: 0 })
  })

  it('removeVessel removes correct vessel by vesselId', () => {
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'Ship A' })
    useScenarioStore.getState().addVessel({ vesselId: 2, name: 'Ship B' })
    useScenarioStore.getState().removeVessel(1)
    const vessels = useScenarioStore.getState().vessels
    expect(vessels).toHaveLength(1)
    expect(vessels[0].vesselId).toBe(2)
  })

  it('updateVesselPosition updates x and y', () => {
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'Ship A' })
    useScenarioStore.getState().updateVesselPosition(1, 100, 200)
    const vessel = useScenarioStore.getState().vessels.find(v => v.vesselId === 1)
    expect(vessel?.x).toBe(100)
    expect(vessel?.y).toBe(200)
  })

  it('updateVesselHeading normalizes heading to [0,360)', () => {
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'Ship A' })

    useScenarioStore.getState().updateVesselHeading(1, -10)
    expect(useScenarioStore.getState().vessels[0].headingDeg).toBe(350)

    useScenarioStore.getState().updateVesselHeading(1, 370)
    expect(useScenarioStore.getState().vessels[0].headingDeg).toBe(10)

    useScenarioStore.getState().updateVesselHeading(1, 360)
    expect(useScenarioStore.getState().vessels[0].headingDeg).toBe(0)

    useScenarioStore.getState().updateVesselHeading(1, 0)
    expect(useScenarioStore.getState().vessels[0].headingDeg).toBe(0)
  })

  it('setArea sets both area and areaId', () => {
    useScenarioStore.getState().setArea(mockArea)
    const state = useScenarioStore.getState()
    expect(state.area).toEqual(mockArea)
    expect(state.areaId).toBe(42)
  })

  it('toCreateScenarioDTO converts headingDeg to psi in radians', () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'Ship A' })
    useScenarioStore.getState().updateVesselHeading(1, 90)
    const dto = useScenarioStore.getState().toCreateScenarioDTO()
    expect(dto.initialConditions).toHaveLength(1)
    expect(dto.initialConditions[0].psi).toBeCloseTo(Math.PI / 2)
    expect(dto.initialConditions[0].vesselId).toBe(1)
    expect(dto.areaId).toBe(42)
  })

  it('reset clears vessels, area, and areaId', () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'Ship A' })
    useScenarioStore.getState().reset()
    const state = useScenarioStore.getState()
    expect(state.vessels).toHaveLength(0)
    expect(state.area).toBeNull()
    expect(state.areaId).toBeNull()
  })
})
