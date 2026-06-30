import type { CSSProperties, HTMLAttributes } from 'react'
import { tokens } from '../theme/tokens'

export interface SimulationControlProps extends HTMLAttributes<HTMLDivElement> {
  /** Whether the simulation loop is currently advancing. */
  running: boolean
  /** Start (first time) or resume (after pause). */
  onPlay: () => void
  /** Freeze at the current state (does not reset). */
  onPause: () => void
  /** Disable the control (e.g. before a scenario is ready). */
  disabled?: boolean
}

/**
 * Bottom-center floating Play/Pause control. There is no separate "build" step:
 * the first Play boots the engine, and Pause freezes the simulation in place so
 * the instructor can adjust conditions/vessels and resume from the same state.
 */
export function SimulationControl({
  running,
  onPlay,
  onPause,
  disabled,
  className,
  style,
  ...rest
}: SimulationControlProps) {
  const container: CSSProperties = {
    position: 'fixed',
    bottom: tokens.space.xl,
    left: '50%',
    transform: 'translateX(-50%)',
    display: 'inline-flex',
    alignItems: 'center',
    padding: tokens.space.xs,
    background: tokens.color.surface,
    border: `1px solid ${tokens.color.border}`,
    borderRadius: tokens.radius.pill,
    boxShadow: tokens.shadow.sm,
    zIndex: tokens.z.panel,
    ...style,
  }

  const button: CSSProperties = {
    display: 'inline-flex',
    alignItems: 'center',
    gap: tokens.space.sm,
    height: tokens.size.iconButton,
    padding: `0 ${tokens.space.lg}px`,
    border: 'none',
    borderRadius: tokens.radius.pill,
    background: running ? 'transparent' : tokens.color.accent,
    color: running ? tokens.color.textSecondary : tokens.color.textOnDark,
    fontFamily: tokens.font.sans,
    fontSize: tokens.fontSize.body,
    fontWeight: tokens.fontWeight.medium,
    cursor: disabled ? 'not-allowed' : 'pointer',
    opacity: disabled ? 0.4 : 1,
  }

  return (
    <div role="group" aria-label="Controle de simulação" className={className} style={container} {...rest}>
      {running ? (
        <button type="button" style={button} disabled={disabled} aria-label="Pausar" onClick={onPause}>
          <span aria-hidden="true">⏸</span> Pausar
        </button>
      ) : (
        <button type="button" style={button} disabled={disabled} aria-label="Reproduzir" onClick={onPlay}>
          <span aria-hidden="true">▷</span> Reproduzir
        </button>
      )}
    </div>
  )
}
