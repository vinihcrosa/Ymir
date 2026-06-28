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

// Helper to access raw store state without React hooks
const store = () => useSimulationStore.getState()

beforeEach(() => {
  // Reset store to initial state before every test
  store().reset()
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
    const vessels = [{ vesselId: 1, name: 'Ship A', x: 0, y: 0, headingDeg: 90 }]
    store().loadScenario(vessels)
    expect(store().scenarioVessels).toEqual(vessels)
  })

  it('posts loadScenario message to existing worker', () => {
    // Manually set a mock worker into the store
    const mockWorker = new MockWorker()
    useSimulationStore.setState({ worker: mockWorker as unknown as Worker, status: 'ready' })

    const vessels = [{ vesselId: 2, name: 'Ship B', x: 10, y: 20, headingDeg: 0 }]
    store().loadScenario(vessels)

    expect(mockWorker.postMessage).toHaveBeenCalledWith({ type: 'loadScenario', vessels })
    expect(store().scenarioVessels).toEqual(vessels)
  })
})

describe('simulationStore — stop', () => {
  it('sets status to ready and sends stop message to worker', () => {
    const mockWorker = new MockWorker()
    useSimulationStore.setState({ worker: mockWorker as unknown as Worker, status: 'running' })

    store().stop()

    expect(store().status).toBe('ready')
    expect(mockWorker.postMessage).toHaveBeenCalledWith({ type: 'stop' })
  })

  it('sets status to ready even when no worker is present', () => {
    useSimulationStore.setState({ status: 'running', worker: null })
    store().stop()
    expect(store().status).toBe('ready')
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
      scenarioVessels: [{ vesselId: 3, name: 'Ship C', x: 5, y: 5, headingDeg: 45 }],
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
    store().start(0.05)

    // Status should be loading while worker is being set up
    expect(store().status).toBe('loading')

    // Retrieve the worker instance created by start()
    const mockWorker = store().worker as unknown as MockWorker

    // Simulate the worker posting a 'ready' message
    mockWorker._emit({ type: 'ready' })

    expect(store().status).toBe('running')
  })

  it('sends loadScenario before start when scenarioVessels are set', () => {
    const vessels = [{ vesselId: 4, name: 'Ship D', x: 1, y: 2, headingDeg: 180 }]
    useSimulationStore.setState({ scenarioVessels: vessels })

    store().start(0.05)

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
    store().start()
    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'error', message: 'WASM failed to load' })

    expect(store().status).toBe('error')
    expect(store().error).toBe('WASM failed to load')
  })

  it('uses Unknown error fallback when error message is undefined', () => {
    store().start()
    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'error' })
    expect(store().error).toBe('Unknown error')
  })

  it('handles state message with undefined payload (sets null)', () => {
    store().start()
    const mockWorker = store().worker as unknown as MockWorker
    mockWorker._emit({ type: 'ready' })
    mockWorker._emit({ type: 'state' })
    expect(store().state).toBeNull()
  })

  it('calls postMessage on existing worker when start called again', () => {
    const mockWorker = new MockWorker()
    useSimulationStore.setState({ worker: mockWorker as unknown as Worker, status: 'ready' })
    store().start(0.1)
    expect(mockWorker.postMessage).toHaveBeenCalledWith({ type: 'start', dt: 0.1 })
    expect(store().status).toBe('running')
  })
})
