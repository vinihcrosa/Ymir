import { render, screen } from '@testing-library/react'
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { SimulationControls } from './SimulationControls'
import { useScenarioStore } from '../store'
import { useSimulationStore } from '../../../stores/simulationStore'

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

describe('SimulationControls', () => {
  beforeEach(() => {
    useScenarioStore.getState().reset()
    useSimulationStore.getState().reset()
  })

  it('start button disabled with no area', () => {
    render(<SimulationControls />)
    expect(screen.getByRole('button', { name: /iniciar/i })).toBeDisabled()
  })

  it('start button disabled with area but no vessels', () => {
    useScenarioStore.getState().setArea(mockArea)
    render(<SimulationControls />)
    expect(screen.getByRole('button', { name: /iniciar/i })).toBeDisabled()
  })

  it('start button enabled with area and vessels', () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'VLCC' })
    render(<SimulationControls />)
    expect(screen.getByRole('button', { name: /iniciar/i })).not.toBeDisabled()
  })

  it('shows idle status text initially', () => {
    render(<SimulationControls />)
    expect(screen.getByText(/aguardando/i)).toBeInTheDocument()
  })

  it('shows loading status text when loading', () => {
    useSimulationStore.setState({ status: 'loading' })
    render(<SimulationControls />)
    expect(screen.getByText(/iniciando/i)).toBeInTheDocument()
  })

  it('shows ready status text', () => {
    useSimulationStore.setState({ status: 'ready' })
    render(<SimulationControls />)
    expect(screen.getByText(/pronto/i)).toBeInTheDocument()
  })

  it('shows running status and stop button when running', () => {
    useSimulationStore.setState({ status: 'running', state: { t: 42.5, vessels: [] } })
    render(<SimulationControls />)
    expect(screen.getByRole('button', { name: /parar/i })).toBeInTheDocument()
    expect(screen.getByText(/42\.5s/)).toBeInTheDocument()
  })

  it('shows running without state', () => {
    useSimulationStore.setState({ status: 'running', state: null })
    render(<SimulationControls />)
    expect(screen.getByText(/rodando/i)).toBeInTheDocument()
  })

  it('shows error status text', () => {
    useSimulationStore.setState({ status: 'error' })
    render(<SimulationControls />)
    expect(screen.getByText(/erro/i)).toBeInTheDocument()
  })
})
