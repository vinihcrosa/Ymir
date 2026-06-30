import type { CSSProperties } from 'react'
import { useScenarioStore } from '../store'
import { useAreas } from '../hooks/use-areas'
import { tokens } from '../../../theme/tokens'

const labelStyle: CSSProperties = {
  display: 'block',
  marginBottom: tokens.space.xs,
  fontSize: tokens.fontSize.label,
  fontWeight: tokens.fontWeight.semibold,
  color: tokens.color.textSecondary,
}

const controlStyle: CSSProperties = {
  width: '100%',
  height: tokens.size.inputHeight,
  padding: `0 ${tokens.space.md}px`,
  boxSizing: 'border-box',
  fontFamily: tokens.font.sans,
  fontSize: tokens.fontSize.body,
  color: tokens.color.textPrimary,
  background: tokens.color.surface,
  border: `1px solid ${tokens.color.border}`,
  borderRadius: tokens.radius.button,
}

export function ScenarioForm() {
  const { areaId, setArea } = useScenarioStore()
  const { areas, loading, error, retry } = useAreas()

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: tokens.space.md }}>
      <div>
        <label style={labelStyle}>Área</label>
        {error ? (
          <div style={{ display: 'flex', alignItems: 'center', gap: tokens.space.sm }}>
            <span style={{ color: tokens.color.danger, fontSize: tokens.fontSize.label }}>Erro ao carregar áreas.</span>
            <button type="button" onClick={retry}>Tentar novamente</button>
          </div>
        ) : (
          <select
            value={areaId ?? ''}
            disabled={loading}
            onChange={e => {
              const found = areas.find(a => a.id === Number(e.target.value))
              if (found) setArea(found)
            }}
            style={controlStyle}
          >
            <option value="">{loading ? 'Carregando áreas...' : 'Selecionar área'}</option>
            {areas.map(a => (
              <option key={a.id} value={a.id}>{a.name}</option>
            ))}
          </select>
        )}
      </div>
    </div>
  )
}
