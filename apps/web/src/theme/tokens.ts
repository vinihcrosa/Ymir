/**
 * Design tokens reconciled from the Figma "[INSTRUTOR]" design system across all
 * five instructor screens. Consumed directly by components (inline styles read
 * `tokens.color.accent`) and mirrored to CSS custom properties via
 * {@link applyThemeVars} for use in `theme/global.css` and raw CSS.
 *
 * UI copy ships in Portuguese; identifiers/comments stay in English (project rule).
 */
export const tokens = {
  color: {
    // accent / primary
    accent: '#1a8ff6', //         surface/accent/hc-primary — add button, active fills
    accentText: '#147cec', //     content/accent/primary — active tab text, links
    accentBorder: '#bbeaff', //   accent border, active toolbar fill
    // content (text)
    textPrimary: '#101013',
    textSecondary: '#2f3033',
    textTertiary: '#4a4e5a',
    textSubtle: '#6b7280', //     labels, breadcrumb, placeholders, disabled
    textHcSubtle: '#9ca3af', //   hint / min-max
    // surfaces
    surface: '#ffffff',
    surfaceAlt: '#f9fafb', //     group boxes, inputs, cards
    surfaceActive: '#e5e7eb', //  active sidebar pill
    surfaceDark: '#25272d', //    dark primary buttons ("Salvar cenário", "Build")
    textOnDark: '#ffffff',
    // borders
    border: '#e5e7eb',
    borderActive: '#9ca3af', //   input focus
    borderDark: '#383a42',
    // feedback
    danger: '#c81e1e',
    dangerBorder: '#fbd5d5',
    success: '#10b981',
    warningBg: '#fef3c7',
    warningFg: '#92400e',
    // scrim
    scrim: 'rgba(16,16,19,0.5)', // #10101380
  },
  font: {
    sans: "'Inter', system-ui, -apple-system, sans-serif",
    mono: "'JetBrains Mono', ui-monospace, 'SFMono-Regular', monospace", // numeric values
  },
  fontSize: { xs: 10, sm: 12, label: 13, body: 14, title: 16, panelTitle: 18 },
  fontWeight: { regular: 400, medium: 500, semibold: 600, bold: 700 },
  space: { px: 2, xs: 4, sm: 8, md: 12, lg: 16, xl: 20, xxl: 24 }, // 4px base scale
  radius: { sm: 4, md: 6, button: 8, panel: 12, pill: 999 },
  shadow: {
    sm: '0 1px 2px rgba(0,0,0,0.05)', //  shadow/down/small — topbar, pills, cards
    panel: '0 4px 24px rgba(0,0,0,0.10)', // floating right panel
  },
  size: {
    topbar: 56,
    iconButton: 40,
    inputHeight: 40,
    rightPanel: 484,
    vesselPanel: 398,
    sidebar: 280,
  },
  z: { map: 0, overlay: 1000, panel: 1100, modal: 2000 },
} as const

export type Tokens = typeof tokens

/** Flatten the token tree into CSS custom properties: `--color-accent`, `--space-md`, … */
export function themeVars(): Record<string, string> {
  const vars: Record<string, string> = {}
  for (const [group, entries] of Object.entries(tokens)) {
    for (const [key, value] of Object.entries(entries as Record<string, unknown>)) {
      vars[`--${group}-${key}`] = String(value)
    }
  }
  return vars
}

/** Write the token CSS variables onto a target element (defaults to :root). */
export function applyThemeVars(target: HTMLElement = document.documentElement): void {
  const vars = themeVars()
  for (const [name, value] of Object.entries(vars)) {
    target.style.setProperty(name, value)
  }
}
