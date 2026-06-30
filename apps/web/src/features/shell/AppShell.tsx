import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { TopBar } from '../../ui/TopBar'
import { IconButton } from '../../ui/IconButton'
import { Button } from '../../ui/Button'
import { SimulationControl, type SimulationState } from '../../ui/SimulationControl'
import { MapActions } from '../../ui/MapActions'
import { AlertDialog } from '../../ui/Modal'
import { Sidebar } from '../scenario-creator/components/Sidebar'
import { AreaMapView } from '../scenario-creator/components/AreaMapView'
import { VesselPanel } from '../scenario-creator/components/VesselPanel'
import { useScenarioStore } from '../scenario-creator/store'
import { useSimulationStore } from '../../stores/simulationStore'
import { useMapStore } from '../../stores/mapStore'
import { tokens } from '../../theme/tokens'

const API_BASE = import.meta.env.VITE_API_URL ?? 'http://localhost:3000'

function pillState(status: string): SimulationState {
  if (status === 'running') return 'running'
  if (status === 'loading') return 'building'
  if (status === 'ready') return 'ready'
  return 'idle'
}

/**
 * Instructor workspace shell (Figma "Layout do instrutor"): a top bar over a
 * full-bleed map, with a floating Build/Play/Stop pill and a zoom control. The
 * scenario-setup sidebar and the per-vessel panel dock on top of the map.
 */
export function AppShell() {
  const { name, vessels, toCreateScenarioDTO } = useScenarioStore()
  const { status, start, stop, loadScenario } = useSimulationStore()
  const { zoomIn, zoomOut } = useMapStore()
  const [confirmNoOwnship, setConfirmNoOwnship] = useState(false)
  const navigate = useNavigate()

  async function saveScenario() {
    try {
      const res = await fetch(`${API_BASE}/scenarios`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(toCreateScenarioDTO()),
      })
      if (!res.ok) console.error('Failed to save scenario:', res.status)
    } catch (err) {
      console.error('Network error saving scenario:', err)
    }
  }

  function handleSave() {
    if (vessels.length === 0) {
      setConfirmNoOwnship(true)
      return
    }
    void saveScenario()
  }

  function handleBuild() {
    void saveScenario()
    loadScenario(vessels)
    start()
  }

  const logo = (
    <div style={{ display: 'flex', alignItems: 'center', gap: tokens.space.md }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: tokens.space.xs, fontSize: tokens.fontSize.title }} aria-hidden="true">
        ▣ <span style={{ fontSize: tokens.fontSize.sm, color: tokens.color.textSubtle }}>▾</span>
      </div>
      <nav aria-label="Caminho do cenário" style={{ display: 'flex', alignItems: 'center', gap: tokens.space.sm, fontSize: tokens.fontSize.body, color: tokens.color.textSubtle }}>
        <span>Pasta pai</span>
        <span aria-hidden="true">›</span>
        <span style={{ color: tokens.color.textPrimary, fontWeight: tokens.fontWeight.medium }}>{name}</span>
      </nav>
    </div>
  )

  const center = (
    <div style={{ display: 'flex', alignItems: 'center', gap: tokens.space.sm }}>
      <IconButton icon="+" label="Adicionar embarcação" variant="accent" />
      <IconButton icon="ⓘ" label="Informações do cenário" />
      <IconButton icon="☁" label="Sincronizar" />
      <IconButton icon="⚙" label="Configurações" />
    </div>
  )

  const right = (
    <div style={{ display: 'flex', alignItems: 'center', gap: tokens.space.sm }}>
      <Button variant="secondary" size="sm" onClick={() => navigate('/')}>Sair do cenário</Button>
      <Button variant="dark" size="sm" icon="💾" onClick={handleSave}>Salvar cenário</Button>
    </div>
  )

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100vh', overflow: 'hidden' }}>
      <TopBar left={logo} center={center} right={right} actionsDisabled={vessels.length === 0 ? 'center' : 'none'} />
      <div style={{ flex: 1, display: 'flex', minHeight: 0 }}>
        <Sidebar />
        <div style={{ flex: 1, position: 'relative' }}>
          <AreaMapView />
          <VesselPanel />
          <MapActions onZoomIn={zoomIn} onZoomOut={zoomOut} />
          <SimulationControl
            state={pillState(status)}
            onBuild={handleBuild}
            onPlay={start}
            onStop={stop}
          />
        </div>
      </div>
      <AlertDialog
        open={confirmNoOwnship}
        title="Seu cenário não possui um ownship"
        body="Seu cenário não possui um ownship, portanto não será possível utilizá-lo em uma cabine. Tem certeza que deseja salvar?"
        cancelLabel="Cancelar"
        confirmLabel="Salvar mesmo assim"
        onCancel={() => setConfirmNoOwnship(false)}
        onConfirm={() => { setConfirmNoOwnship(false); void saveScenario() }}
      />
    </div>
  )
}
