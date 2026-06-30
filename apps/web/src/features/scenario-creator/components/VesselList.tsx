import { useScenarioStore } from '../store'
import { tokens } from '../../../theme/tokens'

/**
 * Management list of vessels already added to the scenario. Adding happens via
 * the top-bar "+" (VesselPicker); this list handles per-vessel heading and
 * removal with a readable two-line row.
 */
export function VesselList() {
  const { vessels, removeVessel, updateVesselHeading } = useScenarioStore()

  return (
    <div>
      <strong style={{ fontSize: tokens.fontSize.body, display: 'block', marginBottom: tokens.space.sm }}>
        Embarcações ({vessels.length})
      </strong>

      {vessels.length === 0 && (
        <p style={{ color: tokens.color.textHcSubtle, fontSize: tokens.fontSize.label }}>
          Use o + na barra superior para adicionar.
        </p>
      )}

      {vessels.map(v => (
        <div
          key={v.instanceId}
          style={{
            border: `1px solid ${tokens.color.border}`,
            borderRadius: tokens.radius.md,
            padding: tokens.space.sm,
            marginBottom: tokens.space.sm,
            background: tokens.color.surface,
          }}
        >
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', gap: tokens.space.xs }}>
            <span style={{ fontSize: tokens.fontSize.label, fontWeight: tokens.fontWeight.medium, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }} title={v.name}>
              {v.name}
            </span>
            <button
              type="button"
              onClick={() => removeVessel(v.instanceId)}
              title="Remover"
              aria-label={`Remover ${v.name}`}
              style={{ background: 'none', border: 'none', cursor: 'pointer', color: tokens.color.textSubtle, fontSize: tokens.fontSize.title, lineHeight: 1, flexShrink: 0 }}
            >
              ×
            </button>
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: tokens.space.sm, marginTop: tokens.space.xs, fontSize: tokens.fontSize.sm, color: tokens.color.textSubtle }}>
            <span className="mono">x {v.x.toFixed(0)}  y {v.y.toFixed(0)} m</span>
            <span style={{ marginLeft: 'auto', display: 'inline-flex', alignItems: 'center', gap: 4 }}>
              Rumo
              <input
                type="number"
                min={0}
                max={360}
                value={v.headingDeg.toFixed(0)}
                onChange={e => updateVesselHeading(v.instanceId, Number(e.target.value))}
                style={{ width: 52, padding: '2px 4px', border: `1px solid ${tokens.color.border}`, borderRadius: tokens.radius.sm, fontFamily: tokens.font.mono }}
                aria-label={`Rumo ${v.name}`}
              />
              °
            </span>
          </div>
        </div>
      ))}
    </div>
  )
}
