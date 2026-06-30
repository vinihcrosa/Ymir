import { useSimulationStore } from '../../../stores/simulationStore'
import { tokens } from '../../../theme/tokens'

/**
 * Compact simulation status strip: lifecycle text + which physics engine is
 * actually loaded (real WASM vs JS mock fallback). The Build/Play/Stop actions
 * live in the floating {@link SimulationControl} pill in the app shell.
 */
export function SimulationControls() {
  const { status, state, engine } = useSimulationStore()

  return (
    <div style={{ borderTop: `1px solid ${tokens.color.border}`, paddingTop: tokens.space.md }}>
      <div style={{ fontSize: tokens.fontSize.label, color: tokens.color.textSubtle }}>
        {status === 'idle' && 'Aguardando configuração'}
        {status === 'loading' && '⌛ Iniciando...'}
        {status === 'ready' && '✓ Pronto'}
        {status === 'running' && state && `▶ Rodando — t = ${state.t.toFixed(1)}s`}
        {status === 'running' && !state && '▶ Rodando'}
        {status === 'error' && <span style={{ color: tokens.color.danger }}>⚠ Erro na simulação</span>}
      </div>
      {engine === 'mock' && (status === 'ready' || status === 'running') && (
        <div
          role="alert"
          style={{ marginTop: tokens.space.sm, fontSize: tokens.fontSize.sm, color: tokens.color.warningFg, background: tokens.color.warningBg, border: `1px solid ${tokens.color.warningFg}33`, borderRadius: tokens.radius.sm, padding: '0.4rem 0.5rem' }}
        >
          ⚠ Física simulada (mock) — o módulo WASM não foi compilado. Rode <code>pnpm build:wasm</code> para a dinâmica real.
        </div>
      )}
      {engine === 'wasm' && (status === 'ready' || status === 'running') && (
        <div style={{ marginTop: tokens.space.sm, fontSize: tokens.fontSize.sm, color: tokens.color.success }}>
          ● Física real (WASM)
        </div>
      )}
    </div>
  )
}
