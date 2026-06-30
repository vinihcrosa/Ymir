import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, beforeEach } from 'vitest'
import { CameraSelector } from './CameraSelector'
import { useViewStore } from '../../../stores/viewStore'

describe('CameraSelector', () => {
  beforeEach(() => useViewStore.setState({ cameraId: 'Front' }))

  it('marks the active camera as pressed', () => {
    render(<CameraSelector />)
    expect(screen.getByRole('button', { name: 'Proa' })).toHaveAttribute('aria-pressed', 'true')
    expect(screen.getByRole('button', { name: 'Popa' })).toHaveAttribute('aria-pressed', 'false')
  })

  it('switches the active onboard camera', () => {
    render(<CameraSelector />)
    fireEvent.click(screen.getByRole('button', { name: 'Bombordo' }))
    expect(useViewStore.getState().cameraId).toBe('Portside')
    fireEvent.click(screen.getByRole('button', { name: 'Ponte' }))
    expect(useViewStore.getState().cameraId).toBe('Bridge')
  })

  it('offers the free-fly camera', () => {
    render(<CameraSelector />)
    fireEvent.click(screen.getByRole('button', { name: 'Livre' }))
    expect(useViewStore.getState().cameraId).toBe('Free')
  })
})
