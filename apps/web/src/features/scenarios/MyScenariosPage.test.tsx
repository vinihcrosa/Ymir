import { render, screen, fireEvent, waitFor } from '@testing-library/react'
import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import { MemoryRouter } from 'react-router-dom'
import { MyScenariosPage } from './MyScenariosPage'

const navigate = vi.fn()
vi.mock('react-router-dom', async (orig) => ({
  ...(await orig<typeof import('react-router-dom')>()),
  useNavigate: () => navigate,
}))

const scenarios = [
  { id: 1, name: 'Aproximação Santos', description: 'Manobra de atracação', duration: 3600, dt: 0.05, initialConditions: [], createdAt: '2026-01-02T00:00:00Z' },
  { id: 2, name: 'Saída Guanabara', duration: 3600, dt: 0.05, initialConditions: [], createdAt: '2026-01-03T00:00:00Z' },
]

function renderPage() {
  return render(<MemoryRouter><MyScenariosPage /></MemoryRouter>)
}

describe('MyScenariosPage', () => {
  beforeEach(() => {
    navigate.mockReset()
    vi.stubGlobal('fetch', vi.fn().mockResolvedValue({ ok: true, json: () => Promise.resolve(scenarios) }))
  })
  afterEach(() => vi.restoreAllMocks())

  it('lists scenarios from the API', async () => {
    renderPage()
    expect(await screen.findByText('Aproximação Santos')).toBeInTheDocument()
    expect(screen.getByText('Saída Guanabara')).toBeInTheDocument()
    expect(screen.getAllByTestId('scenario-card')).toHaveLength(2)
  })

  it('filters by search query', async () => {
    renderPage()
    await screen.findByText('Aproximação Santos')
    fireEvent.change(screen.getByLabelText(/Buscar cenários/i), { target: { value: 'santos' } })
    expect(screen.getByText('Aproximação Santos')).toBeInTheDocument()
    expect(screen.queryByText('Saída Guanabara')).toBeNull()
  })

  it('navigates to the editor on "Novo cenário"', async () => {
    renderPage()
    await screen.findByText('Aproximação Santos')
    fireEvent.click(screen.getByRole('button', { name: /Novo cenário/i }))
    expect(navigate).toHaveBeenCalledWith('/editor')
  })

  it('opening a card navigates to the editor with the scenario id', async () => {
    renderPage()
    await screen.findByText('Aproximação Santos')
    fireEvent.click(screen.getByRole('button', { name: /Abrir cenário Aproximação Santos/i }))
    expect(navigate).toHaveBeenCalledWith('/editor?scenario=1')
  })

  it('deleting a card calls DELETE and removes it', async () => {
    renderPage()
    await screen.findByText('Aproximação Santos')
    fireEvent.click(screen.getByRole('button', { name: /Excluir cenário Aproximação Santos/i }))
    await waitFor(() => expect(fetch).toHaveBeenCalledWith(
      expect.stringContaining('/scenarios/1'),
      expect.objectContaining({ method: 'DELETE' }),
    ))
    await waitFor(() => expect(screen.queryByText('Aproximação Santos')).toBeNull())
  })

  it('shows the empty state when there are no scenarios', async () => {
    vi.stubGlobal('fetch', vi.fn().mockResolvedValue({ ok: true, json: () => Promise.resolve([]) }))
    renderPage()
    expect(await screen.findByText(/Nenhum cenário criado ainda/i)).toBeInTheDocument()
  })
})
