import { describe, it, expect, beforeEach } from 'vitest'
import { useVesselPanelStore } from './vesselPanelStore'
import type { VesselConfigDTO } from '@ymir/types'

const config = { id: 1, name: 'VLCC' } as unknown as VesselConfigDTO

function reset() {
  useVesselPanelStore.setState({
    selectedVesselId: null,
    vesselTypeId: null,
    config: null,
    configLoading: false,
    rudderAngles: {},
    thrusterPowers: {},
    thrusterAzimuths: {},
  })
}

describe('vesselPanelStore', () => {
  beforeEach(reset)

  it('open() selects the instance, records the type and clears actuators', () => {
    useVesselPanelStore.getState().setRudderAngle(1, 20)
    useVesselPanelStore.getState().open(7, 3)
    const s = useVesselPanelStore.getState()
    expect(s.selectedVesselId).toBe(7)
    expect(s.vesselTypeId).toBe(3)
    expect(s.configLoading).toBe(true)
    expect(s.rudderAngles).toEqual({})
  })

  it('close() clears selection and config', () => {
    useVesselPanelStore.getState().open(7, 3)
    useVesselPanelStore.getState().close()
    const s = useVesselPanelStore.getState()
    expect(s.selectedVesselId).toBeNull()
    expect(s.vesselTypeId).toBeNull()
    expect(s.config).toBeNull()
  })

  it('setConfig() stores config and loading flag', () => {
    useVesselPanelStore.getState().setConfig(config, false)
    const s = useVesselPanelStore.getState()
    expect(s.config).toBe(config)
    expect(s.configLoading).toBe(false)
  })

  it('actuator setters keep per-device values independent', () => {
    const st = useVesselPanelStore.getState()
    st.setRudderAngle(0, -15)
    st.setRudderAngle(1, 30)
    st.setThrusterPower(0, 80)
    st.setThrusterAzimuth(0, 45)
    const s = useVesselPanelStore.getState()
    expect(s.rudderAngles).toEqual({ 0: -15, 1: 30 })
    expect(s.thrusterPowers).toEqual({ 0: 80 })
    expect(s.thrusterAzimuths).toEqual({ 0: 45 })
  })
})
