import type { CSSProperties, InputHTMLAttributes } from 'react'
import { tokens } from '../theme/tokens'

export interface SliderProps
  extends Omit<
    InputHTMLAttributes<HTMLInputElement>,
    'value' | 'min' | 'max' | 'step' | 'onChange' | 'type'
  > {
  /** Field label shown top-left. */
  label: string
  /** Current numeric value. */
  value: number
  /** Minimum bound. */
  min: number
  /** Maximum bound. */
  max: number
  /** Step increment (default 1). */
  step?: number
  /** Optional unit suffix appended to the value readout (e.g. "kn"). */
  unit?: string
  /** Called with the parsed numeric value on change. */
  onChange: (value: number) => void
  /** Disables the slider. */
  disabled?: boolean
}

const rootStyle: CSSProperties = {
  display: 'flex',
  flexDirection: 'column',
  gap: tokens.space.sm,
  fontFamily: tokens.font.sans,
}

const topRowStyle: CSSProperties = {
  display: 'flex',
  alignItems: 'baseline',
  justifyContent: 'space-between',
  gap: tokens.space.sm,
}

const labelStyle: CSSProperties = {
  fontSize: tokens.fontSize.label,
  fontWeight: tokens.fontWeight.medium,
  color: tokens.color.textSubtle,
}

const readoutStyle: CSSProperties = {
  fontFamily: tokens.font.mono,
  fontSize: tokens.fontSize.body,
  fontWeight: tokens.fontWeight.semibold,
  color: tokens.color.textPrimary,
}

const inputStyle: CSSProperties = {
  width: '100%',
  accentColor: tokens.color.accent,
  cursor: 'pointer',
  margin: 0,
}

const hintsRowStyle: CSSProperties = {
  display: 'flex',
  justifyContent: 'space-between',
  fontSize: tokens.fontSize.xs,
  color: tokens.color.textHcSubtle,
}

export function Slider({
  label,
  value,
  min,
  max,
  step = 1,
  unit,
  onChange,
  disabled,
  className,
  style,
  ...rest
}: SliderProps) {
  const suffix = unit ? ` ${unit}` : ''
  return (
    <div
      className={className}
      style={{ ...rootStyle, ...(disabled ? { opacity: 0.6 } : null), ...style }}
    >
      <div style={topRowStyle}>
        <span style={labelStyle}>{label}</span>
        <span style={readoutStyle} aria-hidden>
          {value.toFixed(0)}
          {suffix}
        </span>
      </div>
      <input
        type="range"
        value={value}
        min={min}
        max={max}
        step={step}
        disabled={disabled}
        aria-label={label}
        aria-valuetext={`${value.toFixed(0)}${suffix}`}
        style={{ ...inputStyle, cursor: disabled ? 'not-allowed' : inputStyle.cursor }}
        onChange={(e) => onChange(Number(e.target.value))}
        {...rest}
      />
      <div style={hintsRowStyle}>
        <span>
          {min}
          {suffix}
        </span>
        <span>
          {max}
          {suffix}
        </span>
      </div>
    </div>
  )
}
