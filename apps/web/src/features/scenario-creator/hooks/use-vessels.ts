import { useState, useEffect } from 'react'
import type { VesselDTO } from '@ymir/types'

const API_BASE = import.meta.env.VITE_API_URL ?? 'http://localhost:3000'

export function useVessels() {
  const [vessels, setVessels] = useState<VesselDTO[]>([])
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    let cancelled = false
    setLoading(true)
    setError(null)
    fetch(`${API_BASE}/vessels`)
      .then(r => {
        if (!r.ok) throw new Error(`HTTP ${r.status}`)
        return r.json() as Promise<VesselDTO[]>
      })
      .then(data => { if (!cancelled) { setVessels(data); setLoading(false) } })
      .catch(err => { if (!cancelled) { setError(String(err)); setLoading(false) } })
    return () => { cancelled = true }
  }, [])

  return { vessels, loading, error }
}
