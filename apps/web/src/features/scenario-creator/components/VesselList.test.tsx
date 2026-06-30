import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, beforeEach } from 'vitest'
import { VesselList } from './VesselList'
import { useScenarioStore } from '../store'

const mockArea = { id: 1, name: 'Test', origin: { latitude: -22.9, longitude: -43.2 }, polygon: { type: 'Polygon' as const, coordinates: [] }, gravity: 9.81, magneticCorrection: -22.5, waterDensity: 1.025, airDensity: 0.001275, createdAt: '2024-01-01T00:00:00Z' }

describe('VesselList', () => {
  beforeEach(() => {
    useScenarioStore.getState().reset()
  })

  it('shows the empty-state hint pointing at the top-bar add', () => {
    render(<VesselList />)
    expect(screen.getByText(/barra superior/i)).toBeInTheDocument()
  })

  it('shows a vessel after it is added', () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 42, name: 'VLCC Test' })
    render(<VesselList />)
    expect(screen.getByText('VLCC Test')).toBeInTheDocument()
  })

  it('removes a vessel on click', () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 42, name: 'VLCC Test' })
    render(<VesselList />)
    fireEvent.click(screen.getByRole('button', { name: /remover/i }))
    expect(useScenarioStore.getState().vessels).toHaveLength(0)
  })

  it('updates heading via the input', () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 42, name: 'VLCC Test' })
    render(<VesselList />)
    fireEvent.change(screen.getByRole('spinbutton', { name: /rumo/i }), { target: { value: '180' } })
    expect(useScenarioStore.getState().vessels[0].headingDeg).toBe(180)
  })
})
