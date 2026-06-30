import { describe, it, expect, vi } from 'vitest'
import { render, screen, fireEvent } from '@testing-library/react'
import { SimulationControl } from './SimulationControl'

describe('SimulationControl', () => {
  it('shows Play when not running and fires onPlay', () => {
    const onPlay = vi.fn()
    render(<SimulationControl running={false} onPlay={onPlay} onPause={() => {}} />)
    const btn = screen.getByRole('button', { name: /Reproduzir/i })
    expect(btn).toBeInTheDocument()
    expect(screen.queryByRole('button', { name: /Pausar/i })).toBeNull()
    fireEvent.click(btn)
    expect(onPlay).toHaveBeenCalled()
  })

  it('shows Pause when running and fires onPause', () => {
    const onPause = vi.fn()
    render(<SimulationControl running onPlay={() => {}} onPause={onPause} />)
    const btn = screen.getByRole('button', { name: /Pausar/i })
    expect(btn).toBeInTheDocument()
    expect(screen.queryByRole('button', { name: /Reproduzir/i })).toBeNull()
    fireEvent.click(btn)
    expect(onPause).toHaveBeenCalled()
  })

  it('respects disabled', () => {
    const onPlay = vi.fn()
    render(<SimulationControl running={false} disabled onPlay={onPlay} onPause={() => {}} />)
    fireEvent.click(screen.getByRole('button', { name: /Reproduzir/i }))
    expect(onPlay).not.toHaveBeenCalled()
  })
})
