import { describe, it, expect, vi } from 'vitest'
import { render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { Modal, AlertDialog } from './Modal'

describe('Modal', () => {
  it('renders nothing when open=false', () => {
    const { container } = render(
      <Modal open={false} onClose={() => {}}>
        <p>conteúdo</p>
      </Modal>,
    )
    expect(container).toBeEmptyDOMElement()
    expect(screen.queryByText('conteúdo')).toBeNull()
  })

  it('renders children when open', () => {
    render(
      <Modal open onClose={() => {}}>
        <p>conteúdo</p>
      </Modal>,
    )
    expect(screen.getByText('conteúdo')).toBeTruthy()
    expect(screen.getByRole('dialog')).toBeTruthy()
  })

  it('calls onClose when the scrim is clicked', async () => {
    const user = userEvent.setup()
    const onClose = vi.fn()
    render(
      <Modal open onClose={onClose}>
        <p>conteúdo</p>
      </Modal>,
    )
    await user.click(screen.getByTestId('modal-scrim'))
    expect(onClose).toHaveBeenCalledTimes(1)
  })

  it('does not call onClose when the card is clicked', async () => {
    const user = userEvent.setup()
    const onClose = vi.fn()
    render(
      <Modal open onClose={onClose}>
        <p>conteúdo</p>
      </Modal>,
    )
    await user.click(screen.getByText('conteúdo'))
    expect(onClose).not.toHaveBeenCalled()
  })

  it('calls onClose when Escape is pressed', async () => {
    const user = userEvent.setup()
    const onClose = vi.fn()
    render(
      <Modal open onClose={onClose}>
        <p>conteúdo</p>
      </Modal>,
    )
    await user.keyboard('{Escape}')
    expect(onClose).toHaveBeenCalledTimes(1)
  })
})

describe('AlertDialog', () => {
  const baseProps = {
    open: true,
    title: 'Seu cenário não possui um ownship',
    body: 'Deseja continuar mesmo assim?',
    onConfirm: () => {},
    onCancel: () => {},
  }

  it('renders the title and body', () => {
    render(<AlertDialog {...baseProps} />)
    expect(screen.getByText('Seu cenário não possui um ownship')).toBeTruthy()
    expect(screen.getByText('Deseja continuar mesmo assim?')).toBeTruthy()
  })

  it('renders default Portuguese labels and a "?" badge', () => {
    render(<AlertDialog {...baseProps} />)
    expect(screen.getByRole('button', { name: 'Confirmar' })).toBeTruthy()
    expect(screen.getByRole('button', { name: 'Cancelar' })).toBeTruthy()
    expect(screen.getByText('?')).toBeTruthy()
  })

  it('renders nothing when open=false', () => {
    const { container } = render(<AlertDialog {...baseProps} open={false} />)
    expect(container).toBeEmptyDOMElement()
  })

  it('fires onConfirm when the confirm button is clicked', async () => {
    const user = userEvent.setup()
    const onConfirm = vi.fn()
    render(<AlertDialog {...baseProps} onConfirm={onConfirm} />)
    await user.click(screen.getByRole('button', { name: 'Confirmar' }))
    expect(onConfirm).toHaveBeenCalledTimes(1)
  })

  it('fires onCancel when the cancel button is clicked', async () => {
    const user = userEvent.setup()
    const onCancel = vi.fn()
    render(<AlertDialog {...baseProps} onCancel={onCancel} />)
    await user.click(screen.getByRole('button', { name: 'Cancelar' }))
    expect(onCancel).toHaveBeenCalledTimes(1)
  })

  it('honors custom labels and danger variant', () => {
    render(
      <AlertDialog
        {...baseProps}
        confirmLabel="Excluir"
        cancelLabel="Voltar"
        danger
      />,
    )
    expect(screen.getByRole('button', { name: 'Excluir' })).toBeTruthy()
    expect(screen.getByRole('button', { name: 'Voltar' })).toBeTruthy()
  })
})
