import { describe, it, expect, vi } from 'vitest'
import { render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { SimulationControl } from './SimulationControl'

const noop = () => {}

describe('SimulationControl', () => {
  it('fires onBuild when the build cell is pressed', async () => {
    const onBuild = vi.fn()
    render(<SimulationControl state="idle" onBuild={onBuild} onPlay={noop} onStop={noop} />)
    await userEvent.click(screen.getByRole('button', { name: 'Build simulation' }))
    expect(onBuild).toHaveBeenCalledTimes(1)
  })

  it('uses a custom buildLabel', () => {
    render(
      <SimulationControl state="idle" onBuild={noop} onPlay={noop} onStop={noop} buildLabel="Compilar" />,
    )
    expect(screen.getByRole('button', { name: 'Compilar' })).toBeDefined()
  })

  it('disables Play when idle and enables it when ready', () => {
    const { rerender } = render(
      <SimulationControl state="idle" onBuild={noop} onPlay={noop} onStop={noop} />,
    )
    expect((screen.getByRole('button', { name: 'Reproduzir' }) as HTMLButtonElement).disabled).toBe(true)

    rerender(<SimulationControl state="ready" onBuild={noop} onPlay={noop} onStop={noop} />)
    expect((screen.getByRole('button', { name: 'Reproduzir' }) as HTMLButtonElement).disabled).toBe(false)
  })

  it('fires onPlay when enabled', async () => {
    const onPlay = vi.fn()
    render(<SimulationControl state="ready" onBuild={noop} onPlay={onPlay} onStop={noop} />)
    await userEvent.click(screen.getByRole('button', { name: 'Reproduzir' }))
    expect(onPlay).toHaveBeenCalledTimes(1)
  })

  it('disables Stop unless running', () => {
    const { rerender } = render(
      <SimulationControl state="ready" onBuild={noop} onPlay={noop} onStop={noop} />,
    )
    expect((screen.getByRole('button', { name: 'Parar' }) as HTMLButtonElement).disabled).toBe(true)

    rerender(<SimulationControl state="running" onBuild={noop} onPlay={noop} onStop={noop} />)
    expect((screen.getByRole('button', { name: 'Parar' }) as HTMLButtonElement).disabled).toBe(false)
  })

  it('fires onStop in the running state', async () => {
    const onStop = vi.fn()
    render(<SimulationControl state="running" onBuild={noop} onPlay={noop} onStop={onStop} />)
    await userEvent.click(screen.getByRole('button', { name: 'Parar' }))
    expect(onStop).toHaveBeenCalledTimes(1)
  })

  it('hides the build cell while running and keeps Stop enabled', () => {
    render(<SimulationControl state="running" onBuild={noop} onPlay={noop} onStop={noop} />)
    expect(screen.queryByRole('button', { name: 'Build simulation' })).toBeNull()
    expect((screen.getByRole('button', { name: 'Parar' }) as HTMLButtonElement).disabled).toBe(false)
  })
})
