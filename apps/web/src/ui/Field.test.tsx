import { render, screen } from '@testing-library/react'
import { describe, it, expect } from 'vitest'
import { Field } from './Field'

describe('Field', () => {
  it('renders the label and value', () => {
    render(<Field label="Calado">23</Field>)
    expect(screen.getByText('Calado')).toBeInTheDocument()
    expect(screen.getByText('23')).toBeInTheDocument()
  })

  it('appends the unit to the value', () => {
    render(
      <Field label="Boca" unit="m">
        63
      </Field>,
    )
    // Value and unit live in the same element.
    expect(screen.getByText('63 m')).toBeInTheDocument()
  })

  it('adds the "mono" class when mono is set', () => {
    render(
      <Field label="LPP" mono>
        350
      </Field>,
    )
    const value = screen.getByText('350')
    expect(value).toHaveClass('mono')
  })

  it('does not add the "mono" class by default', () => {
    render(<Field label="Nome">VLCC</Field>)
    expect(screen.getByText('VLCC')).not.toHaveClass('mono')
  })

  it('merges a custom className onto the root row', () => {
    const { container } = render(
      <Field label="X" className="custom-row">
        1
      </Field>,
    )
    expect(container.firstChild).toHaveClass('custom-row')
  })
})
