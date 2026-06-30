import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { TopBar } from '../../ui/TopBar'
import { IconButton } from '../../ui/IconButton'
import { Button } from '../../ui/Button'
import { SimulationControl } from '../../ui/SimulationControl'
import { MapActions } from '../../ui/MapActions'
import { AlertDialog } from '../../ui/Modal'
import { Sidebar } from '../scenario-creator/components/Sidebar'
import { AreaMapView } from '../scenario-creator/components/AreaMapView'
import { Area3DView } from '../scenario-creator/components/Area3DView'
import { ViewToggle } from '../scenario-creator/components/ViewToggle'
import { VesselPanel } from '../scenario-creator/components/VesselPanel'
import { VesselPicker } from '../scenario-creator/components/VesselPicker'
import { ScenarioInfoPanel } from '../scenario-info/ScenarioInfoPanel'
import { useScenarioStore } from '../scenario-creator/store'
import { useSimulationStore } from '../../stores/simulationStore'
import { useMapStore } from '../../stores/mapStore'
import { useViewStore } from '../../stores/viewStore'
import { tokens } from '../../theme/tokens'

const API_BASE = import.meta.env.VITE_API_URL ?? 'http://localhost:3000'

/**
 * Instructor workspace shell (Figma "Layout do instrutor"): a top bar over a
 * full-bleed map, with a floating Play/Pause control and a zoom control. The
 * scenario-setup sidebar and the per-vessel panel dock on top of the map.
 */
export function AppShell() {
  const { name, vessels, setName, toCreateScenarioDTO } = useScenarioStore()
  const { status, state, scenarioVessels, play, pause, loadScenario, applyEnvironment } = useSimulationStore()
  const { zoomIn, zoomOut } = useMapStore()
  const { mode } = useViewStore()
  const [confirmNoOwnship, setConfirmNoOwnship] = useState(false)
  const [infoOpen, setInfoOpen] = useState(false)
  const [pickerOpen, setPickerOpen] = useState(false)
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

  // Play boots the engine on first use and resumes after a pause. If vessels were
  // added/removed while paused, reseed the engine — existing vessels keep their
  // current (live) position so the scene doesn't jump back to the start.
  function handlePlay() {
    const sim = useSimulationStore.getState()
    if (!sim.worker) {
      loadScenario(vessels)
      play()
      return
    }
    const loaded = scenarioVessels
    const setChanged =
      vessels.length !== loaded.length ||
      vessels.some(v => !loaded.some(l => l.instanceId === v.instanceId))
    if (setChanged) {
      const live = state?.vessels ?? []
      const reseed = vessels.map(v => {
        const lv = live.find(s => s.id === v.instanceId)
        return lv ? { ...v, x: lv.x, y: lv.y, headingDeg: (lv.psi * 180) / Math.PI } : v
      })
      loadScenario(reseed)
    }
    applyEnvironment()
    play()
  }

  const logo = (
    <div style={{ display: 'flex', alignItems: 'center', gap: tokens.space.md }}>
      <div style={{ display: 'flex', alignItems: 'center', gap: tokens.space.xs, fontSize: tokens.fontSize.title }} aria-hidden="true">
        ▣ <span style={{ fontSize: tokens.fontSize.sm, color: tokens.color.textSubtle }}>▾</span>
      </div>
      <nav aria-label="Caminho do cenário" style={{ display: 'flex', alignItems: 'center', gap: tokens.space.sm, fontSize: tokens.fontSize.body, color: tokens.color.textSubtle }}>
        <span>Pasta pai</span>
        <span aria-hidden="true">›</span>
        <input
          value={name}
          onChange={e => setName(e.target.value)}
          aria-label="Nome do cenário"
          style={{
            border: '1px solid transparent', background: 'transparent',
            color: tokens.color.textPrimary, fontWeight: tokens.fontWeight.medium,
            fontSize: tokens.fontSize.body, fontFamily: tokens.font.sans,
            padding: '2px 4px', borderRadius: tokens.radius.sm, width: 160,
          }}
          onFocus={e => { e.currentTarget.style.borderColor = tokens.color.border }}
          onBlur={e => { e.currentTarget.style.borderColor = 'transparent' }}
        />
      </nav>
    </div>
  )

  const center = (
    <div style={{ display: 'flex', alignItems: 'center', gap: tokens.space.sm }}>
      <IconButton icon="+" label="Adicionar embarcação" variant={pickerOpen ? 'active' : 'accent'} onClick={() => setPickerOpen(o => !o)} />
      <IconButton icon="ⓘ" label="Informações do cenário" variant={infoOpen ? 'active' : 'ghost'} onClick={() => setInfoOpen(o => !o)} />
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
    <div style={{ position: 'relative', display: 'flex', flexDirection: 'column', height: '100vh', overflow: 'hidden' }}>
      <TopBar left={logo} center={center} right={right} />
      <VesselPicker open={pickerOpen} onClose={() => setPickerOpen(false)} />
      <div style={{ flex: 1, display: 'flex', minHeight: 0 }}>
        <Sidebar />
        <div style={{ flex: 1, position: 'relative' }}>
          {mode === 'map' ? <AreaMapView /> : <Area3DView />}
          <ViewToggle />
          <VesselPanel />
          <ScenarioInfoPanel open={infoOpen} onClose={() => setInfoOpen(false)} />
          {mode === 'map' && <MapActions onZoomIn={zoomIn} onZoomOut={zoomOut} />}
          <SimulationControl
            running={status === 'running'}
            disabled={vessels.length === 0}
            onPlay={handlePlay}
            onPause={pause}
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
