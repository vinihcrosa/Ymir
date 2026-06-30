import { render, screen, fireEvent } from '@testing-library/react'
import { describe, it, expect, vi } from 'vitest'
import { Slider } from './Slider'

describe('Slider', () => {
  it('renders the label and the value with unit', () => {
    render(<Slider label="Velocidade" value={12} min={0} max={30} unit="kn" onChange={() => {}} />)
    expect(screen.getByText('Velocidade')).toBeInTheDocument()
    expect(screen.getByText('12 kn')).toBeInTheDocument()
  })

  it('calls onChange with a Number on change events', () => {
    const onChange = vi.fn()
    render(<Slider label="Rumo" value={10} min={0} max={100} onChange={onChange} />)
    const input = screen.getByRole('slider') as HTMLInputElement
    fireEvent.change(input, { target: { value: '42' } })
    expect(onChange).toHaveBeenCalledWith(42)
    expect(typeof onChange.mock.calls[0][0]).toBe('number')
  })

  it('respects the min and max attributes', () => {
    render(<Slider label="Profundidade" value={5} min={2} max={20} step={2} onChange={() => {}} />)
    const input = screen.getByRole('slider') as HTMLInputElement
    expect(input.min).toBe('2')
    expect(input.max).toBe('20')
    expect(input.step).toBe('2')
  })

  it('disables the input when disabled', () => {
    render(<Slider label="Vento" value={3} min={0} max={50} disabled onChange={() => {}} />)
    expect(screen.getByRole('slider')).toBeDisabled()
  })
})
