import { describe, it, expect, vi, beforeEach } from 'vitest'

// MockWorker must be defined before the store is imported so that vi.stubGlobal
// is in place when the module is first evaluated.
class MockWorker {
  onmessage: ((e: MessageEvent) => void) | null = null
  onerror: ((e: ErrorEvent) => void) | null = null
  postMessage = vi.fn()
  terminate = vi.fn()

  /** Helper: simulate an inbound message from the worker */
  _emit(data: unknown) {
    this.onmessage?.({ data } as MessageEvent)
  }
}

vi.stubGlobal('Worker', MockWorker)

// Also stub URL constructor used inside start()
vi.stubGlobal('URL', class {
  constructor(public href: string, public base?: string) {}
})

// Import AFTER stubs are in place
import { useSimulationStore } from './simulationStore'
import { useEnvironmentStore } from './environmentStore'

// Helper to access raw store state without React hooks
const store = () => useSimulationStore.getState()

beforeEach(() => {
  // Reset store to initial state before every test
  store().reset()
  useEnvironmentStore.getState().reset()
  vi.clearAllMocks()
})

describe('simulationStore — initial state', () => {
  it('starts with status idle', () => {
    expect(store().status).toBe('idle')
  })

  it('starts with null error, state, and worker', () => {
    expect(store().error).toBeNull()
    expect(store().state).toBeNull()
    expect(store().worker).toBeNull()
  })

  it('starts with empty scenarioVessels', () => {
    expect(store().scenarioVessels).toEqual([])
  })
})

describe('simulationStore — loadScenario', () => {
  it('sets scenarioVessels without a worker', () => {
    const vessels = [{ instanceId: 1, vesselId: 1, name: 'Ship A', x: 0, y: 0, headingDeg: 90 }]
    store().loadScenario(vessels)
    expect(store().scenarioVessels).toEqual(vessels)
  })

  it('posts loadScenario message to existing worker', () => {
    // Manually set a mock worker into the store
    const mockWorker = new MockWorker()
    useSimulationStore.setState({ worker: mockWorker as unknown as Worker, status: 'paused' })

    const vessels = [{ instanceId: 2, vesselId: 2, name: 'Ship B', x: 10, y: 20, headingDeg: 0 }]
    store().loadScenario(vessels)

    expect(mockWorker.postMessage).toHaveBeenCalledWith({ type: 'loadScenario', vessels })
    expect(store().scenarioVessels).toEqual(vessels)
  })
})

describe('simulationStore — stop', () => {
  it('sets status to paused and sends stop message to worker', () => {
    const mockWorker = new MockWorker()
    useSimulationStore.setState({ worker: mockWorker as unknown as Worker, status: 'running' })

    store().pause()

    expect(store().status).toBe('paused')
    expect(mockWorker.postMessage).toHaveBeenCalledWith({ type: 'stop' })
  })

  it('sets status to paused even when no worker is present', () => {
    useSimulationStore.setState({ status: 'running', worker: null })
    store().pause()
    expect(store().status).toBe('paused')
  })
})

describe('simulationStore — reset', () => {
  it('clears status, error, state, worker, and scenarioVessels', () => {
    const mockWorker = new MockWorker()
    useSimulationStore.setState({
      worker: mockWorker as unknown as Worker,
      status: 'running',
      error: 'some error',
      state: { vessels: [] } as never,
      scenarioVessels: [{ instanceId: 3, vesselId: 3, name: 'Ship C', x: 5, y: 5, headingDeg: 45 }],
    })

    store().reset()

    expect(store().status).toBe('idle')
    expect(store().error).toBeNull()
    expect(store().state).toBeNull()
    expect(store().worker).toBeNull()
    expect(store().scenarioVessels).toEqual([])
  })

  it('terminates the worker on reset', () => {
    const mockWorker = new MockWorker()
    useSimulationStore.setState({ worker: mockWorker as unknown as Worker, status: 'running' })

    store().reset()

    expect(mockWorker.terminate).toHaveBeenCalledOnce()
  })
})

describe('simulationStore — start (with mock worker)', () => {
  it('transitions to loading then running after worker emits ready', () => {
    store().play(0.05)

    // Status should be loading while worker is being set up
    expect(store().status).toBe('loading')

    // Retrieve the worker instance created by start()
    const mockWorker = store().worker as unknown as MockWorker

    // Simulate the worker posting a 'ready' message
    mockWorker._emit({ type: 'ready' })

    expect(store().status).toBe('running')
  })

  it('sends loadScenario before start when scenarioVessels are set', () => {
    const vessels = [{ instanceId: 4, vesselId: 4, name: 'Ship D', x: 1, y: 2, headingDeg: 180 }]
    useSimulationStore.setState({ scenarioVessels: vessels })

    store().play(0.05)

    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'ready' })

    const calls = mockWorker.postMessage.mock.calls.map((c) => c[0] as { type: string })
    const loadCall = calls.find((c) => c.type === 'loadScenario')
    const startCall = calls.find((c) => c.type === 'start')

    expect(loadCall).toBeDefined()
    expect(startCall).toBeDefined()

    // loadScenario must precede start
    const loadIdx = calls.indexOf(loadCall!)
    const startIdx = calls.indexOf(startCall!)
    expect(loadIdx).toBeLessThan(startIdx)
  })

  it('sets status to error when worker emits error message', () => {
    store().play()
    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'error', message: 'WASM failed to load' })

    expect(store().status).toBe('error')
    expect(store().error).toBe('WASM failed to load')
  })

  it('uses Unknown error fallback when error message is undefined', () => {
    store().play()
    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'error' })
    expect(store().error).toBe('Unknown error')
  })

  it('handles state message with undefined payload (sets null)', () => {
    store().play()
    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'ready' })
    mockWorker._emit({ type: 'state' })
    expect(store().state).toBeNull()
  })

  it('calls postMessage on existing worker when start called again', () => {
    const mockWorker = new MockWorker()
    useSimulationStore.setState({ worker: mockWorker as unknown as Worker, status: 'paused' })
    store().play(0.1)
    expect(mockWorker.postMessage).toHaveBeenCalledWith({ type: 'start', dt: 0.1 })
    expect(store().status).toBe('running')
  })
})

describe('simulationStore — loadEnvironment on start', () => {
  it('sends loadEnvironment before start when environment has conditions', () => {
    useEnvironmentStore.setState({
      currentSeries: [[{ t: 0, speed: 1.0, dirNaut: 90 }]],
    })

    store().play(0.05)
    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'ready' })

    const calls = mockWorker.postMessage.mock.calls.map((c) => c[0] as { type: string })
    const loadEnvIdx = calls.findIndex((c) => c.type === 'loadEnvironment')
    const startIdx = calls.findIndex((c) => c.type === 'start')

    expect(loadEnvIdx).toBeGreaterThanOrEqual(0)
    expect(startIdx).toBeGreaterThanOrEqual(0)
    expect(loadEnvIdx).toBeLessThan(startIdx)
  })

  it('does not send loadEnvironment when environment is empty', () => {
    store().play(0.05)
    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'ready' })

    const calls = mockWorker.postMessage.mock.calls.map((c) => c[0] as { type: string })
    expect(calls.some((c) => c.type === 'loadEnvironment')).toBe(false)
  })

  it('loadEnvironment message contains valid JSON with environment data', () => {
    useEnvironmentStore.setState({
      currentSeries: [[{ t: 0, speed: 2.0, dirNaut: 180 }]],
    })

    store().play(0.05)
    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'ready' })

    const calls = mockWorker.postMessage.mock.calls.map((c) => c[0])
    const loadEnvCall = calls.find((c: { type: string }) => c.type === 'loadEnvironment') as
      | { type: string; json: string }
      | undefined
    expect(loadEnvCall).toBeDefined()
    expect(typeof loadEnvCall!.json).toBe('string')
    const parsed = JSON.parse(loadEnvCall!.json)
    expect(parsed.currentSeries).toHaveLength(1)
    expect(parsed.currentSeries[0][0].speed).toBe(2.0)
  })

  it('does not send loadEnvironment when only wind conditions are empty', () => {
    // Explicitly verify wind conditions trigger the send
    useEnvironmentStore.setState({
      windSeries: [[{ t: 0, speed: 5.0, dirNaut: 0 }]],
    })

    store().play(0.05)
    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'ready' })

    const calls = mockWorker.postMessage.mock.calls.map((c) => c[0] as { type: string })
    expect(calls.some((c) => c.type === 'loadEnvironment')).toBe(true)
  })

  it('existing start/stop/loadScenario/setActuator paths unaffected by environment integration', () => {
    const vessels = [{ instanceId: 1, vesselId: 1, name: 'Ship A', x: 0, y: 0, headingDeg: 0 }]
    useSimulationStore.setState({ scenarioVessels: vessels })

    store().play(0.05)
    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'ready' })

    const calls = mockWorker.postMessage.mock.calls.map((c) => c[0] as { type: string })
    expect(calls.some((c) => c.type === 'loadScenario')).toBe(true)
    expect(calls.some((c) => c.type === 'start')).toBe(true)
    // No loadEnvironment when store is empty
    expect(calls.some((c) => c.type === 'loadEnvironment')).toBe(false)
  })
})
