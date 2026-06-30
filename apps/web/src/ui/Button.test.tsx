import { render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { describe, it, expect, vi } from 'vitest'
import { Button } from './Button'
import { tokens } from '../theme/tokens'

describe('Button', () => {
  it('renders its label', () => {
    render(<Button>Salvar cenário</Button>)
    expect(screen.getByRole('button', { name: 'Salvar cenário' })).toBeInTheDocument()
  })

  it('fires onClick when pressed', async () => {
    const onClick = vi.fn()
    render(<Button onClick={onClick}>Adicionar</Button>)
    await userEvent.click(screen.getByRole('button'))
    expect(onClick).toHaveBeenCalledTimes(1)
  })

  it('does not fire onClick when disabled', async () => {
    const onClick = vi.fn()
    render(
      <Button disabled onClick={onClick}>
        Adicionar
      </Button>,
    )
    await userEvent.click(screen.getByRole('button'))
    expect(onClick).not.toHaveBeenCalled()
  })

  it('sets disabled when loading', () => {
    render(<Button loading>Salvando</Button>)
    const button = screen.getByRole('button')
    expect(button).toBeDisabled()
    expect(button).toHaveAttribute('aria-busy', 'true')
  })

  it('reflects the primary variant background', () => {
    render(<Button variant="primary">Primário</Button>)
    expect(screen.getByRole('button')).toHaveStyle({ background: tokens.color.accent })
  })

  it('reflects the dark variant background', () => {
    render(<Button variant="dark">Salvar cenário</Button>)
    expect(screen.getByRole('button')).toHaveStyle({ background: tokens.color.surfaceDark })
  })

  it('reflects the danger variant background', () => {
    render(<Button variant="danger">Excluir</Button>)
    expect(screen.getByRole('button')).toHaveStyle({ background: tokens.color.danger })
  })

  it('uses a not-allowed cursor when disabled', () => {
    render(<Button disabled>Indisponível</Button>)
    expect(screen.getByRole('button')).toHaveStyle({ cursor: 'not-allowed' })
  })

  it('defaults to type button', () => {
    render(<Button>Ação</Button>)
    expect(screen.getByRole('button')).toHaveAttribute('type', 'button')
  })

  it('stretches when fullWidth', () => {
    render(<Button fullWidth>Largo</Button>)
    expect(screen.getByRole('button')).toHaveStyle({ width: '100%' })
  })

  it('spreads extra props such as aria-label to the button', () => {
    render(<Button aria-label="fechar" icon={<svg />} />)
    expect(screen.getByRole('button', { name: 'fechar' })).toBeInTheDocument()
  })
})
