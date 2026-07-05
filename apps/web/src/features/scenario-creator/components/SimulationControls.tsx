import { useSimulationStore } from '../../../stores/simulationStore'
import { tokens } from '../../../theme/tokens'

/**
 * Compact simulation status strip: lifecycle text + connection to the
 * server-side simulation. The Build/Play/Stop actions live in the floating
 * {@link SimulationControl} pill in the app shell.
 */
export function SimulationControls() {
  const { status, state, connection } = useSimulationStore()

  return (
    <div style={{ borderTop: `1px solid ${tokens.color.border}`, paddingTop: tokens.space.md }}>
      <div style={{ fontSize: tokens.fontSize.label, color: tokens.color.textSubtle }}>
        {status === 'idle' && 'Aguardando configuração'}
        {status === 'loading' && '⌛ Iniciando...'}
        {status === 'paused' && state && `⏸ Pausado — t = ${state.t.toFixed(1)}s`}
        {status === 'paused' && !state && '⏸ Pausado'}
        {status === 'running' && state && `▶ Rodando — t = ${state.t.toFixed(1)}s`}
        {status === 'running' && !state && '▶ Rodando'}
        {status === 'error' && <span style={{ color: tokens.color.danger }}>⚠ Erro na simulação</span>}
      </div>
      {connection === 'reconnecting' && (
        <div
          role="alert"
          style={{ marginTop: tokens.space.sm, fontSize: tokens.fontSize.sm, color: tokens.color.warningFg, background: tokens.color.warningBg, border: `1px solid ${tokens.color.warningFg}33`, borderRadius: tokens.radius.sm, padding: '0.4rem 0.5rem' }}
        >
          ⚠ Reconectando ao servidor de simulação...
        </div>
      )}
      {connection === 'open' && (status === 'paused' || status === 'running') && (
        <div style={{ marginTop: tokens.space.sm, fontSize: tokens.fontSize.sm, color: tokens.color.success }}>
          ● Conectado ao servidor
        </div>
      )}
    </div>
  )
}
