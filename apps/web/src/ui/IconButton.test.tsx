import { describe, it, expect, vi } from 'vitest'
import { render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { IconButton } from './IconButton'
import { tokens } from '../theme/tokens'

describe('IconButton', () => {
  it('exposes the label as aria-label', () => {
    render(<IconButton icon={<span>+</span>} label="Adicionar" />)
    expect(screen.getByRole('button', { name: 'Adicionar' })).toBeDefined()
  })

  it('fires onClick when pressed', async () => {
    const onClick = vi.fn()
    render(<IconButton icon={<span>+</span>} label="Adicionar" onClick={onClick} />)
    await userEvent.click(screen.getByRole('button', { name: 'Adicionar' }))
    expect(onClick).toHaveBeenCalledTimes(1)
  })

  it('applies the accent fill for the active variant', () => {
    // jsdom normalizes hex colors to rgb(), so compare via a probe element.
    const probe = document.createElement('div')
    probe.style.background = tokens.color.accentBorder
    probe.style.color = tokens.color.accentText

    render(<IconButton icon={<span>+</span>} label="Ativo" variant="active" />)
    const btn = screen.getByRole('button', { name: 'Ativo' })
    expect(btn.style.background).toBe(probe.style.background)
    expect(btn.style.color).toBe(probe.style.color)
  })

  it('respects the disabled attribute and does not fire onClick', async () => {
    const onClick = vi.fn()
    render(<IconButton icon={<span>+</span>} label="Adicionar" disabled onClick={onClick} />)
    const btn = screen.getByRole('button', { name: 'Adicionar' }) as HTMLButtonElement
    expect(btn.disabled).toBe(true)
    await userEvent.click(btn)
    expect(onClick).not.toHaveBeenCalled()
  })
})
