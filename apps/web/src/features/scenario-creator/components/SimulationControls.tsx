import { useSimulationStore } from '../../../stores/simulationStore'
import { useScenarioStore } from '../store'

const API_BASE = import.meta.env.VITE_API_URL ?? 'http://localhost:3000'

export function SimulationControls() {
  const { status, state, start, stop, reset, loadScenario } = useSimulationStore()
  const { areaId, vessels, toCreateScenarioDTO } = useScenarioStore()

  const canStart = areaId !== null && vessels.length > 0 && status !== 'running' && status !== 'loading'
  const isRunning = status === 'running'

  async function handleStart() {
    const dto = toCreateScenarioDTO()
    try {
      const res = await fetch(`${API_BASE}/scenarios`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(dto),
      })
      if (!res.ok) {
        console.error('Failed to save scenario:', res.status)
      }
    } catch (err) {
      console.error('Network error saving scenario:', err)
    }
    loadScenario(vessels)
    start()
  }

  return (
    <div style={{ borderTop: '1px solid #ddd', paddingTop: '1rem' }}>
      <div style={{ display: 'flex', gap: '0.5rem', marginBottom: '0.5rem' }}>
        {!isRunning ? (
          <button
            type="button"
            onClick={handleStart}
            disabled={!canStart}
            style={{ flex: 1, padding: '0.75rem', background: canStart ? '#1a73e8' : '#ccc', color: 'white', border: 'none', borderRadius: '4px', cursor: canStart ? 'pointer' : 'default' }}
          >
            Iniciar Simulação
          </button>
        ) : (
          <button
            type="button"
            onClick={stop}
            style={{ flex: 1, padding: '0.75rem', background: '#dc2626', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer' }}
          >
            Parar Simulação
          </button>
        )}
        <button type="button" onClick={reset} style={{ padding: '0.75rem', border: '1px solid #ccc', borderRadius: '4px', cursor: 'pointer' }} title="Resetar">↺</button>
      </div>
      <div style={{ fontSize: '0.875rem', color: '#666' }}>
        {status === 'idle' && 'Aguardando configuração'}
        {status === 'loading' && '⌛ Iniciando...'}
        {status === 'ready' && '✓ Pronto'}
        {status === 'running' && state && `▶ Rodando — t = ${state.t.toFixed(1)}s`}
        {status === 'running' && !state && '▶ Rodando'}
        {status === 'error' && <span style={{ color: 'red' }}>⚠ Erro na simulação</span>}
      </div>
    </div>
  )
}
