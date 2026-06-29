import { render, screen } from '@testing-library/react'
import { describe, it, expect, vi, beforeEach } from 'vitest'
import { ScenarioCreatorPage } from './ScenarioCreatorPage'
import { useScenarioStore } from './store'
import { useSimulationStore } from '../../stores/simulationStore'
import { useEnvironmentStore } from '../../stores/environmentStore'

vi.mock('./components/AreaMapView', () => ({
  AreaMapView: () => <div data-testid="area-map-view" />,
}))

vi.mock('./components/VesselPanel', () => ({
  VesselPanel: () => <div data-testid="vessel-panel" />,
}))

vi.mock('./hooks/use-areas', () => ({
  useAreas: () => ({
    areas: [],
    loading: false,
    error: null,
    retry: vi.fn(),
  }),
}))

vi.mock('./hooks/use-vessels', () => ({
  useVessels: () => ({ vessels: [], loading: false, error: null }),
}))

vi.stubGlobal('Worker', class {
  onmessage: ((e: MessageEvent) => void) | null = null
  onerror: ((e: ErrorEvent) => void) | null = null
  postMessage = vi.fn()
  terminate = vi.fn()
})
vi.stubGlobal('URL', class {
  constructor(public href: string, public base?: string) {}
})

beforeEach(() => {
  useScenarioStore.getState().reset()
  useSimulationStore.getState().reset()
  useEnvironmentStore.getState().reset()
})

describe('ScenarioCreatorPage', () => {
  it('renders without throwing', () => {
    expect(() => render(<ScenarioCreatorPage />)).not.toThrow()
  })

  it('renders EnvironmentConditionPanel', () => {
    render(<ScenarioCreatorPage />)
    expect(screen.getByTestId('environment-condition-panel')).toBeInTheDocument()
  })

  it('renders AreaMapView without regression', () => {
    render(<ScenarioCreatorPage />)
    expect(screen.getByTestId('area-map-view')).toBeInTheDocument()
  })

  it('renders VesselPanel without regression', () => {
    render(<ScenarioCreatorPage />)
    expect(screen.getByTestId('vessel-panel')).toBeInTheDocument()
  })

  it('renders Sidebar heading', () => {
    render(<ScenarioCreatorPage />)
    expect(screen.getByRole('heading', { name: /criar cenário/i })).toBeInTheDocument()
  })

  it('EnvironmentConditionPanel shows all three condition sections', () => {
    render(<ScenarioCreatorPage />)
    expect(screen.getByRole('button', { name: /add current series/i })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /add wind series/i })).toBeInTheDocument()
    expect(screen.getByRole('button', { name: /add wave keyframe/i })).toBeInTheDocument()
  })
})
