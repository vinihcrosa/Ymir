import { TelemetryPanel } from './components/TelemetryPanel.js'

export default function App() {
  return (
    <div>
      <h1 style={{ fontFamily: 'monospace', padding: '1rem 1rem 0' }}>
        Ymir Naval Simulation
      </h1>
      <TelemetryPanel />
    </div>
  )
}
