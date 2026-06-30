import { describe, it, expect } from 'vitest'
import { render, screen } from '@testing-library/react'
import { Card } from './Card'
import { tokens } from '../theme/tokens'

describe('Card', () => {
  it('renders the title and children', () => {
    render(
      <Card title="Condições ambientais">
        <p>Conteúdo do cartão</p>
      </Card>,
    )
    expect(screen.getByText('Condições ambientais')).toBeInTheDocument()
    expect(screen.getByText('Conteúdo do cartão')).toBeInTheDocument()
  })

  // jsdom normalizes hex colors to rgb(...) in the computed style string.
  const toRgb = (hex: string) => {
    const n = parseInt(hex.slice(1), 16)
    return `rgb(${(n >> 16) & 255}, ${(n >> 8) & 255}, ${n & 255})`
  }

  it('uses the surface background by default', () => {
    const { container } = render(<Card>corpo</Card>)
    const root = container.firstElementChild as HTMLElement
    expect(root.style.background).toBe(toRgb(tokens.color.surface))
  })

  it('sets the surfaceAlt background when tone is alt', () => {
    const { container } = render(<Card tone="alt">corpo</Card>)
    const root = container.firstElementChild as HTMLElement
    expect(root.style.background).toBe(toRgb(tokens.color.surfaceAlt))
  })

  it('composes with Card.Header and Card.Body', () => {
    render(
      <Card>
        <Card.Header>Cabeçalho</Card.Header>
        <Card.Body>Corpo composto</Card.Body>
      </Card>,
    )
    expect(screen.getByText('Cabeçalho')).toBeInTheDocument()
    expect(screen.getByText('Corpo composto')).toBeInTheDocument()
  })

  it('merges custom className and style onto the root', () => {
    const { container } = render(
      <Card className="extra" style={{ marginTop: 8 }}>
        x
      </Card>,
    )
    const root = container.firstElementChild as HTMLElement
    expect(root.classList.contains('extra')).toBe(true)
    expect(root.style.marginTop).toBe('8px')
  })
})
