import { ScenarioForm } from './ScenarioForm'
import { VesselList } from './VesselList'
import { SimulationControls } from './SimulationControls'
import { EnvironmentConditionPanel } from './EnvironmentConditionPanel'
import { tokens } from '../../../theme/tokens'

export function Sidebar() {
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
      <ScenarioForm />
      <VesselList />
      <SimulationControls />
      <EnvironmentConditionPanel />
    </aside>
  )
}
