import { describe, it, expect, vi } from 'vitest'
import { render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { Panel } from './Panel'

describe('Panel', () => {
  it('renders title, eyebrow and children', () => {
    render(
      <Panel eyebrow="Embarcação Ownship" title="Detalhes">
        <p>Conteúdo do painel</p>
      </Panel>,
    )

    expect(screen.getByText('Embarcação Ownship')).toBeInTheDocument()
    expect(screen.getByText('Detalhes')).toBeInTheDocument()
    expect(screen.getByText('Conteúdo do painel')).toBeInTheDocument()
    expect(screen.getByTestId('panel')).toBeInTheDocument()
  })

  it('fires onClose when the close button is clicked', async () => {
    const user = userEvent.setup()
    const onClose = vi.fn()
    render(<Panel title="Detalhes" onClose={onClose} />)

    const closeButton = screen.getByRole('button', { name: 'Fechar painel' })
    await user.click(closeButton)

    expect(onClose).toHaveBeenCalledTimes(1)
  })

  it('does not render a close button when onClose is omitted', () => {
    render(<Panel title="Detalhes" />)
    expect(screen.queryByRole('button', { name: 'Fechar painel' })).toBeNull()
  })

  it('renders rightActions in the header', () => {
    render(
      <Panel title="Detalhes" rightActions={<button>Ação</button>}>
        body
      </Panel>,
    )
    expect(screen.getByRole('button', { name: 'Ação' })).toBeInTheDocument()
  })
})
