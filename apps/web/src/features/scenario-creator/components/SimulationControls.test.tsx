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

  it('shows paused status text with elapsed time preserved', () => {
    useSimulationStore.setState({ status: 'paused', state: { t: 12.3, vessels: [] } })
    render(<SimulationControls />)
    expect(screen.getByText(/Pausado/i)).toBeInTheDocument()
    expect(screen.getByText(/12\.3s/)).toBeInTheDocument()
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

  it('warns when the connection is reconnecting', () => {
    useSimulationStore.setState({ status: 'running', state: { t: 1, vessels: [] }, connection: 'reconnecting' })
    render(<SimulationControls />)
    expect(screen.getByRole('alert')).toHaveTextContent(/reconect/i)
  })

  it('confirms when connected to the server', () => {
    useSimulationStore.setState({ status: 'running', state: { t: 1, vessels: [] }, connection: 'open' })
    render(<SimulationControls />)
    expect(screen.getByText(/Conectado ao servidor/i)).toBeInTheDocument()
    expect(screen.queryByRole('alert')).toBeNull()
  })

  it('shows no connection badge before a session exists', () => {
    useSimulationStore.setState({ status: 'idle', connection: 'idle' })
    render(<SimulationControls />)
    expect(screen.queryByText(/servidor/i)).toBeNull()
  })
})
