import { renderHook, waitFor } from '@testing-library/react'
import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import { useVessels } from './use-vessels'
import type { VesselDTO } from '@ymir/types'

const MOCK_VESSEL: VesselDTO = {
  id: 1,
  name: 'Test Vessel',
  length: 200,
  beam: 32,
  draft: 12,
  mass: 50000,
  createdAt: '2024-01-01T00:00:00Z',
}

function makeFetchResponse(body: unknown, ok = true, status = 200): Response {
  return {
    ok,
    status,
    json: () => Promise.resolve(body),
  } as unknown as Response
}

describe('useVessels', () => {
  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn())
  })

  afterEach(() => {
    vi.unstubAllGlobals()
  })

  it('returns loading=true initially', () => {
    vi.mocked(fetch).mockReturnValue(new Promise(() => {}))

    const { result } = renderHook(() => useVessels())

    expect(result.current.loading).toBe(true)
    expect(result.current.vessels).toEqual([])
    expect(result.current.error).toBeNull()
  })

  it('returns vessels on successful fetch', async () => {
    vi.mocked(fetch).mockResolvedValue(makeFetchResponse([MOCK_VESSEL]))

    const { result } = renderHook(() => useVessels())

    await waitFor(() => expect(result.current.loading).toBe(false))

    expect(result.current.vessels).toEqual([MOCK_VESSEL])
    expect(result.current.error).toBeNull()
  })

  it('returns error on network failure', async () => {
    vi.mocked(fetch).mockRejectedValue(new Error('Connection refused'))

    const { result } = renderHook(() => useVessels())

    await waitFor(() => expect(result.current.loading).toBe(false))

    expect(result.current.error).toBe('Error: Connection refused')
    expect(result.current.vessels).toEqual([])
  })

  it('returns error on non-ok HTTP response', async () => {
    vi.mocked(fetch).mockResolvedValue(makeFetchResponse(null, false, 404))

    const { result } = renderHook(() => useVessels())

    await waitFor(() => expect(result.current.loading).toBe(false))

    expect(result.current.error).toBe('Error: HTTP 404')
    expect(result.current.vessels).toEqual([])
  })

  it('fetches from /vessels endpoint', async () => {
    vi.mocked(fetch).mockResolvedValue(makeFetchResponse([MOCK_VESSEL]))

    renderHook(() => useVessels())

    await waitFor(() => {
      expect(vi.mocked(fetch)).toHaveBeenCalledWith(
        expect.stringContaining('/vessels'),
      )
    })
  })
})
