import { render, screen, fireEvent, within } from '@testing-library/react'
import { describe, it, expect, beforeEach } from 'vitest'
import { EnvironmentConditionPanel } from './EnvironmentConditionPanel'
import { useEnvironmentStore } from '../../../stores/environmentStore'

const store = () => useEnvironmentStore.getState()

beforeEach(() => {
  store().reset()
})

// ─── Rendering ──────────────────────────────────────────────────────────────

describe('rendering', () => {
  it('renders without errors when store has no conditions (empty state)', () => {
    render(<EnvironmentConditionPanel />)
    expect(screen.getByTestId('environment-condition-panel')).toBeInTheDocument()
  })

  it('renders all three section labels', () => {
    render(<EnvironmentConditionPanel />)
    expect(screen.getByRole('button', { name: /add current series/i })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /add wind series/i })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /add wave keyframe/i })).toBeInTheDocument()
  })

  it('shows empty state message for current when no series', () => {
    render(<EnvironmentConditionPanel />)
    expect(screen.getByText(/no current conditions configured/i)).toBeInTheDocument()
  })

  it('shows empty state message for wind when no series', () => {
    render(<EnvironmentConditionPanel />)
    expect(screen.getByText(/no wind conditions configured/i)).toBeInTheDocument()
  })

  it('shows empty state message for wave when no keyframes', () => {
    render(<EnvironmentConditionPanel />)
    expect(screen.getByText(/no wave conditions configured/i)).toBeInTheDocument()
  })
})

// ─── Current Series ──────────────────────────────────────────────────────────

describe('current series', () => {
  it('"Add Current Series" button calls addCurrentSeries()', () => {
    render(<EnvironmentConditionPanel />)
    fireEvent.click(screen.getByRole('button', { name: /add current series/i }))
    expect(store().currentSeries).toHaveLength(1)
  })

  it('removing a current series calls removeCurrentSeries with correct index', () => {
    store().addCurrentSeries()
    store().addCurrentSeries()
    render(<EnvironmentConditionPanel />)
    const removeButtons = screen.getAllByRole('button', { name: /remove current series/i })
    fireEvent.click(removeButtons[0])
    expect(store().currentSeries).toHaveLength(1)
  })

  it('speed input change calls setCurrentKeyframe with speed value', () => {
    store().addCurrentSeries()
    render(<EnvironmentConditionPanel />)
    const speedInput = screen.getByRole('spinbutton', { name: /current series 0 keyframe 0 speed/i })
    fireEvent.change(speedInput, { target: { value: '2.5' } })
    expect(store().currentSeries[0][0].speed).toBe(2.5)
  })

  it('direction input change calls setCurrentKeyframe with dirNaut value', () => {
    store().addCurrentSeries()
    render(<EnvironmentConditionPanel />)
    const dirInput = screen.getByRole('spinbutton', { name: /current series 0 keyframe 0 direction/i })
    fireEvent.change(dirInput, { target: { value: '90' } })
    expect(store().currentSeries[0][0].dirNaut).toBe(90)
  })

  it('direction input has type number (rejects non-numeric natively)', () => {
    store().addCurrentSeries()
    render(<EnvironmentConditionPanel />)
    const dirInput = screen.getByRole('spinbutton', { name: /current series 0 keyframe 0 direction/i })
    expect(dirInput).toHaveAttribute('type', 'number')
    expect(dirInput).toHaveAttribute('min', '0')
    expect(dirInput).toHaveAttribute('max', '360')
  })

  it('direction input shows helper text "0 = North, clockwise, from where"', () => {
    store().addCurrentSeries()
    render(<EnvironmentConditionPanel />)
    const helpers = screen.getAllByText('0 = North, clockwise, from where')
    expect(helpers.length).toBeGreaterThanOrEqual(1)
  })

  it('series with 1 keyframe displays "Static" label', () => {
    store().addCurrentSeries()
    render(<EnvironmentConditionPanel />)
    expect(screen.getByText(/series 1 — static/i)).toBeInTheDocument()
  })

  it('series with 2+ keyframes displays "Temporal" label', () => {
    store().addCurrentSeries()
    store().addCurrentKeyframe(0)
    render(<EnvironmentConditionPanel />)
    expect(screen.getByText(/series 1 — temporal/i)).toBeInTheDocument()
  })

  it('series with 2+ keyframes shows temporal interpolation note', () => {
    store().addCurrentSeries()
    store().addCurrentKeyframe(0)
    render(<EnvironmentConditionPanel />)
    expect(screen.getAllByText(/temporal interpolation active/i).length).toBeGreaterThanOrEqual(1)
  })

  it('"Add keyframe" button appends a keyframe to the series', () => {
    store().addCurrentSeries()
    render(<EnvironmentConditionPanel />)
    const addKfBtn = screen.getByRole('button', { name: /add current keyframe to series 0/i })
    fireEvent.click(addKfBtn)
    expect(store().currentSeries[0]).toHaveLength(2)
  })

  it('time input change calls setCurrentKeyframe with t value', () => {
    store().addCurrentSeries()
    render(<EnvironmentConditionPanel />)
    const timeInput = screen.getByRole('spinbutton', { name: /current series 0 keyframe 0 time/i })
    fireEvent.change(timeInput, { target: { value: '600' } })
    expect(store().currentSeries[0][0].t).toBe(600)
  })
})

// ─── Wind Series ──────────────────────────────────────────────────────────────

describe('wind series', () => {
  it('"Add Wind Series" button calls addWindSeries()', () => {
    render(<EnvironmentConditionPanel />)
    fireEvent.click(screen.getByRole('button', { name: /add wind series/i }))
    expect(store().windSeries).toHaveLength(1)
  })

  it('removing a wind series calls removeWindSeries with correct index', () => {
    store().addWindSeries()
    store().addWindSeries()
    render(<EnvironmentConditionPanel />)
    const removeButtons = screen.getAllByRole('button', { name: /remove wind series/i })
    fireEvent.click(removeButtons[0])
    expect(store().windSeries).toHaveLength(1)
  })

  it('speed input change calls setWindKeyframe with speed value', () => {
    store().addWindSeries()
    render(<EnvironmentConditionPanel />)
    const speedInput = screen.getByRole('spinbutton', { name: /wind series 0 keyframe 0 speed/i })
    fireEvent.change(speedInput, { target: { value: '10' } })
    expect(store().windSeries[0][0].speed).toBe(10)
  })

  it('wind series with 1 keyframe shows "Static"', () => {
    store().addWindSeries()
    render(<EnvironmentConditionPanel />)
    expect(screen.getByText(/series 1 — static/i)).toBeInTheDocument()
  })

  it('wind series with 2+ keyframes shows "Temporal"', () => {
    store().addWindSeries()
    store().addWindKeyframe(0)
    render(<EnvironmentConditionPanel />)
    expect(screen.getByText(/series 1 — temporal/i)).toBeInTheDocument()
  })

  it('"Add keyframe" button appends a keyframe to the wind series', () => {
    store().addWindSeries()
    render(<EnvironmentConditionPanel />)
    const addKfBtn = screen.getByRole('button', { name: /add wind keyframe to series 0/i })
    fireEvent.click(addKfBtn)
    expect(store().windSeries[0]).toHaveLength(2)
  })
})

// ─── Wave Section ──────────────────────────────────────────────────────────────

describe('wave section', () => {
  it('"Add Wave Keyframe" button calls addWaveKeyframe()', () => {
    render(<EnvironmentConditionPanel />)
    fireEvent.click(screen.getByRole('button', { name: /add wave keyframe/i }))
    expect(store().waveSeries).toHaveLength(1)
  })

  it('wave spectrum dropdown shows JONSWAP, PIERSON, REGULAR options', () => {
    store().addWaveKeyframe()
    render(<EnvironmentConditionPanel />)
    const select = screen.getByRole('combobox', { name: /wave keyframe 0 spectrum/i })
    expect(within(select).getByRole('option', { name: 'JONSWAP' })).toBeInTheDocument()
    expect(within(select).getByRole('option', { name: 'PIERSON' })).toBeInTheDocument()
    expect(within(select).getByRole('option', { name: 'REGULAR' })).toBeInTheDocument()
  })

  it('gamma input visible when spectrum is JONSWAP (default)', () => {
    store().addWaveKeyframe()
    render(<EnvironmentConditionPanel />)
    expect(screen.getByRole('spinbutton', { name: /wave keyframe 0 gamma/i })).toBeInTheDocument()
  })

  it('gamma input hidden when spectrum is PIERSON', () => {
    store().addWaveKeyframe()
    store().setWaveKeyframe(0, { spectrum: 'PIERSON' })
    render(<EnvironmentConditionPanel />)
    expect(screen.queryByRole('spinbutton', { name: /wave keyframe 0 gamma/i })).not.toBeInTheDocument()
  })

  it('gamma input hidden when spectrum is REGULAR', () => {
    store().addWaveKeyframe()
    store().setWaveKeyframe(0, { spectrum: 'REGULAR' })
    render(<EnvironmentConditionPanel />)
    expect(screen.queryByRole('spinbutton', { name: /wave keyframe 0 gamma/i })).not.toBeInTheDocument()
  })

  it('changing spectrum dropdown to PIERSON hides gamma input', () => {
    store().addWaveKeyframe()
    render(<EnvironmentConditionPanel />)
    const select = screen.getByRole('combobox', { name: /wave keyframe 0 spectrum/i })
    fireEvent.change(select, { target: { value: 'PIERSON' } })
    expect(screen.queryByRole('spinbutton', { name: /wave keyframe 0 gamma/i })).not.toBeInTheDocument()
    expect(store().waveSeries[0].spectrum).toBe('PIERSON')
  })

  it('wave Hs input updates store', () => {
    store().addWaveKeyframe()
    render(<EnvironmentConditionPanel />)
    const HsInput = screen.getByRole('spinbutton', { name: /wave keyframe 0 hs/i })
    fireEvent.change(HsInput, { target: { value: '3.5' } })
    expect(store().waveSeries[0].Hs).toBe(3.5)
  })

  it('wave Tp input updates store', () => {
    store().addWaveKeyframe()
    render(<EnvironmentConditionPanel />)
    const TpInput = screen.getByRole('spinbutton', { name: /wave keyframe 0 tp/i })
    fireEvent.change(TpInput, { target: { value: '12' } })
    expect(store().waveSeries[0].Tp).toBe(12)
  })

  it('wave single keyframe shows Static label', () => {
    store().addWaveKeyframe()
    render(<EnvironmentConditionPanel />)
    expect(screen.getByText(/wave — static/i)).toBeInTheDocument()
  })

  it('wave 2+ keyframes shows Temporal label', () => {
    store().addWaveKeyframe()
    store().addWaveKeyframe()
    render(<EnvironmentConditionPanel />)
    expect(screen.getByText(/wave — temporal/i)).toBeInTheDocument()
  })

  it('remove wave keyframe button updates store', () => {
    store().addWaveKeyframe()
    store().addWaveKeyframe()
    render(<EnvironmentConditionPanel />)
    const removeBtns = screen.getAllByRole('button', { name: /remove wave keyframe/i })
    fireEvent.click(removeBtns[0])
    expect(store().waveSeries).toHaveLength(1)
  })
})

// ─── Integration ──────────────────────────────────────────────────────────────

describe('integration', () => {
  it('full flow: add current series → set direction 90 speed 1.0 → toJson reflects values', () => {
    render(<EnvironmentConditionPanel />)

    // Add a current series
    fireEvent.click(screen.getByRole('button', { name: /add current series/i }))
    expect(store().currentSeries).toHaveLength(1)

    // Set direction to 90
    const dirInput = screen.getByRole('spinbutton', { name: /current series 0 keyframe 0 direction/i })
    fireEvent.change(dirInput, { target: { value: '90' } })

    // Set speed to 1.0
    const speedInput = screen.getByRole('spinbutton', { name: /current series 0 keyframe 0 speed/i })
    fireEvent.change(speedInput, { target: { value: '1.0' } })

    // Verify store state
    const kf = store().currentSeries[0][0]
    expect(kf.dirNaut).toBe(90)
    expect(kf.speed).toBe(1)

    // Verify toJson serializes correctly
    const json = JSON.parse(store().toJson())
    expect(json.currentSeries[0][0].dirNaut).toBe(90)
    expect(json.currentSeries[0][0].speed).toBe(1)
  })

  it('add wave keyframe then set all fields → toJson round-trip', () => {
    render(<EnvironmentConditionPanel />)

    fireEvent.click(screen.getByRole('button', { name: /add wave keyframe/i }))

    fireEvent.change(screen.getByRole('spinbutton', { name: /wave keyframe 0 hs/i }), { target: { value: '2.0' } })
    fireEvent.change(screen.getByRole('spinbutton', { name: /wave keyframe 0 tp/i }), { target: { value: '10' } })
    fireEvent.change(screen.getByRole('spinbutton', { name: /wave keyframe 0 direction/i }), { target: { value: '270' } })

    const json = JSON.parse(store().toJson())
    expect(json.waveSeries[0].Hs).toBe(2.0)
    expect(json.waveSeries[0].Tp).toBe(10)
    expect(json.waveSeries[0].dirNaut).toBe(270)
  })
})
