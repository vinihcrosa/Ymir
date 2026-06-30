import { useScenarioStore } from '../store'
import { useVessels } from '../hooks/use-vessels'
import { tokens } from '../../../theme/tokens'

export interface VesselPickerProps {
  open: boolean
  onClose: () => void
}

/**
 * Floating picker (anchored under the top bar "+") for adding a vessel to the
 * scenario. Replaces the old inline sidebar picker so vessels are added in the
 * map-first flow. Requires an area to be selected first.
 */
export function VesselPicker({ open, onClose }: VesselPickerProps) {
  const { area, addVessel } = useScenarioStore()
  const { vessels, loading } = useVessels()
  if (!open) return null

  return (
    <div
      data-testid="vessel-picker"
      role="dialog"
      aria-label="Adicionar embarcação"
      style={{
        position: 'absolute',
        top: tokens.size.topbar + tokens.space.sm,
        left: '50%',
        transform: 'translateX(-50%)',
        width: 320,
        maxHeight: 420,
        overflowY: 'auto',
        background: tokens.color.surface,
        border: `1px solid ${tokens.color.border}`,
        borderRadius: tokens.radius.panel,
        boxShadow: tokens.shadow.panel,
        zIndex: tokens.z.panel,
        padding: tokens.space.sm,
      }}
    >
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', padding: `${tokens.space.xs}px ${tokens.space.sm}px ${tokens.space.sm}px` }}>
        <strong style={{ fontSize: tokens.fontSize.label }}>Adicionar embarcação</strong>
        <button type="button" onClick={onClose} aria-label="Fechar" style={{ background: 'none', border: 'none', cursor: 'pointer', color: tokens.color.textSubtle, fontSize: tokens.fontSize.title }}>×</button>
      </div>
      {!area && (
        <p style={{ padding: tokens.space.sm, fontSize: tokens.fontSize.label, color: tokens.color.warningFg, background: tokens.color.warningBg, borderRadius: tokens.radius.sm, margin: tokens.space.xs }}>
          Selecione uma área antes de adicionar embarcações.
        </p>
      )}
      {area && loading && <p style={{ padding: tokens.space.sm, color: tokens.color.textSubtle }}>Carregando...</p>}
      {area && !loading && vessels.map(v => (
        <button
          key={v.id}
          type="button"
          onClick={() => { addVessel({ vesselId: v.id, name: v.name }); onClose() }}
          style={{ display: 'block', width: '100%', textAlign: 'left', padding: `${tokens.space.sm}px ${tokens.space.md}px`, marginBottom: 2, background: 'transparent', border: 'none', borderRadius: tokens.radius.sm, cursor: 'pointer', fontSize: tokens.fontSize.label, color: tokens.color.textPrimary }}
        >
          {v.name}
        </button>
      ))}
    </div>
  )
}
