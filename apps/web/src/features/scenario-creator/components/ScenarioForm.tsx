import { useScenarioStore } from '../store'
import { useAreas } from '../hooks/use-areas'

export function ScenarioForm() {
  const { name, areaId, setName, setArea } = useScenarioStore()
  const { areas, loading, error, retry } = useAreas()

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '0.75rem' }}>
      <div>
        <label style={{ display: 'block', marginBottom: '0.25rem', fontWeight: 600 }}>
          Nome do cenário
        </label>
        <input
          type="text"
          value={name}
          onChange={e => setName(e.target.value)}
          style={{ width: '100%', padding: '0.5rem', boxSizing: 'border-box' }}
        />
      </div>
      <div>
        <label style={{ display: 'block', marginBottom: '0.25rem', fontWeight: 600 }}>
          Área
        </label>
        {error ? (
          <div>
            <span style={{ color: 'red', fontSize: '0.875rem' }}>Erro ao carregar áreas.</span>
            <button type="button" onClick={retry} style={{ marginLeft: '0.5rem' }}>
              Tentar novamente
            </button>
          </div>
        ) : (
          <select
            value={areaId ?? ''}
            disabled={loading}
            onChange={e => {
              const found = areas.find(a => a.id === Number(e.target.value))
              if (found) setArea(found)
            }}
            style={{ width: '100%', padding: '0.5rem' }}
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
