import { useState } from 'react'
import { ScenarioForm } from './ScenarioForm'
import { VesselList } from './VesselList'
import { SimulationControls } from './SimulationControls'
import { EnvironmentConditionPanel } from './EnvironmentConditionPanel'
import { tokens } from '../../../theme/tokens'

/**
 * Left scenario-setup panel (área, embarcações, status, condições ambientais).
 * Collapsible so the map can take the full width when the instructor only wants
 * to watch the simulation.
 */
export function Sidebar() {
  const [collapsed, setCollapsed] = useState(false)

  if (collapsed) {
    return (
      <button
        type="button"
        onClick={() => setCollapsed(false)}
        aria-label="Expandir painel"
        title="Expandir painel"
        style={{
          width: 28, minWidth: 28, height: '100%',
          border: 'none', borderRight: `1px solid ${tokens.color.border}`,
          background: tokens.color.surface, cursor: 'pointer',
          color: tokens.color.textSubtle, fontSize: tokens.fontSize.body,
        }}
      >
        »
      </button>
    )
  }

  return (
    <aside style={{
      width: tokens.size.sidebar,
      minWidth: tokens.size.sidebar,
      height: '100%',
      overflowY: 'auto',
      padding: tokens.space.lg,
      boxSizing: 'border-box',
      background: tokens.color.surface,
      borderRight: `1px solid ${tokens.color.border}`,
      display: 'flex',
      flexDirection: 'column',
      gap: tokens.space.xl,
    }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <strong style={{ fontSize: tokens.fontSize.title }}>Cenário</strong>
        <button
          type="button"
          onClick={() => setCollapsed(true)}
          aria-label="Recolher painel"
          title="Recolher painel"
          style={{ background: 'none', border: 'none', cursor: 'pointer', color: tokens.color.textSubtle, fontSize: tokens.fontSize.body }}
        >
          «
        </button>
      </div>
      <ScenarioForm />
      <VesselList />
      <SimulationControls />
      <EnvironmentConditionPanel />
    </aside>
  )
}
