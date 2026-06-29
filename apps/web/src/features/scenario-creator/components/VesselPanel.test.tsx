import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import { VesselPanel } from './VesselPanel'
import { useVesselPanelStore } from '../../../stores/vesselPanelStore'
import { useSimulationStore } from '../../../stores/simulationStore'
import type { VesselConfigDTO } from '@ymir/types'

const config = {
  id: 1,
  name: '3R_GUAMARE_VLCC',
  lpp: 350,
  beam: 63,
  draft: 23,
  physicalProps: { displacementWeight: 4350000 },
  connectionPoints: [],
  rudders: [{ rudderId: 0, name: 'LEME_PRINCIPAL', angleMaximum: 35 }],
  thrusters: [
    { thrusterId: 0, name: 'MAIN', maximumPower: 34000, azimuthSpeed: 0 },
    { thrusterId: 1, name: 'BOW_AZ', maximumPower: 2000, azimuthSpeed: 10 },
  ],
} as unknown as VesselConfigDTO

function mockFetchOnce(ok: boolean) {
  return vi.fn().mockResolvedValue({
    ok,
    json: () => Promise.resolve(config),
  })
}

function resetStores() {
  useVesselPanelStore.setState({
    selectedVesselId: null, vesselTypeId: null, config: null, configLoading: false,
    rudderAngles: {}, thrusterPowers: {}, thrusterAzimuths: {},
  })
  useSimulationStore.setState({ status: 'idle', state: null, worker: null })
}

describe('VesselPanel', () => {
  beforeEach(resetStores)
  afterEach(() => { vi.restoreAllMocks() })

  it('renders nothing when no vessel is selected', () => {
    const { container } = render(<VesselPanel />)
    expect(container.querySelector('[data-testid="vessel-panel"]')).toBeNull()
  })

  it('fetches and renders the Geral tab with vessel dimensions', async () => {
    vi.stubGlobal('fetch', mockFetchOnce(true))
    render(<VesselPanel />)
    useVesselPanelStore.getState().open(1, 1)
    expect(await screen.findByText('Dimensões')).toBeInTheDocument()
    expect(screen.getByText('350.0 m')).toBeInTheDocument() // LOA
    expect(screen.getByText('63.0 m')).toBeInTheDocument()  // Boca
    expect(screen.getByText('36000 kW')).toBeInTheDocument() // total power
  })

  it('shows error message when config fetch fails', async () => {
    vi.stubGlobal('fetch', mockFetchOnce(false))
    render(<VesselPanel />)
    useVesselPanelStore.getState().open(1, 1)
    expect(await screen.findByText(/Falha ao carregar/i)).toBeInTheDocument()
  })

  it('shows live kinematics (u/v/r) while running', async () => {
    vi.stubGlobal('fetch', mockFetchOnce(true))
    useSimulationStore.setState({
      status: 'running',
      state: { t: 10, vessels: [{ id: 1, x: 100, y: 50, z: 0, phi: 0, theta: 0, psi: 0, u: 7.81, v: -0.35, r: 0.0057 }] },
    })
    render(<VesselPanel />)
    useVesselPanelStore.getState().open(1, 1)
    await screen.findByText('Dimensões')
    expect(screen.getByText('7.81 m/s')).toBeInTheDocument()
    expect(screen.getByText('-0.35 m/s')).toBeInTheDocument()
    expect(screen.getByText('0.0057 rad/s')).toBeInTheDocument()
  })

  it('Controles tab moves a rudder and posts an actuator command', async () => {
    vi.stubGlobal('fetch', mockFetchOnce(true))
    const postMessage = vi.fn()
    useSimulationStore.setState({ worker: { postMessage } as unknown as Worker })
    render(<VesselPanel />)
    useVesselPanelStore.getState().open(1, 1)
    await screen.findByText('Dimensões')

    fireEvent.click(screen.getByRole('tab', { name: 'Controles' }))
    const sliders = await screen.findAllByRole('slider')
    // first slider is the rudder
    fireEvent.change(sliders[0], { target: { value: '20' } })

    expect(useVesselPanelStore.getState().rudderAngles[0]).toBe(20)
    expect(postMessage).toHaveBeenCalledWith(
      expect.objectContaining({ type: 'setActuator', deviceType: 'rudder', value: 20 }),
    )
  })

  it('renders an azimuth dial for azimuthal thrusters and reacts to clicks', async () => {
    vi.stubGlobal('fetch', mockFetchOnce(true))
    useSimulationStore.setState({ worker: { postMessage: vi.fn() } as unknown as Worker })
    render(<VesselPanel />)
    useVesselPanelStore.getState().open(1, 1)
    await screen.findByText('Dimensões')
    fireEvent.click(screen.getByRole('tab', { name: 'Controles' }))
    // azimuthal thruster shows "azimutal" label
    expect(await screen.findByText(/azimutal/)).toBeInTheDocument()
  })

  it('closes the panel via the close button', async () => {
    vi.stubGlobal('fetch', mockFetchOnce(true))
    render(<VesselPanel />)
    useVesselPanelStore.getState().open(1, 1)
    await screen.findByText('Dimensões')
    fireEvent.click(screen.getByRole('button', { name: /Fechar painel/i }))
    expect(useVesselPanelStore.getState().selectedVesselId).toBeNull()
  })
})
