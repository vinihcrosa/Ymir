import { useEffect, useState } from 'react'
import type { VesselConfigDTO, VesselRudderDTO, VesselThrusterDTO } from '@ymir/types'
import { useVesselPanelStore } from '../../../stores/vesselPanelStore'
import { useSimulationStore } from '../../../stores/simulationStore'
import { useScenarioStore } from '../store'

const API_BASE = import.meta.env.VITE_API_URL ?? 'http://localhost:3000'

const panel: React.CSSProperties = {
  position: 'absolute',
  top: 16,
  right: 16,
  width: 380,
  maxHeight: 'calc(100vh - 32px)',
  background: '#fff',
  borderRadius: 8,
  boxShadow: '0 4px 24px rgba(0,0,0,0.18)',
  display: 'flex',
  flexDirection: 'column',
  zIndex: 1000,
  overflow: 'hidden',
}

const header: React.CSSProperties = {
  padding: '14px 16px 10px',
  borderBottom: '1px solid #f0f0f0',
}

const tabBar: React.CSSProperties = {
  display: 'flex',
  borderBottom: '1px solid #e5e7eb',
  padding: '0 16px',
}

const content: React.CSSProperties = {
  flex: 1,
  overflowY: 'auto',
  padding: '16px',
}

const section: React.CSSProperties = {
  marginBottom: 20,
}

const sectionTitle: React.CSSProperties = {
  fontSize: 11,
  fontWeight: 600,
  color: '#6b7280',
  textTransform: 'uppercase',
  letterSpacing: '0.05em',
  marginBottom: 8,
}

const row: React.CSSProperties = {
  display: 'flex',
  justifyContent: 'space-between',
  alignItems: 'center',
  padding: '5px 0',
  borderBottom: '1px solid #f9fafb',
}

const rowLabel: React.CSSProperties = { fontSize: 13, color: '#6b7280', minWidth: 80 }
const rowValue: React.CSSProperties = { fontSize: 13, color: '#111827', fontWeight: 500 }

const card: React.CSSProperties = {
  background: '#f9fafb',
  border: '1px solid #e5e7eb',
  borderRadius: 6,
  padding: '12px 14px',
  marginBottom: 10,
}

const cardTitle: React.CSSProperties = {
  fontSize: 12,
  fontWeight: 600,
  color: '#374151',
  marginBottom: 10,
}

function SliderRow({ label, value, min, max, step = 1, unit, onChange }: {
  label: string
  value: number
  min: number
  max: number
  step?: number
  unit: string
  onChange: (v: number) => void
}) {
  return (
    <div style={{ marginBottom: 10 }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 4 }}>
        <span style={{ fontSize: 12, color: '#6b7280' }}>{label}</span>
        <span style={{ fontSize: 12, fontWeight: 600, color: '#111827', minWidth: 48, textAlign: 'right' }}>
          {value.toFixed(value === Math.round(value) ? 0 : 1)}{unit}
        </span>
      </div>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
        style={{ width: '100%', accentColor: '#1a73e8' }}
      />
      <div style={{ display: 'flex', justifyContent: 'space-between' }}>
        <span style={{ fontSize: 10, color: '#9ca3af' }}>{min}{unit}</span>
        <span style={{ fontSize: 10, color: '#9ca3af' }}>{max}{unit}</span>
      </div>
    </div>
  )
}

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
    <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', marginBottom: 8 }}>
      <svg
        width={size}
        height={size}
        style={{ cursor: 'pointer', userSelect: 'none' }}
        onClick={handleSvgClick}
      >
        <circle cx={cx} cy={cy} r={r} fill="none" stroke="#e5e7eb" strokeWidth={2} />
        {[-150, -120, -90, -60, -30, 0, 30, 60, 90, 120, 150, 180].map((deg) => {
          const a = (deg * Math.PI) / 180
          const x1 = cx + (r - 4) * Math.sin(a)
          const y1 = cy - (r - 4) * Math.cos(a)
          const x2 = cx + r * Math.sin(a)
          const y2 = cy - r * Math.cos(a)
          return <line key={deg} x1={x1} y1={y1} x2={x2} y2={y2} stroke="#d1d5db" strokeWidth={1} />
        })}
        {/* 0 tick mark */}
        <text x={cx} y={cy - r - 6} textAnchor="middle" fontSize={9} fill="#9ca3af">0</text>
        <line x1={cx} y1={cy} x2={ix} y2={iy} stroke="#1a73e8" strokeWidth={2.5} strokeLinecap="round" />
        <circle cx={ix} cy={iy} r={4} fill="#1a73e8" />
        <circle cx={cx} cy={cy} r={3} fill="#374151" />
      </svg>
      <span style={{ fontSize: 11, color: '#6b7280', marginTop: 2 }}>Azimute: <b style={{ color: '#111827' }}>{value}°</b></span>
    </div>
  )
}

function RudderCard({ rudder, angle, onAngleChange }: {
  rudder: VesselRudderDTO
  angle: number
  onAngleChange: (deg: number) => void
}) {
  return (
    <div style={card}>
      <div style={cardTitle}>⚓ {rudder.name}</div>
      <SliderRow
        label="Ângulo"
        value={angle}
        min={-rudder.angleMaximum}
        max={rudder.angleMaximum}
        unit="°"
        onChange={onAngleChange}
      />
    </div>
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
    <div style={card}>
      <div style={cardTitle}>
        {isAzimuthal ? '🔄' : '➡️'} {thruster.name}
        <span style={{ fontWeight: 400, color: '#9ca3af', marginLeft: 6, fontSize: 11 }}>
          {isAzimuthal ? 'azimutal' : 'tunnel'} · {thruster.maximumPower} kW
        </span>
      </div>
      {isAzimuthal && (
        <AzimuthDial value={azimuth} onChange={onAzimuthChange} />
      )}
      <SliderRow
        label="Potência"
        value={power}
        min={-100}
        max={100}
        unit="%"
        onChange={onPowerChange}
      />
    </div>
  )
}

function GeralTab({ config, vesselId }: { config: VesselConfigDTO; vesselId: number }) {
  const { status, state } = useSimulationStore()
  const { vessels: draftVessels } = useScenarioStore()
  const isRunning = status === 'running'

  const liveVessel = isRunning ? state?.vessels.find(v => v.id === vesselId) : null
  const draft = draftVessels.find(v => v.vesselId === vesselId)

  const x = liveVessel ? liveVessel.x : (draft?.x ?? 0)
  const y = liveVessel ? liveVessel.y : (draft?.y ?? 0)
  const headingDeg = liveVessel
    ? (liveVessel.psi * 180 / Math.PI + 360) % 360
    : (draft?.headingDeg ?? 0)
  const u = liveVessel?.u ?? 0
  const v = liveVessel?.v ?? 0
  const r = liveVessel?.r ?? 0

  return (
    <>
      <div style={section}>
        <div style={sectionTitle}>Identificação</div>
        <div style={row}>
          <span style={rowLabel}>Nome</span>
          <span style={{ ...rowValue, fontSize: 11, maxWidth: 220, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{config.name}</span>
        </div>
        <div style={row}>
          <span style={rowLabel}>ID</span>
          <span style={rowValue}>{config.id}</span>
        </div>
      </div>

      <div style={section}>
        <div style={sectionTitle}>Dimensões</div>
        <div style={row}>
          <span style={rowLabel}>LOA</span>
          <span style={rowValue}>{config.lpp.toFixed(1)} m</span>
        </div>
        <div style={row}>
          <span style={rowLabel}>Boca</span>
          <span style={rowValue}>{config.beam.toFixed(1)} m</span>
        </div>
        <div style={row}>
          <span style={rowLabel}>Calado</span>
          <span style={rowValue}>{config.draft.toFixed(1)} m</span>
        </div>
        <div style={row}>
          <span style={rowLabel}>Deslocamento</span>
          <span style={rowValue}>{(config.physicalProps.displacementWeight / 1000).toFixed(0)} kt</span>
        </div>
      </div>

      <div style={section}>
        <div style={sectionTitle}>Posição e curso {isRunning && <span style={{ color: '#10b981', fontWeight: 400, textTransform: 'none' }}>● live</span>}</div>
        <div style={row}>
          <span style={rowLabel}>X</span>
          <span style={rowValue}>{x.toFixed(1)} m</span>
        </div>
        <div style={row}>
          <span style={rowLabel}>Y</span>
          <span style={rowValue}>{y.toFixed(1)} m</span>
        </div>
        <div style={row}>
          <span style={rowLabel}>Rumo</span>
          <span style={rowValue}>{headingDeg.toFixed(1)} °</span>
        </div>
        {isRunning && (
          <>
            <div style={row}>
              <span style={rowLabel}>u (surge)</span>
              <span style={rowValue}>{u.toFixed(2)} m/s</span>
            </div>
            <div style={row}>
              <span style={rowLabel}>v (sway)</span>
              <span style={rowValue}>{v.toFixed(2)} m/s</span>
            </div>
            <div style={row}>
              <span style={rowLabel}>r (yaw)</span>
              <span style={rowValue}>{r.toFixed(4)} rad/s</span>
            </div>
          </>
        )}
      </div>

      <div style={section}>
        <div style={sectionTitle}>Propulsão</div>
        <div style={row}>
          <span style={rowLabel}>Propulsores</span>
          <span style={rowValue}>{config.thrusters.length}</span>
        </div>
        <div style={row}>
          <span style={rowLabel}>Lemes</span>
          <span style={rowValue}>{config.rudders.length}</span>
        </div>
        <div style={row}>
          <span style={rowLabel}>Pot. total</span>
          <span style={rowValue}>{config.thrusters.reduce((s, t) => s + t.maximumPower, 0).toFixed(0)} kW</span>
        </div>
      </div>
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
    const azimuth = thrusterAzimuths[thrusterId] ?? 0
    worker?.postMessage({ type: 'setActuator', vesselId, deviceType: 'thruster', deviceId: thrusterId, value: pct, value2: azimuth })
  }

  function sendThrusterAzimuth(thrusterId: number, deg: number) {
    setThrusterAzimuth(thrusterId, deg)
    const power = thrusterPowers[thrusterId] ?? 0
    worker?.postMessage({ type: 'setActuator', vesselId, deviceType: 'thruster', deviceId: thrusterId, value: power, value2: deg })
  }

  if (config.rudders.length === 0 && config.thrusters.length === 0) {
    return <p style={{ color: '#6b7280', fontSize: 13, textAlign: 'center', marginTop: 32 }}>Sem propulsores ou lemes configurados.</p>
  }

  return (
    <>
      {config.rudders.length > 0 && (
        <div style={section}>
          <div style={sectionTitle}>Lemes</div>
          {config.rudders.map(r => (
            <RudderCard
              key={r.rudderId}
              rudder={r}
              angle={rudderAngles[r.rudderId] ?? 0}
              onAngleChange={(deg) => sendRudder(r.rudderId, deg)}
            />
          ))}
        </div>
      )}

      {config.thrusters.length > 0 && (
        <div style={section}>
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

type PanelTab = 'geral' | 'controles'

export function VesselPanel() {
  const { selectedVesselId, vesselTypeId, config, configLoading, close, setConfig } = useVesselPanelStore()
  const [activeTab, setActiveTab] = useState<PanelTab>('geral')

  useEffect(() => {
    if (selectedVesselId == null || vesselTypeId == null) return
    let cancelled = false
    setConfig(null, true)
    fetch(`${API_BASE}/vessels/${vesselTypeId}/config`)
      .then(r => r.ok ? r.json() as Promise<VesselConfigDTO> : null)
      .then(cfg => { if (!cancelled) setConfig(cfg, false) })
      .catch(() => { if (!cancelled) setConfig(null, false) })
    return () => { cancelled = true }
  }, [selectedVesselId, setConfig])

  if (selectedVesselId == null) return null

  function tabStyle(tab: PanelTab): React.CSSProperties {
    return {
      padding: '8px 14px',
      fontSize: 13,
      fontWeight: activeTab === tab ? 600 : 400,
      color: activeTab === tab ? '#1a73e8' : '#6b7280',
      cursor: 'pointer',
      background: 'none',
      border: 'none',
      borderBottom: activeTab === tab ? '2px solid #1a73e8' : '2px solid transparent',
      marginBottom: -1,
    }
  }

  return (
    <div style={panel} data-testid="vessel-panel">
      <div style={header}>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
          <div>
            <div style={{ fontSize: 10, color: '#9ca3af', fontWeight: 500, textTransform: 'uppercase', letterSpacing: '0.06em' }}>Ownship</div>
            <div style={{ fontSize: 14, fontWeight: 700, color: '#111827', marginTop: 2, maxWidth: 300, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
              {config?.name ?? `Vessel ${selectedVesselId}`}
            </div>
          </div>
          <button
            onClick={close}
            style={{ background: 'none', border: 'none', cursor: 'pointer', fontSize: 18, color: '#6b7280', padding: '0 4px', lineHeight: 1, marginTop: -2 }}
            aria-label="Fechar painel"
          >
            ×
          </button>
        </div>
      </div>

      <div style={tabBar}>
        <button style={tabStyle('geral')} onClick={() => setActiveTab('geral')}>Geral</button>
        <button style={tabStyle('controles')} onClick={() => setActiveTab('controles')}>Controles</button>
      </div>

      <div style={content}>
        {configLoading && (
          <p style={{ textAlign: 'center', color: '#9ca3af', fontSize: 13, marginTop: 32 }}>Carregando...</p>
        )}
        {!configLoading && !config && (
          <p style={{ textAlign: 'center', color: '#ef4444', fontSize: 13, marginTop: 32 }}>Falha ao carregar configuração.</p>
        )}
        {!configLoading && config && activeTab === 'geral' && (
          <GeralTab config={config} vesselId={selectedVesselId} />
        )}
        {!configLoading && config && activeTab === 'controles' && (
          <ControlesTab config={config} vesselId={selectedVesselId} />
        )}
      </div>
    </div>
  )
}
