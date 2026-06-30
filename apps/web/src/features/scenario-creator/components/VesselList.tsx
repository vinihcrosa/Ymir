import { useState } from 'react'
import { useScenarioStore } from '../store'
import { useVessels } from '../hooks/use-vessels'
import { Button } from '../../../ui/Button'
import { tokens } from '../../../theme/tokens'

export function VesselList() {
  const { area, vessels, addVessel, removeVessel, updateVesselHeading } = useScenarioStore()
  const { vessels: available, loading } = useVessels()
  const [showPicker, setShowPicker] = useState(false)

  return (
    <div>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: tokens.space.sm }}>
        <strong style={{ fontSize: tokens.fontSize.body }}>Embarcações ({vessels.length})</strong>
        <Button variant="ghost" size="sm" disabled={!area} onClick={() => setShowPicker(p => !p)}>+ Adicionar</Button>
      </div>
      {showPicker && (
        <div style={{ background: tokens.color.surfaceAlt, padding: tokens.space.sm, marginBottom: tokens.space.sm, borderRadius: tokens.radius.md, border: `1px solid ${tokens.color.border}` }}>
          {loading ? <span>Carregando...</span> : available.map(v => (
            <button
              key={v.id}
              type="button"
              onClick={() => { addVessel({ vesselId: v.id, name: v.name }); setShowPicker(false) }}
              style={{ display: 'block', width: '100%', textAlign: 'left', padding: tokens.space.xs, marginBottom: tokens.space.xs, background: 'transparent', border: 'none', borderRadius: tokens.radius.sm, cursor: 'pointer', fontSize: tokens.fontSize.label, color: tokens.color.textPrimary }}
            >
              {v.name}
            </button>
          ))}
        </div>
      )}
      {vessels.map(v => (
        <div key={v.instanceId} style={{ display: 'flex', gap: tokens.space.sm, alignItems: 'center', marginBottom: tokens.space.sm, fontSize: tokens.fontSize.label }}>
          <span style={{ flex: 1, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{v.name}</span>
          <span className="mono" style={{ color: tokens.color.textSubtle }}>{v.x.toFixed(1)}, {v.y.toFixed(1)}m</span>
          <input
            type="number"
            min={0}
            max={360}
            value={v.headingDeg.toFixed(0)}
            onChange={e => updateVesselHeading(v.instanceId, Number(e.target.value))}
            style={{ width: 56, padding: tokens.space.xs, border: `1px solid ${tokens.color.border}`, borderRadius: tokens.radius.sm, fontFamily: tokens.font.mono }}
            title="Rumo (°)"
            aria-label="Rumo"
          />
          <span style={{ color: tokens.color.textSubtle }}>°</span>
          <button type="button" onClick={() => removeVessel(v.instanceId)} title="Remover" aria-label="Remover embarcação" style={{ background: 'transparent', border: 'none', cursor: 'pointer', color: tokens.color.textSubtle, fontSize: tokens.fontSize.title }}>×</button>
        </div>
      ))}
      {vessels.length === 0 && <p style={{ color: tokens.color.textHcSubtle, fontSize: tokens.fontSize.label }}>Nenhuma embarcação adicionada.</p>}
    </div>
  )
}
