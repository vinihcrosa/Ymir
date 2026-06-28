import { renderHook, waitFor } from '@testing-library/react'
import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import { useAreas } from './use-areas'
import type { AreaDTO } from '@ymir/types'

const MOCK_AREA: AreaDTO = {
  id: 1,
  name: 'test area',
  origin: { latitude: -22.9, longitude: -43.2 },
  polygon: {
    type: 'Polygon',
    coordinates: [
      [
        [-43.3, -23.0],
        [-43.1, -23.0],
        [-43.1, -22.8],
        [-43.3, -22.8],
        [-43.3, -23.0],
      ],
    ],
  },
  gravity: 9.81,
  magneticCorrection: -22.5,
  waterDensity: 1.025,
  airDensity: 0.001275,
  createdAt: '2024-01-01T00:00:00Z',
}

function makeFetchResponse(body: unknown, ok = true, status = 200): Response {
  return {
    ok,
    status,
    json: () => Promise.resolve(body),
  } as unknown as Response
}

describe('useAreas', () => {
  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn())
  })

  afterEach(() => {
    vi.unstubAllGlobals()
  })

  it('returns loading=true initially', () => {
    vi.mocked(fetch).mockReturnValue(new Promise(() => {}))

    const { result } = renderHook(() => useAreas())

    expect(result.current.loading).toBe(true)
    expect(result.current.areas).toEqual([])
    expect(result.current.error).toBeNull()
  })

  it('returns areas on successful fetch', async () => {
    vi.mocked(fetch).mockResolvedValue(makeFetchResponse([MOCK_AREA]))

    const { result } = renderHook(() => useAreas())

    await waitFor(() => expect(result.current.loading).toBe(false))

    expect(result.current.areas).toEqual([MOCK_AREA])
    expect(result.current.error).toBeNull()
  })

  it('returns error on network failure', async () => {
    vi.mocked(fetch).mockRejectedValue(new Error('Network error'))

    const { result } = renderHook(() => useAreas())

    await waitFor(() => expect(result.current.loading).toBe(false))

    expect(result.current.error).toBe('Error: Network error')
    expect(result.current.areas).toEqual([])
  })

  it('returns error on non-ok HTTP response', async () => {
    vi.mocked(fetch).mockResolvedValue(makeFetchResponse(null, false, 500))

    const { result } = renderHook(() => useAreas())

    await waitFor(() => expect(result.current.loading).toBe(false))

    expect(result.current.error).toBe('Error: HTTP 500')
    expect(result.current.areas).toEqual([])
  })

  it('retry() re-triggers fetch', async () => {
    vi.mocked(fetch)
      .mockRejectedValueOnce(new Error('Network error'))
      .mockResolvedValueOnce(makeFetchResponse([MOCK_AREA]))

    const { result } = renderHook(() => useAreas())

    await waitFor(() => expect(result.current.loading).toBe(false))
    expect(result.current.error).toBeTruthy()

    result.current.retry()

    await waitFor(() => {
      expect(result.current.loading).toBe(false)
      expect(result.current.error).toBeNull()
    })

    expect(result.current.areas).toEqual([MOCK_AREA])
    expect(vi.mocked(fetch)).toHaveBeenCalledTimes(2)
  })
})
