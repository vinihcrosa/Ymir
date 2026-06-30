import type { CSSProperties, HTMLAttributes, ReactNode } from 'react'
import { tokens } from '../theme/tokens'

export type CardTone = 'default' | 'alt'

export interface CardProps extends HTMLAttributes<HTMLDivElement> {
  /** Optional uppercase title row rendered above the body. */
  title?: string
  /** `alt` swaps the surface background to `surfaceAlt`. */
  tone?: CardTone
  children?: ReactNode
}

const rootStyle = (tone: CardTone): CSSProperties => ({
  background: tone === 'alt' ? tokens.color.surfaceAlt : tokens.color.surface,
  border: `1px solid ${tokens.color.border}`,
  borderRadius: tokens.radius.md,
  padding: tokens.space.md,
  marginBottom: tokens.space.sm,
})

const titleStyle: CSSProperties = {
  fontSize: tokens.fontSize.sm,
  fontWeight: tokens.fontWeight.semibold,
  color: tokens.color.textSubtle,
  letterSpacing: '0.05em',
  textTransform: 'uppercase',
  marginBottom: tokens.space.sm,
}

/**
 * Bordered group box. Render a simple titled card via the `title` prop, or
 * compose freely with {@link CardHeader} and {@link CardBody}.
 */
function CardRoot({ title, tone = 'default', children, style, ...rest }: CardProps) {
  return (
    <div style={{ ...rootStyle(tone), ...style }} {...rest}>
      {title !== undefined && <div style={titleStyle}>{title}</div>}
      {children}
    </div>
  )
}

export interface CardSectionProps extends HTMLAttributes<HTMLDivElement> {
  children?: ReactNode
}

/** Uppercase small title row for composed cards. */
export function CardHeader({ children, style, ...rest }: CardSectionProps) {
  return (
    <div style={{ ...titleStyle, ...style }} {...rest}>
      {children}
    </div>
  )
}

/** Body region for composed cards. */
export function CardBody({ children, style, ...rest }: CardSectionProps) {
  return (
    <div style={style} {...rest}>
      {children}
    </div>
  )
}

/** Compound `Card` with `Card.Header` and `Card.Body` sub-components. */
export const Card = Object.assign(CardRoot, {
  Header: CardHeader,
  Body: CardBody,
})
