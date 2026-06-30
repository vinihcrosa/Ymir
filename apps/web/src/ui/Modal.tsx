import { useEffect, type CSSProperties, type ReactNode } from 'react'
import { tokens } from '../theme/tokens'

export interface ModalProps {
  /** Whether the modal is visible. When false, nothing is rendered. */
  open: boolean
  /** Called on scrim click or Escape key. */
  onClose: () => void
  children: ReactNode
  /** Card width in pixels. Defaults to 420. */
  width?: number
  className?: string
  style?: CSSProperties
}

const scrimStyle: CSSProperties = {
  position: 'fixed',
  inset: 0,
  backgroundColor: tokens.color.scrim,
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  zIndex: tokens.z.modal,
}

const cardStyle: CSSProperties = {
  backgroundColor: tokens.color.surface,
  borderRadius: tokens.radius.panel,
  boxShadow: tokens.shadow.panel,
  padding: tokens.space.xl,
  fontFamily: tokens.font.sans,
  color: tokens.color.textPrimary,
  boxSizing: 'border-box',
  maxWidth: '90vw',
}

/**
 * Centered modal dialog rendered over a full-viewport scrim. Clicking the scrim
 * (outside the card) or pressing Escape invokes {@link ModalProps.onClose}.
 */
export function Modal({
  open,
  onClose,
  children,
  width = 420,
  className,
  style,
  ...rest
}: ModalProps): React.JSX.Element | null {
  useEffect(() => {
    if (!open) return
    const onKeyDown = (e: KeyboardEvent): void => {
      if (e.key === 'Escape') onClose()
    }
    document.addEventListener('keydown', onKeyDown)
    return () => document.removeEventListener('keydown', onKeyDown)
  }, [open, onClose])

  if (!open) return null

  return (
    <div
      style={scrimStyle}
      onClick={onClose}
      data-testid="modal-scrim"
    >
      <div
        role="dialog"
        aria-modal="true"
        className={className}
        style={{ ...cardStyle, width, ...style }}
        onClick={(e) => e.stopPropagation()}
        {...rest}
      >
        {children}
      </div>
    </div>
  )
}

export interface AlertDialogProps {
  /** Whether the dialog is visible. When false, nothing is rendered. */
  open: boolean
  title: string
  body: string
  /** Confirm button label. Defaults to "Confirmar". */
  confirmLabel?: string
  /** Cancel button label. Defaults to "Cancelar". */
  cancelLabel?: string
  onConfirm: () => void
  onCancel: () => void
  /** Render the confirm button with the danger styling. */
  danger?: boolean
}

const badgeStyle: CSSProperties = {
  width: 32,
  height: 32,
  borderRadius: tokens.radius.pill,
  backgroundColor: tokens.color.surfaceAlt,
  border: `1px solid ${tokens.color.border}`,
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  fontSize: tokens.fontSize.title,
  fontWeight: tokens.fontWeight.bold,
  color: tokens.color.textSecondary,
  flexShrink: 0,
}

const titleStyle: CSSProperties = {
  margin: 0,
  fontSize: tokens.fontSize.title,
  fontWeight: tokens.fontWeight.bold,
  color: tokens.color.textPrimary,
}

const bodyStyle: CSSProperties = {
  margin: `${tokens.space.sm}px 0 0`,
  fontSize: tokens.fontSize.body,
  color: tokens.color.textSecondary,
  lineHeight: 1.5,
}

const footerStyle: CSSProperties = {
  display: 'flex',
  justifyContent: 'flex-end',
  gap: tokens.space.sm,
  marginTop: tokens.space.xl,
}

const buttonBase: CSSProperties = {
  height: 36,
  padding: `0 ${tokens.space.lg}px`,
  borderRadius: tokens.radius.button,
  fontFamily: tokens.font.sans,
  fontSize: tokens.fontSize.body,
  fontWeight: tokens.fontWeight.medium,
  cursor: 'pointer',
}

const cancelButtonStyle: CSSProperties = {
  ...buttonBase,
  backgroundColor: tokens.color.surface,
  color: tokens.color.textSecondary,
  border: `1px solid ${tokens.color.border}`,
}

/**
 * Confirmation dialog built on {@link Modal}. Shows a "?" badge, bold title,
 * body text and a footer with a secondary Cancel button plus a dark (or danger)
 * Confirm button. Used e.g. for "Seu cenário não possui um ownship".
 */
export function AlertDialog({
  open,
  title,
  body,
  confirmLabel = 'Confirmar',
  cancelLabel = 'Cancelar',
  onConfirm,
  onCancel,
  danger,
}: AlertDialogProps): React.JSX.Element | null {
  const confirmButtonStyle: CSSProperties = {
    ...buttonBase,
    backgroundColor: danger ? tokens.color.danger : tokens.color.surfaceDark,
    color: tokens.color.textOnDark,
    border: '1px solid transparent',
  }

  return (
    <Modal open={open} onClose={onCancel} width={420}>
      <div role="alertdialog" aria-labelledby="alert-dialog-title">
        <div style={{ display: 'flex', alignItems: 'flex-start', gap: tokens.space.md }}>
          <div style={badgeStyle} aria-hidden="true">
            ?
          </div>
          <div style={{ minWidth: 0 }}>
            <h2 id="alert-dialog-title" style={titleStyle}>
              {title}
            </h2>
            <p style={bodyStyle}>{body}</p>
          </div>
        </div>
        <div style={footerStyle}>
          <button type="button" style={cancelButtonStyle} onClick={onCancel}>
            {cancelLabel}
          </button>
          <button type="button" style={confirmButtonStyle} onClick={onConfirm}>
            {confirmLabel}
          </button>
        </div>
      </div>
    </Modal>
  )
}
