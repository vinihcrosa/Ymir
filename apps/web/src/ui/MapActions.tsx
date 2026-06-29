import type { CSSProperties, HTMLAttributes } from 'react'
import { tokens } from '../theme/tokens'

export interface MapActionsProps extends HTMLAttributes<HTMLDivElement> {
  /** Called when the "+" (zoom in) button is pressed. */
  onZoomIn: () => void
  /** Called when the "−" (zoom out) button is pressed. */
  onZoomOut: () => void
}

const buttonStyle: CSSProperties = {
  display: 'inline-flex',
  alignItems: 'center',
  justifyContent: 'center',
  width: tokens.size.iconButton,
  height: tokens.size.iconButton,
  padding: 0,
  background: 'transparent',
  border: 'none',
  color: tokens.color.textSecondary,
  fontSize: tokens.fontSize.title,
  fontFamily: tokens.font.sans,
  lineHeight: 1,
  cursor: 'pointer',
}

/**
 * Bottom-right vertical zoom pill for the instructor map. Renders two stacked
 * real <button>s ("+" / "−") separated by a divider, inside a white pill with a
 * subtle shadow. Styling is inline and reads directly from the design tokens.
 */
export function MapActions({
  onZoomIn,
  onZoomOut,
  className,
  style,
  ...rest
}: MapActionsProps) {
  const container: CSSProperties = {
    position: 'absolute',
    right: tokens.space.lg,
    bottom: tokens.space.lg,
    display: 'inline-flex',
    flexDirection: 'column',
    background: tokens.color.surface,
    borderRadius: tokens.radius.pill,
    boxShadow: tokens.shadow.sm,
    overflow: 'hidden',
  }

  const divider: CSSProperties = {
    height: 1,
    background: tokens.color.border,
    margin: `0 ${tokens.space.sm}px`,
  }

  return (
    <div
      role="group"
      aria-label="Controles de zoom do mapa"
      className={className}
      style={{ ...container, ...style }}
      {...rest}
    >
      <button type="button" aria-label="Aproximar" style={buttonStyle} onClick={onZoomIn}>
        +
      </button>
      <div style={divider} aria-hidden="true" />
      <button type="button" aria-label="Afastar" style={buttonStyle} onClick={onZoomOut}>
        −
      </button>
    </div>
  )
}
