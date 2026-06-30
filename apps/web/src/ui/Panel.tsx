import type { CSSProperties, ReactNode, HTMLAttributes } from 'react'
import { tokens } from '../theme/tokens'

export interface PanelProps extends Omit<HTMLAttributes<HTMLDivElement>, 'title'> {
  /** Main heading rendered in the header (bold, title size). */
  title?: string
  /** Small uppercase label above the title, e.g. "Embarcação Ownship". */
  eyebrow?: string
  /** Optional close handler; renders the "×" button when provided. */
  onClose?: () => void
  /** Extra controls rendered on the right side of the header. */
  rightActions?: ReactNode
  /** Panel width in pixels (default: size.vesselPanel). */
  width?: number
  /** Body content (scrolls). */
  children?: ReactNode
}

/**
 * Floating right-docked card shell for VesselPanel / drawers. Absolutely
 * positioned top/right, fixed max height, scrolling body, hidden overflow.
 */
export function Panel({
  title,
  eyebrow,
  onClose,
  rightActions,
  width = tokens.size.vesselPanel,
  children,
  className,
  style,
  ...rest
}: PanelProps) {
  const rootStyle: CSSProperties = {
    position: 'absolute',
    top: tokens.space.lg,
    right: tokens.space.lg,
    width,
    maxHeight: 'calc(100vh - 32px)',
    display: 'flex',
    flexDirection: 'column',
    overflow: 'hidden',
    background: tokens.color.surface,
    borderRadius: tokens.radius.panel,
    boxShadow: tokens.shadow.panel,
    zIndex: tokens.z.panel,
    fontFamily: tokens.font.sans,
    color: tokens.color.textPrimary,
    ...style,
  }

  const headerStyle: CSSProperties = {
    display: 'flex',
    alignItems: 'flex-start',
    gap: tokens.space.sm,
    padding: tokens.space.lg,
    borderBottom: `1px solid ${tokens.color.border}`,
  }

  const eyebrowStyle: CSSProperties = {
    fontSize: tokens.fontSize.xs,
    fontWeight: tokens.fontWeight.semibold,
    letterSpacing: '0.06em',
    textTransform: 'uppercase',
    color: tokens.color.textSubtle,
    marginBottom: tokens.space.px,
  }

  const titleStyle: CSSProperties = {
    fontSize: tokens.fontSize.title,
    fontWeight: tokens.fontWeight.bold,
    color: tokens.color.textPrimary,
    margin: 0,
    lineHeight: 1.2,
  }

  const actionsStyle: CSSProperties = {
    display: 'flex',
    alignItems: 'center',
    gap: tokens.space.xs,
    marginLeft: 'auto',
  }

  const closeButtonStyle: CSSProperties = {
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    width: tokens.size.iconButton,
    height: tokens.size.iconButton,
    padding: 0,
    border: 'none',
    background: 'transparent',
    borderRadius: tokens.radius.button,
    cursor: 'pointer',
    fontSize: tokens.fontSize.panelTitle,
    lineHeight: 1,
    color: tokens.color.textTertiary,
  }

  const bodyStyle: CSSProperties = {
    overflowY: 'auto',
    padding: tokens.space.lg,
    flex: '1 1 auto',
    minHeight: 0,
  }

  const hasHeader = Boolean(title || eyebrow || rightActions || onClose)

  return (
    <div data-testid="panel" className={className} style={rootStyle} {...rest}>
      {hasHeader && (
        <header style={headerStyle}>
          <div style={{ minWidth: 0 }}>
            {eyebrow && <div style={eyebrowStyle}>{eyebrow}</div>}
            {title && <h2 style={titleStyle}>{title}</h2>}
          </div>
          {(rightActions || onClose) && (
            <div style={actionsStyle}>
              {rightActions}
              {onClose && (
                <button
                  type="button"
                  aria-label="Fechar painel"
                  onClick={onClose}
                  style={closeButtonStyle}
                >
                  ×
                </button>
              )}
            </div>
          )}
        </header>
      )}
      <div style={bodyStyle}>{children}</div>
    </div>
  )
}
