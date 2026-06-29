import { ScenarioForm } from './ScenarioForm'
import { VesselList } from './VesselList'
import { SimulationControls } from './SimulationControls'
import { EnvironmentConditionPanel } from './EnvironmentConditionPanel'

export function Sidebar() {
  return (
    <aside style={{
      width: '320px',
      minWidth: '320px',
      height: '100vh',
      overflowY: 'auto',
      padding: '1rem',
      boxSizing: 'border-box',
      background: '#fff',
      borderRight: '1px solid #e5e7eb',
      display: 'flex',
      flexDirection: 'column',
      gap: '1.5rem',
    }}>
      <h2 style={{ margin: 0, fontSize: '1.125rem', fontWeight: 700 }}>Criar Cenário</h2>
      <ScenarioForm />
      <VesselList />
      <SimulationControls />
      <EnvironmentConditionPanel />
    </aside>
  )
}
