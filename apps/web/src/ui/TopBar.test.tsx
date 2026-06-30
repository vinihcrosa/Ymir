import { describe, it, expect } from 'vitest'
import { render, screen } from '@testing-library/react'
import { TopBar } from './TopBar'

describe('TopBar', () => {
  it('renders the left, center and right slots', () => {
    render(
      <TopBar
        left={<span>esquerda</span>}
        center={<span>centro</span>}
        right={<button type="button">salvar</button>}
      />,
    )
    expect(screen.getByText('esquerda')).toBeInTheDocument()
    expect(screen.getByText('centro')).toBeInTheDocument()
    expect(screen.getByRole('button', { name: 'salvar' })).toBeInTheDocument()
  })

  it("actionsDisabled='center' disables the center slot but not the right slot", () => {
    render(
      <TopBar
        center={<button type="button">centro</button>}
        right={<button type="button">direita</button>}
        actionsDisabled="center"
      />,
    )

    const centerWrapper = screen.getByText('centro').parentElement as HTMLElement
    expect(centerWrapper).toHaveAttribute('aria-disabled', 'true')
    expect(centerWrapper.style.pointerEvents).toBe('none')
    expect(centerWrapper.style.opacity).toBe('0.4')

    const rightWrapper = screen.getByText('direita').parentElement as HTMLElement
    expect(rightWrapper).not.toHaveAttribute('aria-disabled')
    expect(rightWrapper.style.pointerEvents).not.toBe('none')
  })

  it("actionsDisabled='centerAndRight' disables both the center and right slots", () => {
    render(
      <TopBar
        center={<button type="button">centro</button>}
        right={<button type="button">direita</button>}
        actionsDisabled="centerAndRight"
      />,
    )

    const centerWrapper = screen.getByText('centro').parentElement as HTMLElement
    expect(centerWrapper).toHaveAttribute('aria-disabled', 'true')
    expect(centerWrapper.style.pointerEvents).toBe('none')

    const rightWrapper = screen.getByText('direita').parentElement as HTMLElement
    expect(rightWrapper).toHaveAttribute('aria-disabled', 'true')
    expect(rightWrapper.style.pointerEvents).toBe('none')
    expect(rightWrapper.style.opacity).toBe('0.4')
  })

  it('defaults to no disabled slots', () => {
    render(
      <TopBar
        center={<button type="button">centro</button>}
        right={<button type="button">direita</button>}
      />,
    )
    const centerWrapper = screen.getByText('centro').parentElement as HTMLElement
    const rightWrapper = screen.getByText('direita').parentElement as HTMLElement
    expect(centerWrapper).not.toHaveAttribute('aria-disabled')
    expect(rightWrapper).not.toHaveAttribute('aria-disabled')
  })

  it('merges a custom style onto the root header', () => {
    render(<TopBar data-testid="bar" style={{ position: 'sticky' }} />)
    const bar = screen.getByTestId('bar')
    expect(bar.tagName).toBe('HEADER')
    expect(bar.style.position).toBe('sticky')
    expect(bar.style.height).toBe('56px')
  })
})
