import type { CSSProperties, ReactNode } from 'react'
import { tokens } from '../theme/tokens'

export interface FieldProps {
  /** Left-aligned label text (Portuguese UI copy). */
  label: string
  /** Optional unit appended to the right-side value (e.g. "m", "kn"). */
  unit?: string
  /** The control or value rendered on the right side of the row. */
  children: ReactNode
  /** Render the value with the mono font (and a `mono` class) — for numeric readouts. */
  mono?: boolean
  /** Text alignment of the value/children. Defaults to "right". */
  align?: CSSProperties['textAlign']
  /** Extra class merged onto the root row. */
  className?: string
  /** Extra inline styles merged onto the root row. */
  style?: CSSProperties
}

const rowStyle: CSSProperties = {
  display: 'flex',
  justifyContent: 'space-between',
  alignItems: 'center',
  padding: '5px 0',
  borderBottom: `1px solid ${tokens.color.surfaceAlt}`,
}

const labelStyle: CSSProperties = {
  fontSize: tokens.fontSize.label,
  color: tokens.color.textSubtle,
  minWidth: 80,
}

/**
 * Label-left / value-right row used across instructor panels.
 * The label sits flush left in subtle text; the control or value is right-aligned
 * in primary text with medium weight. Pass `mono` for numeric readouts.
 */
export function Field({
  label,
  unit,
  children,
  mono = false,
  align = 'right',
  className,
  style,
}: FieldProps) {
  const valueStyle: CSSProperties = {
    textAlign: align,
    color: tokens.color.textPrimary,
    fontWeight: tokens.fontWeight.medium,
    fontFamily: mono ? tokens.font.mono : undefined,
  }

  return (
    <div className={className} style={{ ...rowStyle, ...style }}>
      <span style={labelStyle}>{label}</span>
      <span className={mono ? 'mono' : undefined} style={valueStyle}>
        {children}
        {unit ? ` ${unit}` : null}
      </span>
    </div>
  )
}
