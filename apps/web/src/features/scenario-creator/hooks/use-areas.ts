import { useState, useEffect, useCallback } from 'react'
import type { AreaDTO } from '@ymir/types'

const API_BASE = import.meta.env.VITE_API_URL ?? 'http://localhost:3000'

export function useAreas() {
  const [areas, setAreas] = useState<AreaDTO[]>([])
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)
  const [tick, setTick] = useState(0)

  const retry = useCallback(() => setTick(t => t + 1), [])

  useEffect(() => {
    let cancelled = false
    setLoading(true)
    setError(null)
    fetch(`${API_BASE}/areas`)
      .then(r => {
        if (!r.ok) throw new Error(`HTTP ${r.status}`)
        return r.json() as Promise<AreaDTO[]>
      })
      .then(data => { if (!cancelled) { setAreas(data); setLoading(false) } })
      .catch(err => { if (!cancelled) { setError(String(err)); setLoading(false) } })
    return () => { cancelled = true }
  }, [tick])

  return { areas, loading, error, retry }
}
