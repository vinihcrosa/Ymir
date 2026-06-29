import { render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { describe, it, expect, vi } from 'vitest'
import { Tabs, type TabItem } from './Tabs'
import { tokens } from '../theme/tokens'

const tabs: TabItem[] = [
  { id: 'geral', label: 'Geral', content: <p>Conteúdo geral</p> },
  { id: 'avancado', label: 'Avançado', content: <p>Conteúdo avançado</p> },
]

describe('Tabs', () => {
  it('renders all tab labels', () => {
    render(<Tabs tabs={tabs} />)
    expect(screen.getByRole('tab', { name: 'Geral' })).toBeInTheDocument()
    expect(screen.getByRole('tab', { name: 'Avançado' })).toBeInTheDocument()
  })

  it('renders only the active tab content', () => {
    render(<Tabs tabs={tabs} />)
    expect(screen.getByText('Conteúdo geral')).toBeInTheDocument()
    expect(screen.queryByText('Conteúdo avançado')).not.toBeInTheDocument()
  })

  it('switches content when a tab is clicked', async () => {
    const user = userEvent.setup()
    render(<Tabs tabs={tabs} />)
    await user.click(screen.getByRole('tab', { name: 'Avançado' }))
    expect(screen.getByText('Conteúdo avançado')).toBeInTheDocument()
    expect(screen.queryByText('Conteúdo geral')).not.toBeInTheDocument()
  })

  it('styles the active tab with accentText color', () => {
    render(<Tabs tabs={tabs} />)
    const active = screen.getByRole('tab', { name: 'Geral' })
    expect(active).toHaveStyle({ color: tokens.color.accentText })
  })

  it('fires onChange with the selected tab id', async () => {
    const user = userEvent.setup()
    const onChange = vi.fn()
    render(<Tabs tabs={tabs} onChange={onChange} />)
    await user.click(screen.getByRole('tab', { name: 'Avançado' }))
    expect(onChange).toHaveBeenCalledWith('avancado')
  })

  it('respects a controlled activeId', async () => {
    const user = userEvent.setup()
    const onChange = vi.fn()
    render(<Tabs tabs={tabs} activeId="geral" onChange={onChange} />)
    await user.click(screen.getByRole('tab', { name: 'Avançado' }))
    // Controlled: content stays put until parent updates activeId
    expect(screen.getByText('Conteúdo geral')).toBeInTheDocument()
    expect(screen.queryByText('Conteúdo avançado')).not.toBeInTheDocument()
    expect(onChange).toHaveBeenCalledWith('avancado')
  })

  it('honors defaultActiveId when uncontrolled', () => {
    render(<Tabs tabs={tabs} defaultActiveId="avancado" />)
    expect(screen.getByText('Conteúdo avançado')).toBeInTheDocument()
  })
})
