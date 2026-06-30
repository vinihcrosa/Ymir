import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, beforeEach } from 'vitest'
import { ViewToggle } from './ViewToggle'
import { useViewStore } from '../../../stores/viewStore'

describe('ViewToggle', () => {
  beforeEach(() => useViewStore.setState({ mode: 'map' }))

  it('defaults to Mapa selected', () => {
    render(<ViewToggle />)
    expect(screen.getByRole('button', { name: 'Mapa' })).toHaveAttribute('aria-pressed', 'true')
    expect(screen.getByRole('button', { name: '3D' })).toHaveAttribute('aria-pressed', 'false')
  })

  it('switches to 3D and back', () => {
    render(<ViewToggle />)
    fireEvent.click(screen.getByRole('button', { name: '3D' }))
    expect(useViewStore.getState().mode).toBe('3d')
    fireEvent.click(screen.getByRole('button', { name: 'Mapa' }))
    expect(useViewStore.getState().mode).toBe('map')
  })
})
