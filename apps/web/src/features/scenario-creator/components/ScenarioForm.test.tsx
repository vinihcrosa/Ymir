import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { ScenarioForm } from './ScenarioForm'
import { useScenarioStore } from '../store'

vi.mock('../hooks/use-areas', () => ({
  useAreas: () => ({
    areas: [{ id: 1, name: 'Baía de Guanabara', origin: { latitude: -22.9, longitude: -43.2 }, polygon: { type: 'Polygon', coordinates: [] }, gravity: 9.81, magneticCorrection: -22.5, waterDensity: 1.025, airDensity: 0.001275, createdAt: '2024-01-01T00:00:00Z' }],
    loading: false,
    error: null,
    retry: vi.fn(),
  }),
}))

describe('ScenarioForm', () => {
  beforeEach(() => {
    useScenarioStore.getState().reset()
  })

  it('renders the area select', () => {
    render(<ScenarioForm />)
    expect(screen.getByRole('combobox')).toBeInTheDocument()
    expect(screen.getByText('Baía de Guanabara')).toBeInTheDocument()
  })

  it('sets area on select change', () => {
    render(<ScenarioForm />)
    fireEvent.change(screen.getByRole('combobox'), { target: { value: '1' } })
    expect(useScenarioStore.getState().areaId).toBe(1)
  })

  it('no longer owns the scenario name field (moved to the breadcrumb)', () => {
    render(<ScenarioForm />)
    expect(screen.queryByRole('textbox')).toBeNull()
  })
})
