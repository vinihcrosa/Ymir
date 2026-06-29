import { type ButtonHTMLAttributes, type CSSProperties, type ReactNode } from 'react'
import { tokens } from '../theme/tokens'

export type ButtonVariant = 'primary' | 'secondary' | 'dark' | 'ghost' | 'danger'
export type ButtonSize = 'sm' | 'md'

export interface ButtonProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  /** Visual style. Defaults to `primary`. */
  variant?: ButtonVariant
  /** Padding/height preset. Defaults to `md`. */
  size?: ButtonSize
  /** Optional leading/trailing icon node. */
  icon?: ReactNode
  /** Side the icon sits on relative to the label. Defaults to `left`. */
  iconPosition?: 'left' | 'right'
  /** Stretch to fill the parent width. */
  fullWidth?: boolean
  /** Show a spinner and disable interaction. */
  loading?: boolean
}

interface VariantStyle {
  background: string
  color: string
  border: string
}

function variantStyle(variant: ButtonVariant): VariantStyle {
  switch (variant) {
    case 'dark':
      return { background: tokens.color.surfaceDark, color: tokens.color.textOnDark, border: 'none' }
    case 'secondary':
      return {
        background: tokens.color.surface,
        color: tokens.color.textPrimary,
        border: `1px solid ${tokens.color.border}`,
      }
    case 'ghost':
      return { background: 'transparent', color: tokens.color.textPrimary, border: 'none' }
    case 'danger':
      return { background: tokens.color.danger, color: tokens.color.textOnDark, border: 'none' }
    case 'primary':
    default:
      return { background: tokens.color.accent, color: tokens.color.surface, border: 'none' }
  }
}

const sizeStyle: Record<ButtonSize, CSSProperties> = {
  sm: { height: 32, padding: `0 ${tokens.space.md}px`, fontSize: tokens.fontSize.label },
  md: { height: tokens.size.inputHeight, padding: `0 ${tokens.space.lg}px`, fontSize: tokens.fontSize.body },
}

function Spinner({ color }: { color: string }) {
  return (
    <span
      aria-hidden
      style={{
        display: 'inline-block',
        width: '1em',
        height: '1em',
        border: `2px solid ${color}`,
        borderTopColor: 'transparent',
        borderRadius: tokens.radius.pill,
        animation: 'ymir-button-spin 0.6s linear infinite',
      }}
    />
  )
}

/**
 * Primary action button for the instructor design system. Renders a real
 * `<button>` and reads all visual values from the design tokens.
 */
export function Button({
  variant = 'primary',
  size = 'md',
  icon,
  iconPosition = 'left',
  fullWidth = false,
  loading = false,
  disabled = false,
  type = 'button',
  className,
  style,
  children,
  ...rest
}: ButtonProps) {
  const v = variantStyle(variant)
  const isDisabled = disabled || loading

  const computed: CSSProperties = {
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    gap: tokens.space.sm,
    fontFamily: tokens.font.sans,
    fontWeight: tokens.fontWeight.medium,
    lineHeight: 1,
    borderRadius: tokens.radius.button,
    background: v.background,
    color: isDisabled ? tokens.color.textSubtle : v.color,
    border: v.border,
    width: fullWidth ? '100%' : undefined,
    cursor: isDisabled ? 'not-allowed' : 'pointer',
    opacity: isDisabled ? 0.7 : 1,
    boxSizing: 'border-box',
    whiteSpace: 'nowrap',
    transition: 'background 0.15s ease, opacity 0.15s ease',
    ...sizeStyle[size],
    ...style,
  }

  const leading = loading ? <Spinner color={isDisabled ? tokens.color.textSubtle : v.color} /> : icon

  return (
    <>
      <style>{'@keyframes ymir-button-spin{to{transform:rotate(360deg)}}'}</style>
      <button
        type={type}
        className={className}
        style={computed}
        disabled={isDisabled}
        aria-busy={loading || undefined}
        {...rest}
      >
        {leading && iconPosition === 'left' && (
          <span style={{ display: 'inline-flex' }}>{leading}</span>
        )}
        {children}
        {leading && iconPosition === 'right' && (
          <span style={{ display: 'inline-flex' }}>{leading}</span>
        )}
      </button>
    </>
  )
}
