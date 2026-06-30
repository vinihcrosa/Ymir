import { useState, useEffect, useCallback } from 'react'
import type { ScenarioDTO } from '@ymir/types'

const API_BASE = import.meta.env.VITE_API_URL ?? 'http://localhost:3000'

export function useScenarios() {
  const [scenarios, setScenarios] = useState<ScenarioDTO[]>([])
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)
  const [tick, setTick] = useState(0)

  const refresh = useCallback(() => setTick(t => t + 1), [])

  useEffect(() => {
    let cancelled = false
    setLoading(true)
    setError(null)
    fetch(`${API_BASE}/scenarios`)
      .then(r => { if (!r.ok) throw new Error(`HTTP ${r.status}`); return r.json() as Promise<ScenarioDTO[]> })
      .then(data => { if (!cancelled) { setScenarios(data); setLoading(false) } })
      .catch(err => { if (!cancelled) { setError(String(err)); setLoading(false) } })
    return () => { cancelled = true }
  }, [tick])

  const remove = useCallback(async (id: number) => {
    await fetch(`${API_BASE}/scenarios/${id}`, { method: 'DELETE' })
    setScenarios(s => s.filter(x => x.id !== id))
  }, [])

  return { scenarios, loading, error, refresh, remove }
}
