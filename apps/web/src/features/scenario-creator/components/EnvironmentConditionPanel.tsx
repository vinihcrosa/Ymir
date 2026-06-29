import { useEnvironmentStore } from '../../../stores/environmentStore'
import type { EnvironmentConditionDTO, WaveConditionDTO } from '@ymir/types'

const panelStyle: React.CSSProperties = {
  background: '#fff',
  border: '1px solid #e5e7eb',
  borderRadius: 8,
  padding: '16px',
}

const sectionStyle: React.CSSProperties = {
  marginBottom: 20,
}

const sectionTitleStyle: React.CSSProperties = {
  fontSize: 11,
  fontWeight: 600,
  color: '#6b7280',
  textTransform: 'uppercase',
  letterSpacing: '0.05em',
  marginBottom: 8,
  display: 'flex',
  justifyContent: 'space-between',
  alignItems: 'center',
}

const cardStyle: React.CSSProperties = {
  background: '#f9fafb',
  border: '1px solid #e5e7eb',
  borderRadius: 6,
  padding: '10px 12px',
  marginBottom: 8,
}

const cardHeaderStyle: React.CSSProperties = {
  display: 'flex',
  justifyContent: 'space-between',
  alignItems: 'center',
  marginBottom: 8,
}

const labelBadgeStyle: React.CSSProperties = {
  fontSize: 11,
  fontWeight: 600,
  color: '#374151',
}

const temporalNoteStyle: React.CSSProperties = {
  fontSize: 10,
  color: '#6b7280',
  marginTop: 4,
  marginBottom: 6,
  fontStyle: 'italic',
}

const kfRowStyle: React.CSSProperties = {
  display: 'flex',
  gap: 6,
  alignItems: 'flex-end',
  marginBottom: 6,
}

const fieldGroupStyle: React.CSSProperties = {
  display: 'flex',
  flexDirection: 'column',
  gap: 2,
}

const labelStyle: React.CSSProperties = {
  fontSize: 10,
  color: '#6b7280',
}

const inputStyle: React.CSSProperties = {
  width: 70,
  padding: '3px 5px',
  fontSize: 12,
  border: '1px solid #d1d5db',
  borderRadius: 4,
}

const helperStyle: React.CSSProperties = {
  fontSize: 9,
  color: '#9ca3af',
  marginTop: 1,
}

const addKfBtnStyle: React.CSSProperties = {
  fontSize: 11,
  color: '#1a73e8',
  background: 'none',
  border: 'none',
  cursor: 'pointer',
  padding: '2px 4px',
  marginTop: 2,
}

const removeBtnStyle: React.CSSProperties = {
  fontSize: 14,
  color: '#9ca3af',
  background: 'none',
  border: 'none',
  cursor: 'pointer',
  padding: '0 2px',
  lineHeight: 1,
}

const addSeriesBtnStyle: React.CSSProperties = {
  fontSize: 12,
  color: '#1a73e8',
  background: 'none',
  border: '1px dashed #93c5fd',
  borderRadius: 4,
  cursor: 'pointer',
  padding: '4px 10px',
}

const selectStyle: React.CSSProperties = {
  padding: '3px 5px',
  fontSize: 12,
  border: '1px solid #d1d5db',
  borderRadius: 4,
}

function seriesLabel(kfCount: number) {
  return kfCount === 1 ? 'Static' : 'Temporal'
}

function UniformKfRow({
  kf,
  kfIdx,
  seriesIdx,
  onUpdate,
  onRemove,
  disableRemove,
  sectionPrefix,
}: {
  kf: EnvironmentConditionDTO
  kfIdx: number
  seriesIdx: number
  onUpdate: (patch: Partial<EnvironmentConditionDTO>) => void
  onRemove: () => void
  disableRemove: boolean
  sectionPrefix: string
}) {
  return (
    <div style={kfRowStyle}>
      <div style={fieldGroupStyle}>
        <span style={labelStyle}>Time (s)</span>
        <input
          type="number"
          min={0}
          step={1}
          value={kf.t}
          style={inputStyle}
          aria-label={`${sectionPrefix} series ${seriesIdx} keyframe ${kfIdx} time`}
          onChange={(e) => onUpdate({ t: Number(e.target.value) })}
        />
      </div>
      <div style={fieldGroupStyle}>
        <span style={labelStyle}>Direction (°)</span>
        <input
          type="number"
          min={0}
          max={360}
          step={1}
          value={kf.dirNaut}
          style={inputStyle}
          aria-label={`${sectionPrefix} series ${seriesIdx} keyframe ${kfIdx} direction`}
          onChange={(e) => {
            const v = Number(e.target.value)
            if (!isNaN(v)) onUpdate({ dirNaut: v })
          }}
        />
        <span style={helperStyle}>0 = North, clockwise, from where</span>
      </div>
      <div style={fieldGroupStyle}>
        <span style={labelStyle}>Speed (m/s)</span>
        <input
          type="number"
          min={0}
          step={0.1}
          value={kf.speed}
          style={inputStyle}
          aria-label={`${sectionPrefix} series ${seriesIdx} keyframe ${kfIdx} speed`}
          onChange={(e) => onUpdate({ speed: Number(e.target.value) })}
        />
      </div>
      <button
        type="button"
        style={removeBtnStyle}
        disabled={disableRemove}
        aria-label={`Remove ${sectionPrefix} keyframe ${kfIdx} from series ${seriesIdx}`}
        onClick={onRemove}
      >
        ×
      </button>
    </div>
  )
}

function UniformSeriesCard({
  series,
  seriesIdx,
  sectionPrefix,
  onRemoveSeries,
  onAddKeyframe,
  onRemoveKeyframe,
  onUpdateKeyframe,
}: {
  series: EnvironmentConditionDTO[]
  seriesIdx: number
  sectionPrefix: string
  onRemoveSeries: () => void
  onAddKeyframe: () => void
  onRemoveKeyframe: (kfIdx: number) => void
  onUpdateKeyframe: (kfIdx: number, patch: Partial<EnvironmentConditionDTO>) => void
}) {
  const label = seriesLabel(series.length)
  const sorted = [...series].sort((a, b) => a.t - b.t)

  return (
    <div style={cardStyle}>
      <div style={cardHeaderStyle}>
        <span style={labelBadgeStyle}>
          Series {seriesIdx + 1} — {label}
        </span>
        <button
          type="button"
          style={removeBtnStyle}
          aria-label={`Remove ${sectionPrefix} series ${seriesIdx}`}
          onClick={onRemoveSeries}
        >
          ×
        </button>
      </div>

      {series.length > 1 && (
        <p style={temporalNoteStyle}>Temporal interpolation active — values interpolated between keyframes</p>
      )}

      {sorted.map((kf, ki) => {
        const originalIdx = series.indexOf(kf)
        return (
          <UniformKfRow
            key={ki}
            kf={kf}
            kfIdx={originalIdx}
            seriesIdx={seriesIdx}
            sectionPrefix={sectionPrefix}
            onUpdate={(patch) => onUpdateKeyframe(originalIdx, patch)}
            onRemove={() => onRemoveKeyframe(originalIdx)}
            disableRemove={series.length <= 1}
          />
        )
      })}

      <button
        type="button"
        style={addKfBtnStyle}
        aria-label={`Add ${sectionPrefix} keyframe to series ${seriesIdx}`}
        onClick={onAddKeyframe}
      >
        + Add keyframe
      </button>
    </div>
  )
}

function UniformSection({
  title,
  sectionPrefix,
  series,
  onAddSeries,
  onRemoveSeries,
  onAddKeyframe,
  onRemoveKeyframe,
  onUpdateKeyframe,
}: {
  title: string
  sectionPrefix: string
  series: EnvironmentConditionDTO[][]
  onAddSeries: () => void
  onRemoveSeries: (idx: number) => void
  onAddKeyframe: (seriesIdx: number) => void
  onRemoveKeyframe: (seriesIdx: number, kfIdx: number) => void
  onUpdateKeyframe: (seriesIdx: number, kfIdx: number, patch: Partial<EnvironmentConditionDTO>) => void
}) {
  return (
    <div style={sectionStyle}>
      <div style={sectionTitleStyle}>
        <span>{title}</span>
        <button
          type="button"
          style={addSeriesBtnStyle}
          aria-label={`Add ${sectionPrefix} series`}
          onClick={onAddSeries}
        >
          + Add {title} Series
        </button>
      </div>

      {series.length === 0 && (
        <p style={{ fontSize: 12, color: '#9ca3af' }}>No {title.toLowerCase()} conditions configured.</p>
      )}

      {series.map((s, si) => (
        <UniformSeriesCard
          key={si}
          series={s}
          seriesIdx={si}
          sectionPrefix={sectionPrefix}
          onRemoveSeries={() => onRemoveSeries(si)}
          onAddKeyframe={() => onAddKeyframe(si)}
          onRemoveKeyframe={(kfIdx) => onRemoveKeyframe(si, kfIdx)}
          onUpdateKeyframe={(kfIdx, patch) => onUpdateKeyframe(si, kfIdx, patch)}
        />
      ))}
    </div>
  )
}

function WaveKfRow({
  kf,
  kfIdx,
  onUpdate,
  onRemove,
  disableRemove,
}: {
  kf: WaveConditionDTO
  kfIdx: number
  onUpdate: (patch: Partial<WaveConditionDTO>) => void
  onRemove: () => void
  disableRemove: boolean
}) {
  return (
    <div style={{ ...cardStyle, marginBottom: 6 }}>
      <div style={{ ...kfRowStyle, flexWrap: 'wrap' }}>
        <div style={fieldGroupStyle}>
          <span style={labelStyle}>Time (s)</span>
          <input
            type="number"
            min={0}
            step={1}
            value={kf.t}
            style={inputStyle}
            aria-label={`Wave keyframe ${kfIdx} time`}
            onChange={(e) => onUpdate({ t: Number(e.target.value) })}
          />
        </div>
        <div style={fieldGroupStyle}>
          <span style={labelStyle}>Direction (°)</span>
          <input
            type="number"
            min={0}
            max={360}
            step={1}
            value={kf.dirNaut}
            style={inputStyle}
            aria-label={`Wave keyframe ${kfIdx} direction`}
            onChange={(e) => {
              const v = Number(e.target.value)
              if (!isNaN(v)) onUpdate({ dirNaut: v })
            }}
          />
          <span style={helperStyle}>0 = North, clockwise, from where</span>
        </div>
        <div style={fieldGroupStyle}>
          <span style={labelStyle}>Hs (m)</span>
          <input
            type="number"
            min={0}
            step={0.1}
            value={kf.Hs}
            style={inputStyle}
            aria-label={`Wave keyframe ${kfIdx} Hs`}
            onChange={(e) => onUpdate({ Hs: Number(e.target.value) })}
          />
        </div>
        <div style={fieldGroupStyle}>
          <span style={labelStyle}>Tp (s)</span>
          <input
            type="number"
            min={0}
            step={0.1}
            value={kf.Tp}
            style={inputStyle}
            aria-label={`Wave keyframe ${kfIdx} Tp`}
            onChange={(e) => onUpdate({ Tp: Number(e.target.value) })}
          />
        </div>
        <div style={fieldGroupStyle}>
          <span style={labelStyle}>Spectrum</span>
          <select
            value={kf.spectrum}
            style={selectStyle}
            aria-label={`Wave keyframe ${kfIdx} spectrum`}
            onChange={(e) => onUpdate({ spectrum: e.target.value as WaveConditionDTO['spectrum'] })}
          >
            <option value="JONSWAP">JONSWAP</option>
            <option value="PIERSON">PIERSON</option>
            <option value="REGULAR">REGULAR</option>
          </select>
        </div>
        {kf.spectrum === 'JONSWAP' && (
          <div style={fieldGroupStyle}>
            <span style={labelStyle}>Gamma</span>
            <input
              type="number"
              min={0}
              step={0.1}
              value={kf.gamma}
              style={inputStyle}
              aria-label={`Wave keyframe ${kfIdx} gamma`}
              onChange={(e) => onUpdate({ gamma: Number(e.target.value) })}
            />
          </div>
        )}
        <button
          type="button"
          style={{ ...removeBtnStyle, alignSelf: 'center' }}
          disabled={disableRemove}
          aria-label={`Remove wave keyframe ${kfIdx}`}
          onClick={onRemove}
        >
          ×
        </button>
      </div>
    </div>
  )
}

function WaveSection() {
  const { waveSeries, addWaveKeyframe, removeWaveKeyframe, setWaveKeyframe } = useEnvironmentStore()
  const label = waveSeries.length > 0 ? seriesLabel(waveSeries.length) : null
  const sorted = [...waveSeries].sort((a, b) => a.t - b.t)

  return (
    <div style={sectionStyle}>
      <div style={sectionTitleStyle}>
        <span>
          Wave{label ? ` — ${label}` : ''}
        </span>
        <button
          type="button"
          style={addSeriesBtnStyle}
          aria-label="Add wave keyframe"
          onClick={addWaveKeyframe}
        >
          + Add Wave Keyframe
        </button>
      </div>

      {waveSeries.length === 0 && (
        <p style={{ fontSize: 12, color: '#9ca3af' }}>No wave conditions configured.</p>
      )}

      {waveSeries.length > 1 && (
        <p style={temporalNoteStyle}>Temporal interpolation active — values interpolated between keyframes</p>
      )}

      {sorted.map((kf, ki) => {
        const originalIdx = waveSeries.indexOf(kf)
        return (
          <WaveKfRow
            key={ki}
            kf={kf}
            kfIdx={originalIdx}
            onUpdate={(patch) => setWaveKeyframe(originalIdx, patch)}
            onRemove={() => removeWaveKeyframe(originalIdx)}
            disableRemove={false}
          />
        )
      })}
    </div>
  )
}

export function EnvironmentConditionPanel() {
  const {
    currentSeries,
    windSeries,
    addCurrentSeries,
    removeCurrentSeries,
    setCurrentKeyframe,
    addCurrentKeyframe,
    removeCurrentKeyframe,
    addWindSeries,
    removeWindSeries,
    setWindKeyframe,
    addWindKeyframe,
    removeWindKeyframe,
  } = useEnvironmentStore()

  return (
    <div style={panelStyle} data-testid="environment-condition-panel">
      <UniformSection
        title="Current"
        sectionPrefix="current"
        series={currentSeries}
        onAddSeries={addCurrentSeries}
        onRemoveSeries={removeCurrentSeries}
        onAddKeyframe={addCurrentKeyframe}
        onRemoveKeyframe={removeCurrentKeyframe}
        onUpdateKeyframe={setCurrentKeyframe}
      />

      <UniformSection
        title="Wind"
        sectionPrefix="wind"
        series={windSeries}
        onAddSeries={addWindSeries}
        onRemoveSeries={removeWindSeries}
        onAddKeyframe={addWindKeyframe}
        onRemoveKeyframe={removeWindKeyframe}
        onUpdateKeyframe={setWindKeyframe}
      />

      <WaveSection />
    </div>
  )
}
