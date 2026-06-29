import type { CSSProperties, HTMLAttributes, ReactNode } from 'react'
import { tokens } from '../theme/tokens'

export type SimulationState = 'idle' | 'building' | 'ready' | 'running'

export interface SimulationControlProps extends HTMLAttributes<HTMLDivElement> {
  /** Current simulation lifecycle state. */
  state: SimulationState
  /** Fired when the dark "Build simulation" cell is pressed. */
  onBuild: () => void
  /** Fired when the Play cell is pressed (enabled when ready or running). */
  onPlay: () => void
  /** Fired when the Stop cell is pressed (enabled only while running). */
  onStop: () => void
  /** Label for the primary build cell. Defaults to "Build simulation". */
  buildLabel?: string
}

/** Inline icon-cell button, IconButton-like but laid out as a group segment. */
function Cell({
  label,
  icon,
  onClick,
  disabled,
  primary,
}: {
  label: string
  icon: ReactNode
  onClick: () => void
  disabled?: boolean
  primary?: boolean
}) {
  const style: CSSProperties = {
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    gap: tokens.space.sm,
    height: tokens.size.iconButton,
    padding: primary ? `0 ${tokens.space.lg}px` : 0,
    width: primary ? undefined : tokens.size.iconButton,
    border: 'none',
    background: primary ? tokens.color.surfaceDark : 'transparent',
    color: primary ? tokens.color.textOnDark : tokens.color.textSecondary,
    fontFamily: tokens.font.sans,
    fontSize: tokens.fontSize.body,
    fontWeight: tokens.fontWeight.medium,
    cursor: disabled ? 'not-allowed' : 'pointer',
    opacity: disabled ? 0.4 : 1,
    transition: 'background 120ms ease, color 120ms ease',
  }
  return (
    <button
      type="button"
      aria-label={label}
      title={label}
      disabled={disabled}
      onClick={onClick}
      style={style}
    >
      <span aria-hidden="true">{icon}</span>
      {primary ? <span>{label}</span> : null}
    </button>
  )
}

/**
 * Bottom-center floating pill that controls the Build / Play / Stop lifecycle of
 * a simulation. Renders a dark primary "Build simulation" cell followed by Play
 * and Stop icon cells. Styling is inline and reads from the design tokens.
 *
 * Enablement rules:
 * - Play  — enabled when state is 'ready' or 'running'.
 * - Stop  — enabled only while 'running'.
 * - Build — disabled while 'running' (and hidden if not building/ready/idle).
 */
export function SimulationControl({
  state,
  onBuild,
  onPlay,
  onStop,
  buildLabel = 'Build simulation',
  className,
  style,
  ...rest
}: SimulationControlProps) {
  const isRunning = state === 'running'
  const playEnabled = state === 'ready' || state === 'running'
  const stopEnabled = isRunning
  const buildDisabled = isRunning || state === 'building'

  const container: CSSProperties = {
    position: 'fixed',
    bottom: tokens.space.xl,
    left: '50%',
    transform: 'translateX(-50%)',
    display: 'inline-flex',
    alignItems: 'center',
    gap: 0,
    padding: tokens.space.xs,
    background: tokens.color.surface,
    border: `1px solid ${tokens.color.border}`,
    borderRadius: tokens.radius.pill,
    boxShadow: tokens.shadow.sm,
    zIndex: tokens.z.panel,
    ...style,
  }

  return (
    <div role="group" aria-label="Controle de simulação" className={className} style={container} {...rest}>
      {!isRunning ? (
        <Cell
          label={buildLabel}
          icon="🔗"
          primary
          disabled={buildDisabled}
          onClick={onBuild}
        />
      ) : null}
      <Cell label="Reproduzir" icon="▷" disabled={!playEnabled} onClick={onPlay} />
      <Cell label="Parar" icon="▢" disabled={!stopEnabled} onClick={onStop} />
    </div>
  )
}
