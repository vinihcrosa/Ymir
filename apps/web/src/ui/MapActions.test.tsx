import { describe, it, expect, vi } from 'vitest'
import { render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { MapActions } from './MapActions'

describe('MapActions', () => {
  it('renders both buttons with their aria-labels', () => {
    render(<MapActions onZoomIn={() => {}} onZoomOut={() => {}} />)
    expect(screen.getByRole('button', { name: 'Aproximar' })).toBeDefined()
    expect(screen.getByRole('button', { name: 'Afastar' })).toBeDefined()
  })

  it('fires onZoomIn when "+" is clicked', async () => {
    const onZoomIn = vi.fn()
    render(<MapActions onZoomIn={onZoomIn} onZoomOut={() => {}} />)
    await userEvent.click(screen.getByRole('button', { name: 'Aproximar' }))
    expect(onZoomIn).toHaveBeenCalledTimes(1)
  })

  it('fires onZoomOut when "−" is clicked', async () => {
    const onZoomOut = vi.fn()
    render(<MapActions onZoomIn={() => {}} onZoomOut={onZoomOut} />)
    await userEvent.click(screen.getByRole('button', { name: 'Afastar' }))
    expect(onZoomOut).toHaveBeenCalledTimes(1)
  })
})
