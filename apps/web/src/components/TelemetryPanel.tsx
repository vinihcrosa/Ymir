import { useSimulationStore } from '../stores/simulationStore.js'
import type { VesselStateDTO } from '@ymir/types'

function fmt(n: number, decimals = 3) {
  return n.toFixed(decimals)
}

function VesselRow({ vessel }: { vessel: VesselStateDTO }) {
  return (
    <tr>
      <td>{vessel.id}</td>
      <td>{fmt(vessel.x)}</td>
      <td>{fmt(vessel.y)}</td>
      <td>{fmt(vessel.psi)}</td>
      <td>{fmt(vessel.u)}</td>
      <td>{fmt(vessel.v)}</td>
      <td>{fmt(vessel.r)}</td>
    </tr>
  )
}

export function TelemetryPanel() {
  const { status, error, state, start, stop, reset } = useSimulationStore()

  return (
    <div style={{ fontFamily: 'monospace', padding: '1rem' }}>
      <h2>Telemetria</h2>
      <div style={{ marginBottom: '0.5rem' }}>
        <span>Status: <strong>{status}</strong></span>
        {state && <span style={{ marginLeft: '1rem' }}>t = {fmt(state.t, 2)} s</span>}
      </div>

      <div style={{ marginBottom: '1rem', display: 'flex', gap: '0.5rem' }}>
        <button onClick={() => start()} disabled={status === 'running' || status === 'loading'}>
          Iniciar
        </button>
        <button onClick={stop} disabled={status !== 'running'}>
          Pausar
        </button>
        <button onClick={reset}>
          Resetar
        </button>
      </div>

      {error && (
        <div style={{ color: 'red', marginBottom: '0.5rem' }}>
          Erro: {error}
        </div>
      )}

      {state && state.vessels.length > 0 ? (
        <table style={{ borderCollapse: 'collapse', width: '100%' }}>
          <thead>
            <tr>
              {['ID', 'x [m]', 'y [m]', 'ψ [rad]', 'u [m/s]', 'v [m/s]', 'r [rad/s]'].map(h => (
                <th key={h} style={{ border: '1px solid #ccc', padding: '4px 8px', textAlign: 'right' }}>{h}</th>
              ))}
            </tr>
          </thead>
          <tbody>
            {state.vessels.map(v => (
              <VesselRow key={v.id} vessel={v} />
            ))}
          </tbody>
        </table>
      ) : (
        <p>Aguardando simulação...</p>
      )}
    </div>
  )
}
