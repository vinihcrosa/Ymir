import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { ScenarioInfoPanel } from './ScenarioInfoPanel'
import { useScenarioStore } from '../scenario-creator/store'

const mockArea = { id: 1, name: 'baia_de_guanabara', description: 'Baía de Guanabara', origin: { latitude: -22.9, longitude: -43.2 }, polygon: { type: 'Polygon' as const, coordinates: [] }, gravity: 9.81, magneticCorrection: -22.5, waterDensity: 1.025, airDensity: 0.001275, createdAt: '2024-01-01T00:00:00Z' }

describe('ScenarioInfoPanel', () => {
  beforeEach(() => useScenarioStore.getState().reset())

  it('renders nothing when closed', () => {
    const { container } = render(<ScenarioInfoPanel open={false} onClose={() => {}} />)
    expect(container.querySelector('[data-testid="scenario-info-panel"]')).toBeNull()
  })

  it('shows scenario name, area and vessel count when open', () => {
    useScenarioStore.getState().setArea(mockArea)
    useScenarioStore.getState().addVessel({ vesselId: 1, name: 'VLCC' })
    render(<ScenarioInfoPanel open onClose={() => {}} />)
    expect(screen.getByText('Baía de Guanabara')).toBeInTheDocument()
    expect(screen.getByText('9.81 m/s²')).toBeInTheDocument()
    expect(screen.getByText('1')).toBeInTheDocument() // vessel count
  })

  it('edits the scenario name through the store', () => {
    render(<ScenarioInfoPanel open onClose={() => {}} />)
    fireEvent.change(screen.getByLabelText('Nome do cenário'), { target: { value: 'Treino A' } })
    expect(useScenarioStore.getState().name).toBe('Treino A')
  })

  it('fires onClose from the close button', () => {
    const onClose = vi.fn()
    render(<ScenarioInfoPanel open onClose={onClose} />)
    fireEvent.click(screen.getByRole('button', { name: /Fechar painel/i }))
    expect(onClose).toHaveBeenCalled()
  })
})
