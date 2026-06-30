import type { ScenarioDTO } from '@ymir/types'
import { tokens } from '../../theme/tokens'

export interface ScenarioCardProps {
  scenario: ScenarioDTO
  onOpen: (s: ScenarioDTO) => void
  onDelete: (s: ScenarioDTO) => void
}

/** Grid card for a saved scenario (Figma "Meus cenários"). */
export function ScenarioCard({ scenario, onOpen, onDelete }: ScenarioCardProps) {
  const date = new Date(scenario.createdAt)
  const dateLabel = isNaN(date.getTime()) ? '' : date.toLocaleDateString('pt-BR')

  return (
    <div
      data-testid="scenario-card"
      style={{
        background: tokens.color.surface,
        border: `1px solid ${tokens.color.border}`,
        borderRadius: tokens.radius.panel,
        overflow: 'hidden',
        display: 'flex',
        flexDirection: 'column',
        boxShadow: tokens.shadow.sm,
      }}
    >
      <button
        type="button"
        onClick={() => onOpen(scenario)}
        aria-label={`Abrir cenário ${scenario.name}`}
        style={{
          border: 'none', padding: 0, cursor: 'pointer', textAlign: 'left',
          background: tokens.color.surfaceAlt, height: 120,
          display: 'flex', alignItems: 'center', justifyContent: 'center',
          color: tokens.color.textHcSubtle, fontSize: tokens.fontSize.title,
        }}
      >
        ⚓
      </button>
      <div style={{ padding: tokens.space.md, display: 'flex', flexDirection: 'column', gap: tokens.space.xs }}>
        <div style={{ fontSize: tokens.fontSize.body, fontWeight: tokens.fontWeight.semibold, color: tokens.color.textPrimary, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
          {scenario.name}
        </div>
        {scenario.description && (
          <div style={{ fontSize: tokens.fontSize.label, color: tokens.color.textSubtle, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
            {scenario.description}
          </div>
        )}
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginTop: tokens.space.xs }}>
          <span className="mono" style={{ fontSize: tokens.fontSize.xs, color: tokens.color.textHcSubtle }}>{dateLabel}</span>
          <button
            type="button"
            onClick={() => onDelete(scenario)}
            aria-label={`Excluir cenário ${scenario.name}`}
            style={{ background: 'none', border: 'none', cursor: 'pointer', color: tokens.color.textSubtle, fontSize: tokens.fontSize.body }}
          >
            🗑
          </button>
        </div>
      </div>
    </div>
  )
}
