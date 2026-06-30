import { Routes, Route } from 'react-router-dom'
import { MyScenariosPage } from './features/scenarios/MyScenariosPage'
import { AppShell } from './features/shell/AppShell'

export default function App() {
  return (
    <Routes>
      <Route path="/" element={<MyScenariosPage />} />
      <Route path="/editor" element={<AppShell />} />
    </Routes>
  )
}
