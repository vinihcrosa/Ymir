import type { CSSProperties, HTMLAttributes, ReactNode } from 'react'
import { tokens } from '../theme/tokens'

/** Which slot clusters are visually/interactively disabled. */
export type TopBarActionsDisabled = 'none' | 'center' | 'centerAndRight'

export interface TopBarProps extends HTMLAttributes<HTMLElement> {
  /** Left cluster (logo, breadcrumb, back button). */
  left?: ReactNode
  /** Center cluster, horizontally centered within the bar. */
  center?: ReactNode
  /** Right cluster (primary actions). */
  right?: ReactNode
  /**
   * Disable interaction with action clusters:
   * - `center`: only the center slot
   * - `centerAndRight`: center and right slots
   */
  actionsDisabled?: TopBarActionsDisabled
}

const rootStyle: CSSProperties = {
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'space-between',
  height: tokens.size.topbar,
  paddingLeft: tokens.space.lg,
  paddingRight: tokens.space.lg,
  backgroundColor: tokens.color.surface,
  borderBottom: `1px solid ${tokens.color.border}`,
  boxShadow: tokens.shadow.sm,
  boxSizing: 'border-box',
  fontFamily: tokens.font.sans,
}

const clusterStyle: CSSProperties = {
  display: 'flex',
  alignItems: 'center',
}

const centerStyle: CSSProperties = {
  ...clusterStyle,
  justifyContent: 'center',
}

const disabledStyle: CSSProperties = {
  opacity: 0.4,
  pointerEvents: 'none',
}

/**
 * Application header bar with left / center / right slots. Fixed 56px height,
 * surface background, bottom border and small shadow per the design tokens.
 */
export function TopBar({
  left,
  center,
  right,
  actionsDisabled = 'none',
  className,
  style,
  ...rest
}: TopBarProps) {
  const centerDisabled = actionsDisabled === 'center' || actionsDisabled === 'centerAndRight'
  const rightDisabled = actionsDisabled === 'centerAndRight'

  return (
    <header
      role="banner"
      className={className}
      style={{ ...rootStyle, ...style }}
      {...rest}
    >
      <div style={clusterStyle}>{left}</div>

      <div
        style={centerDisabled ? { ...centerStyle, ...disabledStyle } : centerStyle}
        aria-disabled={centerDisabled || undefined}
      >
        {center}
      </div>

      <div
        style={rightDisabled ? { ...clusterStyle, ...disabledStyle } : clusterStyle}
        aria-disabled={rightDisabled || undefined}
      >
        {right}
      </div>
    </header>
  )
}
