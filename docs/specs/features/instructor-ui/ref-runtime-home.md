I have all I need. Here is the map.

---

# Instructor App — Home (Meus cenários) + Running Simulation UI

Base path: `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src`

## (a) Home / "Meus cenários" — scenario list

### Layout & flow
- `app/Home/index.tsx` — root flex layout, full viewport (`100vh`). Left `Sidebar` + main content switched by `activeSidebarMenu` (only `'Scenario'` → `<MyScenarios />` is wired today). Holds `openSettings` boolean → renders `SettingsModal` overlay.
- `app/Home/MyScenarios/index.tsx` — the scenario grid. Scrollable `ContentContainer` with infinite scroll (`scrollHandler` loads `nextPage` at 80% scroll via `fetchScenariosPage`). Renders `TopBar`, a state component, the grid of `CardButton`s, a loading ring, an end-of-list message, and the `ReplayModal` (in an `Overlay`).
- `app/Home/NewScenario/index.tsx` — "Novo cenário" route (`/new-scenario`): `Header mode="NewScenario"`, full-screen `Map` with `AreaRender`, and `NewScenarioPanel` (area picker + form). Submitting creates a scenario then navigates to `/edit-scenario/:id`.

### Components
- **Sidebar** (`features/home/components/Sidebar`): brand `Header` with `ActionMenu` dropdown (open ticket, view manual, settings, exit — last 3 are `console.info` stubs), middle menu list (single "Meus cenários" map item, also a stub `onClick`), `Footer` with Settings button. Active state via `$isActive`.
- **TopBar** (`features/home/myScenarios/components/TopBar`): left title toggles "Meus cenários" ↔ "Resultados da pesquisa"; right side: debounced (500ms) search `Input` (search icon), grid/list view toggle buttons (`grid`/`list` — visual only, no handlers), "Carregar Replay" button (opens ReplayModal), "Novo cenário" accent button → navigates `/new-scenario`.
- **CardButton** (`features/home/myScenarios/components/CardButton`): scenario card. Has loading skeleton variant. Top `IconsRow` counts (ownships/targets/tugs/anchors/lines with ship/target/ship-side/anchor/line icons). "Ownships" section (first 2 names). "Condições ambientais" section: 2×2 grid of wind/current/waves/swell formatted as `dir° speedknts`, `Hm Ps dir°` (JetBrains Mono font). Footer: name + area name + UTC datetime (`dd/mm/yyyy hh:mm`). Hover/menu reveals `ActionsContainer`: "Abrir" button + dots `ActionMenu` (Duplicar/Renomear/Excluir). Card click → `setSelectedScenarioId` + navigate `/edit-scenario/:id?name=`.
- **ModalInput** — rename/duplicate dialog (pre-filled name, e.g. duplicate uses `"<name> (Copy)"`). Delete uses `ConfirmationDialog` (danger).
- **States** (`features/home/myScenarios/components/States/index.ts`): keyed map `{ loading, error, empty, emptySearch, default }`. `EmptyState` shows `no-scenarios` icon + header/description + "Novo cenário" button; `error` has `onRetry`; `loading` uses `RingAnimation`.

### State (MyScenarios)
`scenarios`, `scenariosFiltered`, `state` (StateKey), `nextPage`, `searchValue`, `replayModalVisible`; `canReqNextPage` ref guards pagination. Selected scenario id lives in `useScenarioStore` (zustand). Search > 0 chars switches the rendered list to filtered results (fetched with `limit: 999`).

## (b) Running simulation UI

Entry: `app/RuntimeSimulation/index.tsx` (route renders during simulation; `Header mode="SimulatingScenario"`).

### Layout
- `Container` → top `Header` (toolbar), `ContentArea` (map + panels), bottom-centered `SimulationContainer` (transport controls).
- `Header` dynamicProps expose the running toolbar: move tools (move/bodies/vessels/buoys/SAR), free/fixed camera toggle, measurement, and panel toggles — Information (`onOpenInformation`), Environment (`onOpenEnvironment`), Settings (`onOpenSettings`), Search & Rescue (`onOpenSar`), plus `currentTime` clock. Ship/Bollard/Line buttons are stubs (`console.info('Temp')`).
- `ContentArea`: `RuntimeMap` (MapLibre via `react-map-gl`) rendered once loaded; `AnimatePresence`/framer-motion fade between `LoadingScenario` and content. Right side shows `PanelContainer` plus a `ZoomContainer` with `CompassControl` + `ZoomControl`. Layout depends on `getPanelVariant()` — `'Sheet'` variant wraps both in a `RightRail`; otherwise zoom controls sit left of the panel.

### Panels, sheets, windows
- **Docked right sheets** (toggled open/closed via `usePanel`, position `'right'`):
  - `ScenarioInformation` sheet (`sheets/ScenarioInformation`) — "Informações gerais", info icon. `ScenarioInfoCard` + `SimulationInfoCard` (maneuver-save flags, evaluate participants, debriefing notes). Refreshes on `environment.general.update`.
  - `EnvironmentalConditions` sheet (`sheets/EnvironmentalConditions`) — "Condições ambientais", cloud icon. Tabs: "Geral", "Correnteza, vento e ondas", "Visibilidade".
  - `Settings` sheet (`sheets/Settings`).
- **Vessel control panels** (`features/simulationScenario/panels/*`, rendered inside `PanelContainer`):
  - `Ownship` — `Panel` titled "Ownship" + vessel name; tabs: Geral, AIS/COLREG, Lights/Shapes/Sounds, Morse Signals, Alarmes/Falhas, MCA, Lines, Anchor. Left actions: focus map, camera dropdown (Front/Bridge/Back/Starboard/Portside view), collision sensor window, "Controle do ownship" (opens control window). Right action menu: "Mover". Live `body.upd.position` updates.
  - `Tug` — `Panel` titled "Tug" + name; tabs: Geral, AIS/COLREG, Lights/Shapes/Sounds, Morse Signals. Left actions: focus map + "Controle do Rebocador". Right menu: Mover.
  - `Target`, `Sar` panels also present.
- **Detached control windows** (opened via `window.instructorAPI.openNewWindow`, own routes):
  - `windows/OwnshipControl` (`/ownship-control?bodyId=`, 850×620) — "Controle do Ownship". `RudderCard` (Leme) + two `AzimuthalCard`s (azimuthal-left/right) + two `TunnelCard`s + `EngineCard`.
  - `windows/TugControl` (`/tug-control?bodyId=`, 478×635) — tabs Control + Information.
  - `windows/CollisionAndBathymetry`.
  - Control cards live in `windows/cards/*`: `EngineCard` (EOT "Potência" telegraph buttons via `eotOptions`, vertical `Slider` 0–10, per-step % inputs, power checkbox, "RPM Máximo", `thruster-id` popover), `RudderCard`, `AzimuthalCard`, `TunnelCard`, `ForceCard`, `LinkModeCard`, `IDsCard`, `InformationCard`. All use react-hook-form + zod, push via `window.instructorAPI.upd*` and refresh on `body.vessel.upd.thruster.*` events.

### Transport controls
`features/editScenario/components/SimulationControl/index.tsx` (reused): bottom bar with `Timer` (elapsed `currentTime`, `HH:MM:SS`), Play/Pause toggled by `EditorState` (Playing→pause, Paused→play), and Stop (danger) → opens `EndSimulationHandler` (end-simulation/evaluation/student-evaluation modals).

### Telemetry / live data
RuntimeSimulation subscribes to: `simulation.upd.time` (clock), `body.upd.collision` and `body.upd.bathymetry.collision` → fetch collided vessel name, show error `Alert`s ("Colisão detectada!"). State: `cameraMode`, `isLoading`, `currentTime`, `points` (area bounds), `collision`, `bathymetryCollision`, `vesselCollided`. Tool state via `useEditScenarioStore` (`currentTool`, `currentMovingBodyId`, `newBody`, `previewHeading`, `currentMapType`). Selection modals gated on `currentTool`: `VesselSelection` (`selectVessel`), `BollardSelection` (`selectBollard`), `SarSelection` (`selectSar`).

## Key Portuguese strings (`config/intl/translations-pt-BR.json`)
- my-scenarios "Meus cenários" · new-scenario "Novo cenário" · search-for-scenario "Pesquisar cenário" · search-results "Resultados da pesquisa" · load-replay "Carregar Replay" · reach-end "Você chegou ao fim" · ownships "Ownships" · environmental-conditions "Condições ambientais"
- general "Geral" · general-information "Informações gerais" · current-wind-waves "Correnteza, vento e ondas" · visibility "Visibilidade"
- ownship-control "Controle do Ownship" · control-ownship "Controle do ownship" · tug-control "Controle do Rebocador" · engine "Máquina" · power "Potência" · rudder "Leme" · thruster-id "ID do Thruster" · max-rpm "RPM Máximo"
- focus-map "Focar no mapa" · camera-control "Controle da Câmera" · front-view "Visão frontal" · bridge-view "Visão da ponte" · move "Mover" · collision-detected-title "Colisão detectada!"

## Notable stubs / non-wired UI
- Sidebar: open-ticket / view-manual / exit are `console.info` stubs; menu item onClick is a stub.
- Header (SimulatingScenario): Ship/Bollard/Line buttons log `'Temp'`.
- TopBar grid/list view toggles have no handlers.