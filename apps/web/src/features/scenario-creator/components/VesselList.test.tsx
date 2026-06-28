import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { VesselList } from './VesselList'
import { useScenarioStore } from '../store'

vi.mock('../hooks/use-vessels', () => ({
  useVessels: () => ({
    vessels: [{ id: 42, name: 'VLCC Test', lpp: 350, beam: 63, draft: 23 }],
    loading: false,
    error: null,
  }),
}))

const mockArea = { id: 1, name: 'Test', origin: { latitude: -22.9, longitude: -43.2 }, polygon: { type: 'Polygon' as const, coordinates: [] }, gravity: 9.81, magneticCorrection: -22.5, waterDensity: 1.025, airDensity: 0.001275, createdAt: '2024-01-01T00:00:00Z' }

describe('VesselList', () => {
  beforeEach(() => {
    useScenarioStore.getState().reset()
  })

  it('shows empty state message when no vessels', () => {
    render(<VesselList />)
    expect(screen.getByText(/nenhuma embarcação/i)).toBeInTheDocument()
  })

  it('add button is disabled without area', () => {
    render(<VesselList />)
    expect(screen.getByRole('button', { name: /adicionar/i })).toBeDisabled()
  })

  it('add button is enabled with area', () => {
    useScenarioStore.getState().setArea(mockArea)
    render(<VesselList />)
    expect(screen.getByRole('button', { name: /adicionar/i })).not.toBeDisabled()
  })

  it('shows vessel after adding', () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 42, name: 'VLCC Test' })
    render(<VesselList />)
    expect(screen.getByText('VLCC Test')).toBeInTheDocument()
  })

  it('removes vessel on click', () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 42, name: 'VLCC Test' })
    render(<VesselList />)
    fireEvent.click(screen.getByRole('button', { name: /remover/i }))
    expect(useScenarioStore.getState().vessels).toHaveLength(0)
  })

  it('opens vessel picker on add button click', () => {
    useScenarioStore.getState().setArea(mockArea)
    render(<VesselList />)
    fireEvent.click(screen.getByRole('button', { name: /adicionar/i }))
    expect(screen.getByText('VLCC Test')).toBeInTheDocument()
  })

  it('adds vessel from picker and closes it', () => {
    useScenarioStore.getState().setArea(mockArea)
    render(<VesselList />)
    fireEvent.click(screen.getByRole('button', { name: /adicionar/i }))
    fireEvent.click(screen.getByRole('button', { name: 'VLCC Test' }))
    expect(useScenarioStore.getState().vessels).toHaveLength(1)
    expect(useScenarioStore.getState().vessels[0].vesselId).toBe(42)
  })

  it('updates heading via input', () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 42, name: 'VLCC Test' })
    render(<VesselList />)
    fireEvent.change(screen.getByRole('spinbutton', { name: /rumo/i }), { target: { value: '180' } })
    expect(useScenarioStore.getState().vessels[0].headingDeg).toBe(180)
  })
})
