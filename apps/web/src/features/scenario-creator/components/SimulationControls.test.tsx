import { render, screen } from '@testing-library/react'
import { describe, it, expect, beforeEach } from 'vitest'
import { SimulationControls } from './SimulationControls'
import { useSimulationStore } from '../../../stores/simulationStore'

describe('SimulationControls (status strip)', () => {
  beforeEach(() => {
    useSimulationStore.getState().reset()
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

  it('shows running status with elapsed time', () => {
    useSimulationStore.setState({ status: 'running', state: { t: 42.5, vessels: [] } })
    render(<SimulationControls />)
    expect(screen.getByText(/42\.5s/)).toBeInTheDocument()
  })

  it('shows error status text', () => {
    useSimulationStore.setState({ status: 'error' })
    render(<SimulationControls />)
    expect(screen.getByText(/erro/i)).toBeInTheDocument()
  })

  it('warns when the physics engine fell back to the mock', () => {
    useSimulationStore.setState({ status: 'running', state: { t: 1, vessels: [] }, engine: 'mock' })
    render(<SimulationControls />)
    expect(screen.getByRole('alert')).toHaveTextContent(/mock/i)
    expect(screen.getByRole('alert')).toHaveTextContent(/build:wasm/)
  })

  it('confirms when real WASM physics is loaded', () => {
    useSimulationStore.setState({ status: 'running', state: { t: 1, vessels: [] }, engine: 'wasm' })
    render(<SimulationControls />)
    expect(screen.getByText(/Física real \(WASM\)/i)).toBeInTheDocument()
    expect(screen.queryByRole('alert')).toBeNull()
  })

  it('shows no engine badge before the worker reports ready', () => {
    useSimulationStore.setState({ status: 'idle', engine: null })
    render(<SimulationControls />)
    expect(screen.queryByText(/Física/i)).toBeNull()
  })
})
