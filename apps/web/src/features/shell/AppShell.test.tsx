import { render, screen, fireEvent, waitFor } from '@testing-library/react'
import { describe, it, expect, vi, beforeEach } from 'vitest'
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
    vi.stubGlobal('fetch', vi.fn().mockResolvedValue({ ok: true, json: () => Promise.resolve({}) }))
  })

  it('renders the top bar (breadcrumb + save) and floating controls', () => {
    render(<AppShell />)
    expect(screen.getByRole('button', { name: /Salvar cenário/i })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /Aproximar/i })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /Afastar/i })).toBeInTheDocument()
    expect(screen.getByLabelText(/Controle de simulação/i)).toBeInTheDocument()
  })

  it('opens the no-ownship alert when saving with no vessels', () => {
    render(<AppShell />)
    fireEvent.click(screen.getByRole('button', { name: /Salvar cenário/i }))
    expect(screen.getByText('Seu cenário não possui um ownship')).toBeInTheDocument()
    // sidebar hooks fetch /areas + /vessels on mount; what must NOT happen is a scenario save
    expect(fetch).not.toHaveBeenCalledWith(
      expect.stringContaining('/scenarios'),
      expect.objectContaining({ method: 'POST' }),
    )
  })

  it('"Salvar mesmo assim" confirms the save and closes the dialog', async () => {
    render(<AppShell />)
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
    render(<AppShell />)
    fireEvent.click(screen.getByRole('button', { name: /Salvar cenário/i }))
    expect(screen.queryByText(/não possui um ownship/i)).toBeNull()
    await waitFor(() => expect(fetch).toHaveBeenCalled())
  })

  it('Build triggers scenario save + simulation start', async () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'VLCC' })
    render(<AppShell />)
    fireEvent.click(screen.getByRole('button', { name: /Build simulation/i }))
    await waitFor(() => expect(fetch).toHaveBeenCalled())
    expect(['loading', 'running']).toContain(useSimulationStore.getState().status)
  })
})
