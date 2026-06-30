import type { ButtonHTMLAttributes, CSSProperties, ReactNode } from 'react'
import { tokens } from '../theme/tokens'

export type IconButtonVariant = 'ghost' | 'accent' | 'active'

export interface IconButtonProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  /** Icon node rendered inside the 40x40 square button. */
  icon: ReactNode
  /** Accessible label, used as both aria-label and title (required for icon-only controls). */
  label: string
  /** Visual variant. Defaults to 'ghost'. */
  variant?: IconButtonVariant
}

const variantStyle: Record<IconButtonVariant, CSSProperties> = {
  ghost: {
    background: 'transparent',
    border: `1px solid transparent`,
    color: tokens.color.textSecondary,
  },
  accent: {
    background: tokens.color.accent,
    border: `1px solid ${tokens.color.accent}`,
    color: tokens.color.textOnDark,
  },
  active: {
    background: tokens.color.accentBorder,
    border: `1px solid ${tokens.color.accentBorder}`,
    color: tokens.color.accentText,
  },
}

/**
 * 40x40 icon-only button for the instructor design system. Renders a real
 * <button> with an aria-label for accessibility. Styling is inline and reads
 * directly from the design tokens.
 */
export function IconButton({
  icon,
  label,
  variant = 'ghost',
  className,
  style,
  disabled,
  onMouseEnter,
  onMouseLeave,
  ...rest
}: IconButtonProps) {
  const base: CSSProperties = {
    display: 'inline-flex',
    alignItems: 'center',
    justifyContent: 'center',
    width: tokens.size.iconButton,
    height: tokens.size.iconButton,
    padding: 0,
    borderRadius: tokens.radius.button,
    cursor: disabled ? 'not-allowed' : 'pointer',
    opacity: disabled ? 0.5 : 1,
    transition: 'background 120ms ease, color 120ms ease',
    ...variantStyle[variant],
  }

  return (
    <button
      type="button"
      aria-label={label}
      title={label}
      disabled={disabled}
      className={className}
      style={{ ...base, ...style }}
      onMouseEnter={(e) => {
        if (!disabled && variant === 'ghost') {
          e.currentTarget.style.background = tokens.color.surfaceAlt
        }
        onMouseEnter?.(e)
      }}
      onMouseLeave={(e) => {
        if (variant === 'ghost') {
          e.currentTarget.style.background = 'transparent'
        }
        onMouseLeave?.(e)
      }}
      {...rest}
    >
      {icon}
    </button>
  )
}
