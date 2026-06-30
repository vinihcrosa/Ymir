import { useMemo, useState } from 'react'
import { useNavigate } from 'react-router-dom'
import type { ScenarioDTO } from '@ymir/types'
import { useScenarios } from './use-scenarios'
import { ScenarioCard } from './ScenarioCard'
import { Button } from '../../ui/Button'
import { tokens } from '../../theme/tokens'

/** Landing screen: lists saved scenarios with search + create (Figma "Meus cenários"). */
export function MyScenariosPage() {
  const { scenarios, loading, error, remove } = useScenarios()
  const [query, setQuery] = useState('')
  const navigate = useNavigate()

  const filtered = useMemo(
    () => scenarios.filter(s => s.name.toLowerCase().includes(query.toLowerCase())),
    [scenarios, query],
  )

  return (
    <div style={{ display: 'flex', height: '100vh', overflow: 'hidden' }}>
      <aside style={{ width: tokens.size.sidebar, minWidth: tokens.size.sidebar, background: tokens.color.surface, borderRight: `1px solid ${tokens.color.border}`, padding: tokens.space.lg, display: 'flex', flexDirection: 'column', gap: tokens.space.md }}>
        <div style={{ fontSize: tokens.fontSize.sm, fontWeight: tokens.fontWeight.bold, letterSpacing: '0.08em', color: tokens.color.textSubtle }}>INSTRUCTOR</div>
        <nav style={{ display: 'flex', flexDirection: 'column', gap: tokens.space.xs }}>
          <span style={{ padding: `${tokens.space.sm}px ${tokens.space.md}px`, borderRadius: tokens.radius.md, background: tokens.color.surfaceActive, fontWeight: tokens.fontWeight.medium, fontSize: tokens.fontSize.body }}>Meus cenários</span>
          <span style={{ padding: `${tokens.space.sm}px ${tokens.space.md}px`, color: tokens.color.textSubtle, fontSize: tokens.fontSize.body }}>Configurações</span>
        </nav>
      </aside>

      <main style={{ flex: 1, overflowY: 'auto', padding: tokens.space.xxl, background: tokens.color.surfaceAlt }}>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: tokens.space.lg }}>
          <h1 style={{ margin: 0, fontSize: tokens.fontSize.panelTitle, fontWeight: tokens.fontWeight.bold }}>Meus cenários</h1>
          <Button variant="primary" onClick={() => navigate('/editor')}>+ Novo cenário</Button>
        </div>

        <input
          type="search"
          value={query}
          onChange={e => setQuery(e.target.value)}
          placeholder="Buscar cenários"
          aria-label="Buscar cenários"
          style={{ width: '100%', maxWidth: 360, height: tokens.size.inputHeight, padding: `0 ${tokens.space.md}px`, marginBottom: tokens.space.lg, border: `1px solid ${tokens.color.border}`, borderRadius: tokens.radius.button, fontSize: tokens.fontSize.body, fontFamily: tokens.font.sans }}
        />

        {loading && <p style={{ color: tokens.color.textSubtle }}>Carregando cenários...</p>}
        {error && <p style={{ color: tokens.color.danger }}>Erro ao carregar cenários.</p>}
        {!loading && !error && filtered.length === 0 && (
          <p style={{ color: tokens.color.textHcSubtle }}>
            {scenarios.length === 0 ? 'Nenhum cenário criado ainda.' : 'Nenhum cenário encontrado.'}
          </p>
        )}

        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(220px, 1fr))', gap: tokens.space.lg }}>
          {filtered.map(s => (
            <ScenarioCard
              key={s.id}
              scenario={s}
              onOpen={(sc: ScenarioDTO) => navigate(`/editor?scenario=${sc.id}`)}
              onDelete={(sc: ScenarioDTO) => void remove(sc.id)}
            />
          ))}
        </div>
      </main>
    </div>
  )
}
