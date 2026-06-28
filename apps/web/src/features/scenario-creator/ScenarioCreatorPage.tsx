import { Sidebar } from './components/Sidebar'
import { AreaMapView } from './components/AreaMapView'
import { VesselPanel } from './components/VesselPanel'

export function ScenarioCreatorPage() {
  return (
    <div style={{ display: 'flex', height: '100vh', overflow: 'hidden' }}>
      <Sidebar />
      <div style={{ flex: 1, height: '100vh', position: 'relative' }}>
        <AreaMapView />
        <VesselPanel />
      </div>
    </div>
  )
}
