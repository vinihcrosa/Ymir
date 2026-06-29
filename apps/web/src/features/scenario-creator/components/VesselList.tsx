import { useState } from 'react'
import { useScenarioStore } from '../store'
import { useVessels } from '../hooks/use-vessels'

export function VesselList() {
  const { area, vessels, addVessel, removeVessel, updateVesselHeading } = useScenarioStore()
  const { vessels: available, loading } = useVessels()
  const [showPicker, setShowPicker] = useState(false)

  return (
    <div>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '0.5rem' }}>
        <strong>Embarcações ({vessels.length})</strong>
        <button
          type="button"
          disabled={!area}
          onClick={() => setShowPicker(p => !p)}
        >
          + Adicionar
        </button>
      </div>
      {showPicker && (
        <div style={{ background: '#f5f5f5', padding: '0.5rem', marginBottom: '0.5rem', borderRadius: '4px' }}>
          {loading ? <span>Carregando...</span> : available.map(v => (
            <button
              key={v.id}
              type="button"
              onClick={() => { addVessel({ vesselId: v.id, name: v.name }); setShowPicker(false) }}
              style={{ display: 'block', width: '100%', textAlign: 'left', padding: '0.25rem', marginBottom: '0.25rem' }}
            >
              {v.name}
            </button>
          ))}
        </div>
      )}
      {vessels.map(v => (
        <div key={v.instanceId} style={{ display: 'flex', gap: '0.5rem', alignItems: 'center', marginBottom: '0.5rem', fontSize: '0.875rem' }}>
          <span style={{ flex: 1 }}>{v.name}</span>
          <span style={{ color: '#666' }}>{v.x.toFixed(1)}, {v.y.toFixed(1)}m</span>
          <input
            type="number"
            min={0}
            max={360}
            value={v.headingDeg.toFixed(0)}
            onChange={e => updateVesselHeading(v.instanceId, Number(e.target.value))}
            style={{ width: '60px', padding: '0.25rem' }}
            title="Rumo (°)"
            aria-label="Rumo"
          />
          <span>°</span>
          <button type="button" onClick={() => removeVessel(v.instanceId)} title="Remover" aria-label="Remover embarcação">×</button>
        </div>
      ))}
      {vessels.length === 0 && <p style={{ color: '#888', fontSize: '0.875rem' }}>Nenhuma embarcação adicionada.</p>}
    </div>
  )
}
