import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { VesselPicker } from './VesselPicker'
import { useScenarioStore } from '../store'

vi.mock('../hooks/use-vessels', () => ({
  useVessels: () => ({
    vessels: [{ id: 42, name: 'VLCC Test', lpp: 350, beam: 63, draft: 23 }],
    loading: false,
    error: null,
  }),
}))

const mockArea = { id: 1, name: 'Test', origin: { latitude: -22.9, longitude: -43.2 }, polygon: { type: 'Polygon' as const, coordinates: [] }, gravity: 9.81, magneticCorrection: -22.5, waterDensity: 1.025, airDensity: 0.001275, createdAt: '2024-01-01T00:00:00Z' }

describe('VesselPicker', () => {
  beforeEach(() => useScenarioStore.getState().reset())

  it('renders nothing when closed', () => {
    const { container } = render(<VesselPicker open={false} onClose={() => {}} />)
    expect(container.querySelector('[data-testid="vessel-picker"]')).toBeNull()
  })

  it('warns to pick an area first when none is selected', () => {
    render(<VesselPicker open onClose={() => {}} />)
    expect(screen.getByText(/Selecione uma área/i)).toBeInTheDocument()
    expect(screen.queryByRole('button', { name: 'VLCC Test' })).toBeNull()
  })

  it('lists available vessels once an area is set', () => {
    useScenarioStore.getState().setArea(mockArea)
    render(<VesselPicker open onClose={() => {}} />)
    expect(screen.getByRole('button', { name: 'VLCC Test' })).toBeInTheDocument()
  })

  it('adds the chosen vessel and closes', () => {
    const onClose = vi.fn()
    useScenarioStore.getState().setArea(mockArea)
    render(<VesselPicker open onClose={onClose} />)
    fireEvent.click(screen.getByRole('button', { name: 'VLCC Test' }))
    expect(useScenarioStore.getState().vessels).toHaveLength(1)
    expect(useScenarioStore.getState().vessels[0].vesselId).toBe(42)
    expect(onClose).toHaveBeenCalled()
  })
})
