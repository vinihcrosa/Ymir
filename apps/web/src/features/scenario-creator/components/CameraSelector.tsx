import { useViewStore, type CameraId } from '../../../stores/viewStore'
import { tokens } from '../../../theme/tokens'

const CAMERAS: { id: CameraId; label: string }[] = [
  { id: 'Front', label: 'Proa' },
  { id: 'Bridge', label: 'Ponte' },
  { id: 'Back', label: 'Popa' },
  { id: 'Starboard', label: 'Boreste' },
  { id: 'Portside', label: 'Bombordo' },
  { id: 'Free', label: 'Livre' },
]

/** Onboard-camera picker shown over the 3D view (Proa/Ponte/Popa/Boreste/Bombordo). */
export function CameraSelector() {
  const { cameraId, setCamera } = useViewStore()

  return (
    <div
      role="group"
      aria-label="Câmera do navio"
      style={{
        position: 'absolute',
        top: tokens.size.topbar - 8,
        left: tokens.space.lg,
        display: 'inline-flex',
        gap: 2,
        padding: 2,
        background: tokens.color.surface,
        border: `1px solid ${tokens.color.border}`,
        borderRadius: tokens.radius.pill,
        boxShadow: tokens.shadow.sm,
        zIndex: tokens.z.panel,
      }}
    >
      {CAMERAS.map((c) => (
        <button
          key={c.id}
          type="button"
          aria-pressed={cameraId === c.id}
          onClick={() => setCamera(c.id)}
          style={{
            padding: `${tokens.space.xs}px ${tokens.space.sm}px`,
            border: 'none',
            background: cameraId === c.id ? tokens.color.accent : 'transparent',
            color: cameraId === c.id ? tokens.color.textOnDark : tokens.color.textSecondary,
            fontFamily: tokens.font.sans,
            fontSize: tokens.fontSize.sm,
            fontWeight: tokens.fontWeight.medium,
            cursor: 'pointer',
            borderRadius: tokens.radius.pill,
          }}
        >
          {c.label}
        </button>
      ))}
    </div>
  )
}
