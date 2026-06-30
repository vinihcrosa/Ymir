import { describe, it, expect } from 'vitest'
import { tokens, themeVars, applyThemeVars } from './tokens'

describe('design tokens', () => {
  it('exposes the core palette', () => {
    expect(tokens.color.accent).toBe('#1a8ff6')
    expect(tokens.color.surfaceDark).toBe('#25272d')
    expect(tokens.size.topbar).toBe(56)
  })

  it('flattens to CSS custom properties', () => {
    const vars = themeVars()
    expect(vars['--color-accent']).toBe('#1a8ff6')
    expect(vars['--space-md']).toBe('12')
    expect(vars['--font-sans']).toContain('Inter')
  })

  it('applyThemeVars writes variables onto the target element', () => {
    const el = document.createElement('div')
    applyThemeVars(el)
    expect(el.style.getPropertyValue('--color-accent')).toBe('#1a8ff6')
    expect(el.style.getPropertyValue('--radius-button')).toBe('8')
  })

  it('does not collide accent and danger semantics', () => {
    expect(tokens.color.accent).not.toBe(tokens.color.danger)
  })
})
