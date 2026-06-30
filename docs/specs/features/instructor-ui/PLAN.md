I have everything I need. Producing the plan now.

---

# Ymir Web — Instructor UI Implementation Plan

Target: `apps/web` (Vite + React 19 + Zustand + Leaflet). Current state: a single-page `ScenarioCreatorPage` (left `Sidebar` with `ScenarioForm`/`VesselList`/`SimulationControls`/`EnvironmentConditionPanel`, full-bleed `AreaMapView`, floating `VesselPanel`). All styling is inline `React.CSSProperties` with hard-coded hex/px. Real WASM physics + mock fallback wired through `simulationStore` + `simulation.worker.ts`. This plan restyles to the Figma design system and stages new screens, deferring everything that needs backend/runtime not yet built.

UI copy ships in **Portuguese** (per user memory: docs/UI PT, code/commits EN). Figma mixes PT/EN placeholders — use the PT strings.

---

## 1. Design tokens

New file: `apps/web/src/theme/tokens.ts` (plain TS object exported as `tokens`, plus a CSS-variable injector `applyThemeVars()` and a `:root` stylesheet in `apps/web/src/theme/global.css`). No styled-components dependency exists; use **CSS variables + a typed token object** consumed by inline styles and a small `cx`/`css` helper. This keeps the migration incremental (components can read `tokens.color.accent` directly while we move off inline literals).

```ts
// tokens.ts — values reconciled across all 5 Figma screens
export const tokens = {
  color: {
    // accent / primary
    accent:        '#1a8ff6', // surface/accent/hc-primary — add button, active fills
    accentText:    '#147cec', // content/accent/primary — active tab text, links
    accentBorder:  '#bbeaff', // accent border, active toolbar fill
    // content (text)
    textPrimary:   '#101013',
    textSecondary: '#2f3033',
    textTertiary:  '#4a4e5a',
    textSubtle:    '#6b7280', // labels, breadcrumb, placeholders, disabled
    textHcSubtle:  '#9ca3af', // hint/min-max
    // surfaces
    surface:       '#ffffff',
    surfaceAlt:    '#f9fafb', // group boxes, inputs, cards
    surfaceActive: '#e5e7eb', // active sidebar pill
    surfaceDark:   '#25272d', // dark primary buttons ("Salvar cenário","Build")
    textOnDark:    '#ffffff',
    // borders
    border:        '#e5e7eb',
    borderActive:  '#9ca3af', // input focus
    borderDark:    '#383a42',
    // feedback
    danger:        '#c81e1e',
    dangerBorder:  '#fbd5d5',
    success:       '#10b981',
    warningBg:     '#fef3c7',
    warningFg:     '#92400e',
    // scrim
    scrim:         'rgba(16,16,19,0.5)', // #10101380
  },
  font: {
    sans: "'Inter', system-ui, sans-serif",
    mono: "'JetBrains Mono', ui-monospace, monospace", // numeric values
  },
  fontSize: { // px
    xs: 10, sm: 12, label: 13, body: 14, title: 16, panelTitle: 18,
  },
  fontWeight: { regular: 400, medium: 500, semibold: 600, bold: 700 },
  space: { 0.5: 2, 1: 4, 2: 8, 3: 12, 4: 16, 5: 20, 6: 24 }, // 4px base
  radius: { sm: 4, md: 6, button: 8, panel: 12, pill: 999 },
  shadow: {
    sm: '0 1px 2px rgba(0,0,0,0.05)', // shadow/down/small — topbar, pills, cards
    panel: '0 4px 24px rgba(0,0,0,0.10)', // floating right panel
  },
  size: { topbar: 56, iconButton: 40, inputHeight: 40, rightPanel: 484, vesselPanel: 398, sidebar: 280 },
  z: { map: 0, overlay: 1000, panel: 1100, modal: 2000 },
} as const
```

Fonts: add `@fontsource/inter` (400/500/600/700) + `@fontsource/jetbrains-mono` to `package.json` and import in `theme/global.css`. (Matches the reference design system, which uses Inter UI + JetBrains Mono for numerics.)

---

## 2. Component primitives to build

New dir: `apps/web/src/ui/`. All primitives are theme-driven, headless-ish, no external UI lib. Each ships with a colocated `.test.tsx`.

| Primitive | File | Responsibility / props |
|---|---|---|
| `Button` | `ui/Button.tsx` | `variant: primary\|secondary\|dark\|ghost\|danger`, `size`, `icon`, `iconPosition`, `fullWidth`, `loading`, `disabled`. Maps to Figma "Salvar cenário" (dark), "+ Novo cenário" (accent), "Sair do cenário" (secondary). |
| `IconButton` | `ui/IconButton.tsx` | 40×40 square; `variant: ghost\|accent\|active`; `active` (light-blue `accentBorder` fill). Topbar `+`, info/cloud/gear. |
| `TopBar` | `ui/TopBar.tsx` | 56px, `surface`, bottom border, `shadow.sm`. Slots: `left` (logo + breadcrumb), `center` (action cluster), `right` (actions). `actionsDisabled: none\|center\|centerAndRight`. |
| `Panel` | `ui/Panel.tsx` | Floating card shell: `title`, `eyebrow`, `leftActions`, `rightActions`, `onClose`, `width`, `position: right`. Radius `panel`, `shadow.panel`. Drives VesselPanel + right drawers. |
| `Tabs` | `ui/Tabs.tsx` | `tabs: {id,label,content}[]`, `activeId`, `onChange`. Active = `accentText` + 2px underline. Replaces VesselPanel's inline tab logic. |
| `Card` | `ui/Card.tsx` | Compound `Card.Root/Header/Body`; `tone: default\|alt`; bordered group box. For "Geral", "Dimensões", env accordions. |
| `Field` / `LabeledRow` | `ui/Field.tsx` | label-left / control-right row; `label`, `unit`, `mono`, `disabled`. |
| `TextInput` | `ui/TextInput.tsx` | `unit` suffix, `error`, `leadingIcon`, focus border `borderActive`. |
| `Select` | `ui/Select.tsx` | styled native `<select>` with chevron. |
| `Slider` | `ui/Slider.tsx` | blue track + thumb; numeric mirror input. Extract the inline `SliderRow` from `VesselPanel.tsx`. |
| `Checkbox` | `ui/Checkbox.tsx` | 16px blue check. |
| `RadioRow` | `ui/RadioRow.tsx` | segmented `Uniform/Map/Plan`. |
| `Accordion` | `ui/Accordion.tsx` | collapsible card (chevron right→down) for Current/Wind/Waves/Swell. |
| `Modal` / `AlertDialog` | `ui/Modal.tsx` | centered card + scrim (`color.scrim`); `AlertDialog` adds "?" badge, title, body, cancel/confirm. |
| `FormStepper` | `ui/FormStepper.tsx` | generic step engine `steps[]`, `onFinish`, `isNextEnabled`. For vessel type→model selection (mirrors reference `FormStepper`). |
| `SimulationControl` | `ui/SimulationControl.tsx` | bottom-center pill: dark "Build simulation" + Play + Stop cells. |
| `MapActions` | `ui/MapActions.tsx` | bottom-right zoom −/+ pill (wraps Leaflet `map.zoomIn/Out`). |
| `Spinner` / `ThreeDots` | `ui/Loading.tsx` | "Loading scenario…" + disabled topbar state. |

---

## 3. Screen-by-screen plan

### Screen 1 — Layout do instrutor (app shell) → **Phase 1**
- **New**: `ui/TopBar.tsx`, `features/shell/AppShell.tsx`, `ui/SimulationControl.tsx`, `ui/MapActions.tsx`, `ui/AlertDialog`.
- **Reuse/rework**: `AreaMapView.tsx` becomes the map surface (drop the left `Sidebar` as the primary chrome; map goes full-bleed). `SimulationControls.tsx` logic (save→`POST /scenarios`→`loadScenario`→`start`) moves behind the new `SimulationControl` pill's Build/Play/Stop.
- **Flow**: TopBar (logo + breadcrumb "Pasta pai › Nome do cenário", center `+`/info/cloud/gear, right "Sair"/"Salvar cenário") over full-bleed map; bottom-center Build/Play/Stop; bottom-right zoom. "Salvar" with no ownship → `AlertDialog` ("Seu cenário não possui um ownship…", Cancelar / Salvar mesmo assim).
- **Defer**: real nautical basemap tiles (keep OSM), live Build/Play/Stop wiring beyond what exists, cloud sync, unsaved-changes tooltip, editable map markers/trajectory.

### Screen 4 — Condições ambientais → **Phase 1** (restyle of existing)
- **Exists**: `EnvironmentConditionPanel.tsx` (Current/Wind keyframe series, Wave keyframes) — keep the keyframe/temporal-series store model (`environmentStore`), it is richer than Figma.
- **New**: `features/environment/EnvironmentPanel.tsx` wrapping in `Panel` (484px, right-docked) with `Tabs`: **Geral** (date/time, water color, sea bed, clouds, meteorological-instruments card), **Correnteza, vento e ondas** (Beaufort select + `Accordion` cards Current/Wind/Waves/Swell with `RadioRow` Uniform/Map/Plan), **Visibilidade** (Rain/Fog `Slider`s). Wrap current/wind/wave editors in the accordions.
- **Defer**: Map/Plan current types, `CurrentPlanModal`, vector-arrow overlay, advanced-mode spatial fields, Beaufort coupling logic.

### Screen 5 — Embarcações (vessel editor) → **Phase 1** (Informações) + Phase 2 (tabs)
- **Exists**: `VesselPanel.tsx` (Geral + Controles tabs). Rework into `Panel` + `Tabs`.
- **Phase 1**: rename/expand "Geral" into **Informações** tab — `Card` groups: Geral (Tipo/Nome/MMSI/ID), Dimensões (LOA/Beam/Draft), Posição e direções (Lat/Long/Heading/Speed). Keep **Controles** tab (rudder/thruster sliders + azimuth dial → `Slider`/`AzimuthDial` primitives). Eyebrow "Embarcação Ownship" + focus/`•••` actions.
- **Phase 2+**: `AIS/RIPEAM` tab (needs AIS fields + RIPEAM mapping). **Defer**: `Âncoras`, `Linhas` tabs, anchor editor panel.

### Screen 3 — Informações do cenário/simulação → **Phase 2**
- **New**: `features/scenario-info/ScenarioInfoPanel.tsx` (right `Panel`, info toolbar toggle). Sections: Scenario information (name/datetime/description), Scenario area (area select + bathymetry), Simulation information (debriefing checkboxes + notes).
- **Defer**: bathymetry combobox + validation alerts, debriefing/recording pipeline, GMDSS modal, clock wiring beyond display.

### Screen 2 — Meus cenários (scenario list) → **Phase 2** (needs scenario CRUD backend)
- **New**: `features/scenarios/MyScenariosPage.tsx`, `ScenarioCard.tsx`, `ScenarioListRow.tsx`, sidebar, search, grid/list toggle. Requires `GET/POST/PUT/DELETE /scenarios` + routing (`react-router-dom` not yet a dep — add in Phase 2).
- **Defer (explicit)**: Load replay, Load backup/autosaved draft, folders, rename/duplicate/delete (scenario CRUD/management) per user instruction.

---

## 4. Phasing

### Phase 1 — App shell + restyle existing flow (everything implementable today)
1. `theme/tokens.ts` + `theme/global.css` + fonts. (task #8)
2. UI primitives §2 needed by current screens: `Button`, `IconButton`, `TopBar`, `Panel`, `Tabs`, `Card`, `Field`, `TextInput`, `Select`, `Slider`, `Checkbox`, `Accordion`, `Modal`/`AlertDialog`, `SimulationControl`, `MapActions`, `Loading`. (task #9)
3. `features/shell/AppShell.tsx` → replaces `ScenarioCreatorPage` layout: TopBar + full-bleed `AreaMapView` + floating `SimulationControl`/`MapActions` + right-docked panels. `App.tsx` renders `AppShell`. (task #9)
4. Restyle in place (task #10): `VesselPanel.tsx` → `Panel`+`Tabs`+`Card` (Informações/Controles); `EnvironmentConditionPanel.tsx` → `EnvironmentPanel` with 3 tabs; `ScenarioForm`/`VesselList`/`SimulationControls` logic folded into TopBar (name/area/save) + Build pill + a vessel-add `FormStepper`. Keep all store wiring (`scenarioStore`, `simulationStore`, `environmentStore`, `vesselPanelStore`) intact.
5. "Salvar"-without-ownship `AlertDialog`.

### Phase 2 — Screens needing new backend/features
- `react-router-dom` + routes (`/`, `/scenario/:id`); `MyScenariosPage` (list view) once scenario CRUD API exists.
- `ScenarioInfoPanel` (Screen 3).
- Vessel `AIS/RIPEAM` tab.

### DEFERRED (leave unimplemented per instruction)
- **Anchors** (Âncoras tab, anchor editor, holding powers, winch/chain/quarteladas).
- **Towlines / mooring lines** (Linhas tab, Corpo A/B).
- **Replay** (Load replay button + replay pipeline).
- **Scenario CRUD/management** (rename, duplicate, delete, folders, autosaved-draft/backup) — beyond create.
- **Debriefing** (recording checkboxes, notes/backup observations, evaluate participants, GMDSS).
- **Map "Map"/"Plan" current/wind/wave spatial editors**, `CurrentPlanModal`, vector-arrow overlay.
- **Real nautical/marine basemap tiles** (keep OSM placeholder).
- **Cloud sync / unsaved-changes tooltip**, **MMSI conflict validation**, **bathymetry validation alerts**.

---

## 5. Dynamics (physics only)

Gap vs MATLAB `dynamics` repo (the source of truth per user memory). Faithful today: inertial, damping, wind, hydrostatic restoring. **Highest-impact fixes, in priority order:**

1. **D3 — Thruster/propeller (major).** `core/libs/physics/src/forces/ThrustForces.cpp:55` uses `Kt = 0.4·P/D − 0.1` clamped — no advance ratio J, no Kq/torque/power, no RPM lag, no transverse-speed reduction, no astern efficiency, no paddle force. Fix: implement Kt/Kq table interpolated on `J = Va/(pitch·n·D)` per `thruster.m:335-354`, add power-saturation loop, first-order RPM lag, `reduc` factor, astern efficiency, paddle/side-wash (`thruster.m:243-297`).
2. **D4 — Rudder (major).** `RudderForces.cpp:54-57` uses analytic thin-airfoil `CL/CD` instead of the Cl/Cd table vs incidence β, and a simplified actuator-disk wake without the `p1`/`hullEfficiency` slipstream factors (`rudder.m:54,119-144`). Fix: load Cl/Cd table, compute β from actual inflow incl. current, add `p1`/`p2` slipstream.
3. **D2 — 2nd-order wave forces (significant).** `world/src/wave/WaveForces.cpp` computes only 1st-order RAO excitation on all 6 DOF with no DOF mask. Fix: apply MATLAB excitation mask `[0 0 1 1 1 0]` (`VesselFastTime.m:531`); add **mean drift**, **slow drift**, **wave-drift damping** (`VesselFastTime.m:778-805`). Dominant for maneuvering/station-keeping.
4. **D1 — Obokata current (partial).** `CurrentForces.cpp:44-89` assumes uniform current across sections, omits `cdz` cross-flow yaw and roll/pitch arms (f[3],f[4]). Fix: sample current per section at its global position, add cross-flow yaw + `frontalHeight`/`lateralHeight` arms (`VesselFastTime.m:616-658`).
5. **D5 — Squat Cs table (minor).** `SquatForces.cpp:15-18` drops the `Cb>1→Cb` passthrough and reorders thresholds vs `VesselFastTime.m:864-872`. Fix: restore ordering `Cb>1→Cb; <0.7→1.7; >=0.8→2.4; else 2.0`.

Confirmed acceptable / out of scope here: integrator difference (Ymir DOPRI5 vs MATLAB RK4 — Ymir's body→inertial transform in `rhs` is arguably more correct), mass handling matches. **Not in scope (DEFER, gated on UI features):** suction (ship-to-ship/bank), mooring lines (`Cabo.m`), fenders, anchor (`Anchor.m`), radiation memory-function (D6/D7).

---

## 6. Test plan

Existing: vitest unit/component tests colocated (`*.test.tsx`) with `@testing-library/react`, `jsdom`, coverage v8; Playwright e2e in `e2e/` (`scenario-creator`, `simulation-run`, `vessel-panel`, `smoke`, `regression`). Setup: `src/test/setup.ts` (jest-dom).

**Tokens / theme**
- `theme/tokens.test.ts` — snapshot of `tokens`; assert no two semantic colors collide unintentionally; `applyThemeVars()` writes expected CSS vars.

**Primitives (each `ui/*.test.tsx`, RTL + user-event)**
- `Button`: renders variant classes, fires `onClick`, respects `disabled`/`loading`.
- `IconButton`: `active` applies accent fill; aria-label present.
- `TopBar`: `actionsDisabled='center'|'centerAndRight'` disables correct slots; left/center/right slots render.
- `Panel`: `onClose` fires on ×; renders eyebrow/title/actions.
- `Tabs`: clicking a tab switches `activeId`, active gets underline, content swaps.
- `Slider`: drag/keyboard updates value, mirror input stays in sync, clamps min/max.
- `Modal`/`AlertDialog`: scrim click + Cancel/Confirm callbacks; focus trap; Esc closes.
- `FormStepper`: `isNextEnabled` gating, back/next, `onFinish(ctx)`.
- `Select`/`Checkbox`/`Accordion`: controlled value changes; accordion expand/collapse toggles chevron + body.

**Restyled screens (keep/extend existing component tests)**
- `VesselPanel.test.tsx` (exists): update selectors to `Panel`/`Tabs`; assert Informações cards (Geral/Dimensões/Posição), Controles sliders send `worker.postMessage({type:'setActuator',...})` with correct `vesselId` (instanceId) — preserve the instance-id regression case.
- `EnvironmentConditionPanel.test.tsx` (exists): assert tab structure (Geral / Correnteza-vento-ondas / Visibilidade); keep keyframe add/remove/temporal-interpolation assertions against `environmentStore`; Rain/Fog sliders mirror numeric input.
- New `features/shell/AppShell.test.tsx`: TopBar Save with no ownship opens `AlertDialog`; "Salvar mesmo assim" calls save path; Build/Play/Stop pill drives `simulationStore.start/stop`; `engine==='mock'` warning still surfaces.
- Keep `ScenarioForm.test.tsx`, `VesselList.test.tsx`, `SimulationControls.test.tsx` green by re-pointing them at the relocated logic.

**E2e (Playwright, extend `e2e/`)**
- `scenario-creator.spec.ts`: update for new shell — select area via TopBar/form, add vessel via FormStepper, drag marker, open VesselPanel.
- `simulation-run.spec.ts`: Build→Play→running telemetry→Stop through the new pill.
- New `app-shell.spec.ts`: topbar disabled states (loading), no-ownship alert dialog flow, zoom −/+ via `MapActions`.
- `regression.spec.ts` / `smoke.spec.ts`: keep as guardrails; assert restyle didn't break sim end-to-end and real-vs-mock engine badge.

**Dynamics (C++ core, separate suite — `core/`)**
- Add/extend gtest cases per fix: D3 Kt/Kq at known J vs MATLAB reference values; D4 rudder Cl/Cd table lookup + slipstream; D2 excitation DOF mask + mean/slow drift presence; D1 per-section current + yaw arm; D5 Cs table branches. Each compares against MATLAB `dynamics` reference outputs (task #11) — physics changes are not driven by the JS test suite.

---

## Key file map (target app, absolute paths)

- App entry: `apps/web/src/App.tsx`, `apps/web/src/main.tsx`
- Current shell: `apps/web/src/features/scenario-creator/ScenarioCreatorPage.tsx`, `components/Sidebar.tsx`, `components/AreaMapView.tsx`
- Restyle targets: `apps/web/src/features/scenario-creator/components/{VesselPanel,EnvironmentConditionPanel,ScenarioForm,VesselList,SimulationControls}.tsx`
- Stores (reused as-is): `apps/web/src/features/scenario-creator/store.ts`, `apps/web/src/stores/{simulationStore,vesselPanelStore,environmentStore}.ts`
- Worker/physics bridge: `apps/web/src/workers/simulation.worker.ts`, `apps/web/src/workers/mock-wasm.ts`
- New: `apps/web/src/theme/{tokens.ts,global.css}`, `apps/web/src/ui/*`, `apps/web/src/features/shell/AppShell.tsx`, `apps/web/src/features/environment/EnvironmentPanel.tsx`
- Dynamics fixes: `core/libs/physics/src/forces/{ThrustForces,RudderForces,CurrentForces,SquatForces}.cpp`, `core/libs/world/src/wave/WaveForces.cpp`
- Tests: colocated `*.test.tsx` per primitive/screen; e2e in `apps/web/e2e/*.spec.ts`; configs `apps/web/{vitest.config.ts,playwright.config.ts}`