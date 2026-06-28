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

  it('renders scenario name input', () => {
    render(<ScenarioForm />)
    expect(screen.getByRole('textbox')).toBeInTheDocument()
  })

  it('renders area select', () => {
    render(<ScenarioForm />)
    expect(screen.getByRole('combobox')).toBeInTheDocument()
    expect(screen.getByText('Baía de Guanabara')).toBeInTheDocument()
  })

  it('updates name on input change', () => {
    render(<ScenarioForm />)
    const input = screen.getByRole('textbox')
    fireEvent.change(input, { target: { value: 'Novo Cenário' } })
    expect(useScenarioStore.getState().name).toBe('Novo Cenário')
  })

  it('sets area on select change', () => {
    render(<ScenarioForm />)
    const select = screen.getByRole('combobox')
    fireEvent.change(select, { target: { value: '1' } })
    expect(useScenarioStore.getState().areaId).toBe(1)
  })
})
