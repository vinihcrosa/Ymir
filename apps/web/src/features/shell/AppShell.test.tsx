import { render, screen, fireEvent, waitFor } from '@testing-library/react'
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { MemoryRouter } from 'react-router-dom'
import { AppShell } from './AppShell'
import { useScenarioStore } from '../scenario-creator/store'
import { useSimulationStore } from '../../stores/simulationStore'

// Leaflet map can't render in jsdom — stub the map surface (covered by e2e).
vi.mock('../scenario-creator/components/AreaMapView', () => ({
  AreaMapView: () => <div data-testid="map" />,
}))

vi.stubGlobal('Worker', class {
  onmessage: ((e: MessageEvent) => void) | null = null
  onerror: ((e: ErrorEvent) => void) | null = null
  postMessage = vi.fn()
  terminate = vi.fn()
})
vi.stubGlobal('URL', class {
  constructor(public href: string, public base?: string) {}
})

const mockArea = { id: 1, name: 'Test', origin: { latitude: -22.9, longitude: -43.2 }, polygon: { type: 'Polygon' as const, coordinates: [] }, gravity: 9.81, magneticCorrection: -22.5, waterDensity: 1.025, airDensity: 0.001275, createdAt: '2024-01-01T00:00:00Z' }

describe('AppShell', () => {
  beforeEach(() => {
    useScenarioStore.getState().reset()
    useSimulationStore.getState().reset()
    // Sidebar hooks fetch /areas and /vessels (expect arrays); the scenario save
    // POSTs to /scenarios. Return an array by default so list hooks don't throw.
    vi.stubGlobal('fetch', vi.fn().mockResolvedValue({ ok: true, json: () => Promise.resolve([]) }))
  })

  it('renders the top bar (breadcrumb + save) and floating controls', () => {
    render(<MemoryRouter><AppShell /></MemoryRouter>)
    expect(screen.getByRole('button', { name: /Salvar cenário/i })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /Aproximar/i })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /Afastar/i })).toBeInTheDocument()
    expect(screen.getByLabelText(/Controle de simulação/i)).toBeInTheDocument()
  })

  it('opens the no-ownship alert when saving with no vessels', () => {
    render(<MemoryRouter><AppShell /></MemoryRouter>)
    fireEvent.click(screen.getByRole('button', { name: /Salvar cenário/i }))
    expect(screen.getByText('Seu cenário não possui um ownship')).toBeInTheDocument()
    // sidebar hooks fetch /areas + /vessels on mount; what must NOT happen is a scenario save
    expect(fetch).not.toHaveBeenCalledWith(
      expect.stringContaining('/scenarios'),
      expect.objectContaining({ method: 'POST' }),
    )
  })

  it('"Salvar mesmo assim" confirms the save and closes the dialog', async () => {
    render(<MemoryRouter><AppShell /></MemoryRouter>)
    fireEvent.click(screen.getByRole('button', { name: /Salvar cenário/i }))
    fireEvent.click(screen.getByRole('button', { name: /Salvar mesmo assim/i }))
    await waitFor(() => expect(fetch).toHaveBeenCalledWith(
      expect.stringContaining('/scenarios'),
      expect.objectContaining({ method: 'POST' }),
    ))
    expect(screen.queryByText(/não possui um ownship/i)).toBeNull()
  })

  it('saves directly (no dialog) when a vessel exists', async () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'VLCC' })
    render(<MemoryRouter><AppShell /></MemoryRouter>)
    fireEvent.click(screen.getByRole('button', { name: /Salvar cenário/i }))
    expect(screen.queryByText(/não possui um ownship/i)).toBeNull()
    await waitFor(() => expect(fetch).toHaveBeenCalled())
  })

  it('Play starts the simulation (no implicit save)', async () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'VLCC' })
    render(<MemoryRouter><AppShell /></MemoryRouter>)
    fireEvent.click(screen.getByRole('button', { name: /Reproduzir/i }))
    await waitFor(() => expect(['loading', 'running']).toContain(useSimulationStore.getState().status))
    // Play must not persist the scenario — that is the explicit "Salvar cenário" action.
    expect(fetch).not.toHaveBeenCalledWith(
      expect.stringContaining('/scenarios'),
      expect.objectContaining({ method: 'POST' }),
    )
  })

  it('pause freezes without resetting (status paused, state kept)', async () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'VLCC' })
    render(<MemoryRouter><AppShell /></MemoryRouter>)
    // Simulate a running sim with elapsed state, then pause via the store.
    useSimulationStore.setState({ status: 'running', state: { t: 12.3, vessels: [] } })
    useSimulationStore.getState().pause()
    expect(useSimulationStore.getState().status).toBe('paused')
    expect(useSimulationStore.getState().state?.t).toBe(12.3)
  })
})
