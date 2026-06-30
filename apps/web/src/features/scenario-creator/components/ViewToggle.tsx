import type { CSSProperties } from 'react'
import { useViewStore, type ViewMode } from '../../../stores/viewStore'
import { tokens } from '../../../theme/tokens'

/** Segmented control (top-left of the workspace) to switch the background between
 *  the 2D map and the 3D scene. Panels are unaffected. */
export function ViewToggle() {
  const { mode, setMode } = useViewStore()

  const seg = (m: ViewMode, label: string): CSSProperties => ({
    padding: `${tokens.space.xs}px ${tokens.space.md}px`,
    border: 'none',
    background: mode === m ? tokens.color.accent : 'transparent',
    color: mode === m ? tokens.color.textOnDark : tokens.color.textSecondary,
    fontFamily: tokens.font.sans,
    fontSize: tokens.fontSize.label,
    fontWeight: tokens.fontWeight.medium,
    cursor: 'pointer',
    borderRadius: tokens.radius.pill,
  })

  return (
    <div
      role="group"
      aria-label="Visualização"
      style={{
        position: 'absolute',
        top: tokens.space.lg,
        left: tokens.space.lg,
        display: 'inline-flex',
        gap: 2,
        padding: 2,
        background: tokens.color.surface,
        border: `1px solid ${tokens.color.border}`,
        borderRadius: tokens.radius.pill,
        boxShadow: tokens.shadow.sm,
        zIndex: tokens.z.panel,
      }}
    >
      <button type="button" style={seg('map', 'Mapa')} aria-pressed={mode === 'map'} onClick={() => setMode('map')}>Mapa</button>
      <button type="button" style={seg('3d', '3D')} aria-pressed={mode === '3d'} onClick={() => setMode('3d')}>3D</button>
    </div>
  )
}
