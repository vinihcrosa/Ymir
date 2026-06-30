import type { CSSProperties } from 'react'
import { useScenarioStore } from '../scenario-creator/store'
import { Panel } from '../../ui/Panel'
import { Card } from '../../ui/Card'
import { Field } from '../../ui/Field'
import { tokens } from '../../theme/tokens'

export interface ScenarioInfoPanelProps {
  open: boolean
  onClose: () => void
}

const inputStyle: CSSProperties = {
  width: '100%',
  height: tokens.size.inputHeight,
  padding: `0 ${tokens.space.md}px`,
  boxSizing: 'border-box',
  fontFamily: tokens.font.sans,
  fontSize: tokens.fontSize.body,
  border: `1px solid ${tokens.color.border}`,
  borderRadius: tokens.radius.button,
}

/** Right-docked panel with scenario metadata (Figma "Informações do cenário"). */
export function ScenarioInfoPanel({ open, onClose }: ScenarioInfoPanelProps) {
  const { name, area, vessels, setName } = useScenarioStore()
  if (!open) return null

  return (
    <Panel data-testid="scenario-info-panel" eyebrow="Cenário" title="Informações" onClose={onClose}>
      <Card title="Informações do cenário">
        <label style={{ display: 'block', fontSize: tokens.fontSize.label, color: tokens.color.textSubtle, marginBottom: tokens.space.xs }}>
          Nome do cenário
        </label>
        <input style={inputStyle} value={name} aria-label="Nome do cenário" onChange={e => setName(e.target.value)} />
      </Card>

      <Card title="Área da simulação">
        <Field label="Área">{area?.description ?? area?.name ?? '—'}</Field>
        <Field label="Gravidade" mono>{area ? `${area.gravity} m/s²` : '—'}</Field>
        <Field label="Densidade água" mono>{area ? `${area.waterDensity} t/m³` : '—'}</Field>
      </Card>

      <Card title="Simulação">
        <Field label="Embarcações" mono>{vessels.length}</Field>
      </Card>
    </Panel>
  )
}
