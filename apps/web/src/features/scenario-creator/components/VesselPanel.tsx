import { useEffect } from 'react'
import type { CSSProperties } from 'react'
import type { VesselConfigDTO, VesselRudderDTO, VesselThrusterDTO } from '@ymir/types'
import { useVesselPanelStore } from '../../../stores/vesselPanelStore'
import { useSimulationStore } from '../../../stores/simulationStore'
import { useScenarioStore } from '../store'
import { Panel } from '../../../ui/Panel'
import { Tabs } from '../../../ui/Tabs'
import { Card } from '../../../ui/Card'
import { Field } from '../../../ui/Field'
import { Slider } from '../../../ui/Slider'
import { tokens } from '../../../theme/tokens'

const API_BASE = import.meta.env.VITE_API_URL ?? 'http://localhost:3000'

function AzimuthDial({ value, onChange }: { value: number; onChange: (v: number) => void }) {
  const size = 120
  const cx = size / 2
  const cy = size / 2
  const r = 48
  const rad = (value * Math.PI) / 180
  const ix = cx + r * Math.sin(rad)
  const iy = cy - r * Math.cos(rad)

  function handleSvgClick(e: React.MouseEvent<SVGSVGElement>) {
    const rect = e.currentTarget.getBoundingClientRect()
    const mx = e.clientX - rect.left - cx
    const my = e.clientY - rect.top - cy
    const angle = Math.atan2(mx, -my) * (180 / Math.PI)
    onChange(Math.round(angle))
  }

  return (
    <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', marginBottom: tokens.space.sm }}>
      <svg width={size} height={size} style={{ cursor: 'pointer', userSelect: 'none' }} onClick={handleSvgClick} role="img" aria-label="Azimute do propulsor">
        <circle cx={cx} cy={cy} r={r} fill="none" stroke={tokens.color.border} strokeWidth={2} />
        {[-150, -120, -90, -60, -30, 0, 30, 60, 90, 120, 150, 180].map((deg) => {
          const a = (deg * Math.PI) / 180
          const x1 = cx + (r - 4) * Math.sin(a)
          const y1 = cy - (r - 4) * Math.cos(a)
          const x2 = cx + r * Math.sin(a)
          const y2 = cy - r * Math.cos(a)
          return <line key={deg} x1={x1} y1={y1} x2={x2} y2={y2} stroke={tokens.color.surfaceActive} strokeWidth={1} />
        })}
        <text x={cx} y={cy - r - 6} textAnchor="middle" fontSize={9} fill={tokens.color.textHcSubtle}>0</text>
        <line x1={cx} y1={cy} x2={ix} y2={iy} stroke={tokens.color.accent} strokeWidth={2.5} strokeLinecap="round" />
        <circle cx={ix} cy={iy} r={4} fill={tokens.color.accent} />
        <circle cx={cx} cy={cy} r={3} fill={tokens.color.textTertiary} />
      </svg>
      <span style={{ fontSize: tokens.fontSize.xs + 1, color: tokens.color.textSubtle, marginTop: 2 }}>
        Azimute: <b style={{ color: tokens.color.textPrimary }}>{value}°</b>
      </span>
    </div>
  )
}

function RudderCard({ rudder, angle, onAngleChange }: {
  rudder: VesselRudderDTO
  angle: number
  onAngleChange: (deg: number) => void
}) {
  return (
    <Card title={`⚓ ${rudder.name}`} tone="alt">
      <Slider label="Ângulo" value={angle} min={-rudder.angleMaximum} max={rudder.angleMaximum} unit="°" onChange={onAngleChange} />
    </Card>
  )
}

function ThrusterCard({ thruster, power, azimuth, onPowerChange, onAzimuthChange }: {
  thruster: VesselThrusterDTO
  power: number
  azimuth: number
  onPowerChange: (pct: number) => void
  onAzimuthChange: (deg: number) => void
}) {
  const isAzimuthal = thruster.azimuthSpeed > 0
  return (
    <Card title={`${isAzimuthal ? '🔄' : '➡️'} ${thruster.name} · ${isAzimuthal ? 'azimutal' : 'tunnel'} · ${thruster.maximumPower} kW`} tone="alt">
      {isAzimuthal && <AzimuthDial value={azimuth} onChange={onAzimuthChange} />}
      <Slider label="Potência" value={power} min={-100} max={100} unit="%" onChange={onPowerChange} />
    </Card>
  )
}

function GeralTab({ config, vesselId }: { config: VesselConfigDTO; vesselId: number }) {
  const { status, state } = useSimulationStore()
  const { vessels: draftVessels } = useScenarioStore()
  const isRunning = status === 'running'

  // `vesselId` is the instance id (selectedVesselId). Live state and draft list
  // are both keyed by instance id — never the vessel-type id.
  const liveVessel = isRunning ? state?.vessels.find(v => v.id === vesselId) : null
  const draft = draftVessels.find(v => v.instanceId === vesselId)

  const x = liveVessel ? liveVessel.x : (draft?.x ?? 0)
  const y = liveVessel ? liveVessel.y : (draft?.y ?? 0)
  const headingDeg = liveVessel ? (liveVessel.psi * 180 / Math.PI + 360) % 360 : (draft?.headingDeg ?? 0)
  const u = liveVessel?.u ?? 0
  const v = liveVessel?.v ?? 0
  const r = liveVessel?.r ?? 0

  return (
    <>
      <Card title="Identificação">
        <Field label="Nome"><span style={{ fontSize: tokens.fontSize.sm }}>{config.name}</span></Field>
        <Field label="ID">{config.id}</Field>
      </Card>
      <Card title="Dimensões">
        <Field label="LOA" mono>{config.lpp.toFixed(1)} m</Field>
        <Field label="Boca" mono>{config.beam.toFixed(1)} m</Field>
        <Field label="Calado" mono>{config.draft.toFixed(1)} m</Field>
        <Field label="Deslocamento" mono>{(config.physicalProps.displacementWeight / 1000).toFixed(0)} kt</Field>
      </Card>
      <Card title={isRunning ? 'Posição e curso ● live' : 'Posição e curso'}>
        <Field label="X" mono>{x.toFixed(1)} m</Field>
        <Field label="Y" mono>{y.toFixed(1)} m</Field>
        <Field label="Rumo" mono>{headingDeg.toFixed(1)} °</Field>
        {isRunning && (
          <>
            <Field label="u (surge)" mono>{u.toFixed(2)} m/s</Field>
            <Field label="v (sway)" mono>{v.toFixed(2)} m/s</Field>
            <Field label="r (yaw)" mono>{r.toFixed(4)} rad/s</Field>
          </>
        )}
      </Card>
      <Card title="Propulsão">
        <Field label="Propulsores" mono>{config.thrusters.length}</Field>
        <Field label="Lemes" mono>{config.rudders.length}</Field>
        <Field label="Pot. total" mono>{config.thrusters.reduce((s, t) => s + t.maximumPower, 0).toFixed(0)} kW</Field>
      </Card>
    </>
  )
}

function ControlesTab({ config, vesselId }: { config: VesselConfigDTO; vesselId: number }) {
  const {
    rudderAngles, thrusterPowers, thrusterAzimuths,
    setRudderAngle, setThrusterPower, setThrusterAzimuth,
  } = useVesselPanelStore()
  const { worker } = useSimulationStore()

  function sendRudder(rudderId: number, deg: number) {
    setRudderAngle(rudderId, deg)
    worker?.postMessage({ type: 'setActuator', vesselId, deviceType: 'rudder', deviceId: rudderId, value: deg })
  }
  function sendThrusterPower(thrusterId: number, pct: number) {
    setThrusterPower(thrusterId, pct)
    worker?.postMessage({ type: 'setActuator', vesselId, deviceType: 'thruster', deviceId: thrusterId, value: pct, value2: thrusterAzimuths[thrusterId] ?? 0 })
  }
  function sendThrusterAzimuth(thrusterId: number, deg: number) {
    setThrusterAzimuth(thrusterId, deg)
    worker?.postMessage({ type: 'setActuator', vesselId, deviceType: 'thruster', deviceId: thrusterId, value: thrusterPowers[thrusterId] ?? 0, value2: deg })
  }

  if (config.rudders.length === 0 && config.thrusters.length === 0) {
    return <p style={{ color: tokens.color.textSubtle, fontSize: tokens.fontSize.label, textAlign: 'center', marginTop: 32 }}>Sem propulsores ou lemes configurados.</p>
  }

  const sectionTitle: CSSProperties = {
    fontSize: tokens.fontSize.sm, fontWeight: tokens.fontWeight.semibold, color: tokens.color.textSubtle,
    textTransform: 'uppercase', letterSpacing: '0.05em', margin: `${tokens.space.sm}px 0`,
  }

  return (
    <>
      {config.rudders.length > 0 && (
        <div>
          <div style={sectionTitle}>Lemes</div>
          {config.rudders.map(r => (
            <RudderCard key={r.rudderId} rudder={r} angle={rudderAngles[r.rudderId] ?? 0} onAngleChange={(deg) => sendRudder(r.rudderId, deg)} />
          ))}
        </div>
      )}
      {config.thrusters.length > 0 && (
        <div>
          <div style={sectionTitle}>Propulsores</div>
          {config.thrusters.map(t => (
            <ThrusterCard
              key={t.thrusterId}
              thruster={t}
              power={thrusterPowers[t.thrusterId] ?? 0}
              azimuth={thrusterAzimuths[t.thrusterId] ?? 0}
              onPowerChange={(pct) => sendThrusterPower(t.thrusterId, pct)}
              onAzimuthChange={(deg) => sendThrusterAzimuth(t.thrusterId, deg)}
            />
          ))}
        </div>
      )}
    </>
  )
}

export function VesselPanel() {
  const { selectedVesselId, vesselTypeId, config, configLoading, close, setConfig } = useVesselPanelStore()

  useEffect(() => {
    if (selectedVesselId == null || vesselTypeId == null) return
    let cancelled = false
    setConfig(null, true)
    fetch(`${API_BASE}/vessels/${vesselTypeId}/config`)
      .then(r => r.ok ? r.json() as Promise<VesselConfigDTO> : null)
      .then(cfg => { if (!cancelled) setConfig(cfg, false) })
      .catch(() => { if (!cancelled) setConfig(null, false) })
    return () => { cancelled = true }
  }, [selectedVesselId, vesselTypeId, setConfig])

  if (selectedVesselId == null) return null

  return (
    <Panel
      data-testid="vessel-panel"
      eyebrow="Embarcação Ownship"
      title={config?.name ?? `Vessel ${selectedVesselId}`}
      onClose={close}
    >
      {configLoading && (
        <p style={{ textAlign: 'center', color: tokens.color.textHcSubtle, fontSize: tokens.fontSize.label, marginTop: 32 }}>Carregando...</p>
      )}
      {!configLoading && !config && (
        <p style={{ textAlign: 'center', color: tokens.color.danger, fontSize: tokens.fontSize.label, marginTop: 32 }}>Falha ao carregar configuração.</p>
      )}
      {!configLoading && config && (
        <Tabs
          tabs={[
            { id: 'geral', label: 'Geral', content: <GeralTab config={config} vesselId={selectedVesselId} /> },
            { id: 'controles', label: 'Controles', content: <ControlesTab config={config} vesselId={selectedVesselId} /> },
          ]}
        />
      )}
    </Panel>
  )
}
