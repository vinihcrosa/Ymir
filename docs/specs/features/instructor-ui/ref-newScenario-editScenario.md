I have a complete picture. Here is the spec.

---

# Scenario Creation / Edit Flow — UI + State Spec

## 1. Routes (`app/Root/index.tsx`, react-router-dom `<Routes>`)

| Path | Element | Phase |
|------|---------|-------|
| `/new-scenario` | `NewScenario` (`app/Home/NewScenario`) | Create: name/date/area form |
| `/edit-scenario/:scenarioId` | `EditScenario` (`app/EditScenario`) | Edit: map + panels + bodies |
| `/runtime-simulation` | `RuntimeSimulation` | After build (out of scope) |

There is no multi-step wizard router. "Steps" exist only as local state inside two flows: the `FormStepper` modal (vessel selection) and the `BuildHandler` state machine.

## 2. End-to-end flow

```
Home → /new-scenario
  └─ NewScenarioPanel (RHF + zod): name, dateTime, area.template, description
     submit → instructorAPI.createScenario → navigate(/edit-scenario/:id)

/edit-scenario/:id (EditScenario)
  ├─ loadScenario() → instructorAPI.loadSimulation (guarded by EditorState.Idle)
  ├─ Map (RuntimeMap) — placement surface
  ├─ Header actions toggle right-docked Sheets:
  │     ScenarioInformation | EnvironmentalConditions | Settings
  ├─ Header actions set currentTool → opens body flows:
  │     selectVessel → VesselSelectionModal (FormStepper: type → vessel)
  │        onFinish → setNewBody + setCurrentTool('addBodyToMap')
  │        → useAddBodyHandler places body on map click
  │     selectBollard → BollardSelectionModal
  │     addLine / measure / drawRoute / addSAR → map-edit tools
  ├─ Clicking a placed body opens its Sheet (Ownship/Target/Tug/Bollard)
  └─ BuildControl → BuildHandler (Step state machine) → build → runtime-simulation
```

The "wizard" is implicit: create form → land in editor → add bodies via map + tool state → configure conditions via docked sheets → build.

## 3. Step orders (local, not routed)

**VesselSelection (`FormStepper`)**: `type` (pick body type) → `vessel` (pick model). `isNextEnabled` gates step 2 on a selected vessel; `onBack` resets the filter store. `onFinish` writes to the zustand edit store.

**BuildHandler (`Step` enum state machine)**: `SaveToContinue` → (`ScenarioNoOwnship`/`NoOwnship` guards) → `Simulation` → conditionally `TextBackup` → `Settings` → `EvaluationSettings` → `Complete`. Next-step is computed by `getNextStep` from boolean flags (`saveManeuverDataOnText`, `saveManeuverDataForDebriefing`, `evaluateParticipants`); back nav uses a `historyRef` stack.

## 4. Form state strategy

Two distinct mechanisms coexist:

- **react-hook-form + zodResolver** — per-form, used in the create panel and in every editable body/condition card. Each card owns its own `useForm`, hydrates via `reset(apiToForm(...))` after fetch, and submits independently (`handleSubmit → instructorAPI...`). Pattern: `xApiToForm` / `xFormToApi` converters + a `schemas.ts` per card. Many use `mode: 'onChange'` autosave; the create panel uses `onSubmit`.
- **zustand stores** (`store/editScenario/`) — cross-component ephemeral editor state, not form values:
  - `useEditScenarioStore` — `currentTool`, `currentBodyType`, `currentTitle`, `newBody`, `previewHeading`, `selectedConnectionPoints`, measure state, `lineEdit`, `currentMapType`.
  - `useVesselFilterStore` / `useBollardFilterStore` — list filters.
  - `useEvaluationStore` (`hasEvaluation`), `useDirtyChanges` (`hasChanges`).
  - `panelStorage` — open/docked panel instances (drives `usePanel`).
  - `useScenarioStore` (in `features/editScenario/hooks`) — only `selectedId`; largely unused vs the store/ version.

Persistence is server-side: there is no global form object. Each card is the unit of truth and pushes to `instructorAPI` on submit.

## 5. Validation

zod schemas colocated as `schemas.ts`. Error messages are i18n keys (e.g. `validator.scenario-must-have-name`) resolved later by `t()`.

- Create panel (`NewScenarioPanel/schemas.ts`): `name` min 1, `dateTime` required + `min(DATE_MIN)`, `area.template` min 1, `description` optional.
- Card schemas (e.g. `PositionAndCourseCard`, `ShipDimensionsCard`, env cards) validate numeric/range fields per body field; units converted via `useUnit` (`convertToSI` / `convertToSelectedUnit`) around the form boundary.

## 6. Panel system

`usePanel` over `panelStorage` zustand. `openPanel`/`closePanel`/`getDockedPanel('right')`/`resetPanels`. `getPanelVariant()` returns `'Sheet'` for body panels (vessels, bollard, anchor, sar, RouterTool) and `'Modal'` otherwise — drives whether `EditScenario` renders the right rail. Only one right-docked panel at a time; opening one of the same id toggles it closed.

## 7. Component inventory (name → responsibility → key props)

**Entry / orchestration**
- `NewScenario` (`app/Home/NewScenario`) → create page; fetches areas, holds map; `onSubmit`→create+navigate. No external props.
- `NewScenarioPanel` → create form (RHF/zod). Props: `areas: ListArea[]`, `onChangeArea(points)`, `onSubmit(data)`.
- `EditScenario` (`app/EditScenario`) → editor shell; loads sim, wires Header actions, renders Map + PanelContainer + BuildControl + selection modals. Reads `:scenarioId`.

**Selection flows**
- `VesselSelectionModal` → 2-step FormStepper (type→vessel); on finish seeds store + `addBodyToMap`. Props: `open`, `onClose`.
- `SelectVesselType` → grid of body-type tiles. Props: `onSelect(type)`.
- `SelectVessel` → filterable vessel list. Props: `vesselType`, `selectedVessel`, `onVesselSelect(v)`.
- `BollardSelectionModal` → analogous bollard picker. Props: `open`, `onClose`.
- `FormStepper` (`components/FormStepper`) → generic step engine carrying a single `ctx` object. Props: `open`, `steps: FormStep<Item>[]`, `initialContext`, `onFinish(ctx)`, `onClose`. Each `FormStep`: `id`, `title`, `content`, `isNextEnabled`, `onBack`, `width/height`, `showFooter`.

**Docked right-rail sheets (editor)**
- `ScenarioInformation` → general info sheet; composes `GeneralCard` + `GMDSSCard`. Self-fetches; no props.
- `EnvironmentalConditions` → `Tabs` of `GeneralTab` + `CurrentWindWaveTab` (wind/waves/swell/current/tide/date cards). Self-fetches; no props.
- `Settings` → units/view settings (`SettingsCard` → `GeneralCard`/`ViewCard`/`UnitsCard`).

**Body sheets** (`sheets/Ownship|Target|Tug|Bollard`) → per-placed-body editor in `Tabs`. Props: `data: { bodyId }`, `activeTab`. Compose tabs:
- Ownship: `GeneralTab`, `AisColregTab`, `AnchorsTab`, `LinesTab`.
- Target/Tug: `GeneralTab`, `AisColregTab`.

**Reusable cards** (`cards/`) — each its own RHF form, `bodyId`+`bodyType` props, autosave on change:
- `PositionAndCourseCard` → lat/lon/course/speed (unit-aware).
- `ShipDimensionsCard` → vessel dimensions.
- `AisCard` → AIS identity fields.
- `ColregCard` → COLREG behavior.

**Build flow**
- `BuildControl` → primary build button + `BuildHandler`. Prop: `disabled`.
- `BuildHandler` → build state machine + confirmation dialogs; assembles `Build` payload from sub-modals. Prop: `onClose`.
- `BuildSimulationModal` (flags), `DebriefingTextBackupModal`, `DebriefingSettingsModal`, `EvaluationSettingsModal` (+ `StudentCard`/`StudentNotesModal`), `BuildSimulationLoading` → step modals; each `data`/`onContinue`/`onGoBack`/`onClose`.

**Map / placement support**
- `RuntimeMap` (`components/Map`) → MapLibre canvas, renders body/anchor/line/SAR polygons + `BodyPreview`. Prop: `scenarioBounds`.
- `useAddBodyHandler` → translates map clicks into body create/move based on `currentTool`/`currentBodyType` (`makeVessel`/`makeBollard`/`makeSar`).
- `ZoomControl`, `CompassControl`, `Measurement` → map overlay controls.

## 8. Portuguese UI strings

All UI text goes through `react-i18next` `t('key')` — there are **no hardcoded Portuguese strings** in these components. Labels reference keys like `new-scenario`, `scenario-name`, `date-and-time`, `scenario-area`, `select-an-area`, `view-area-details`, `area-information`, `ne/nw/se/sw-corner`, `scenario-description`, `create-scenario`, `cancel`, `general-information`, `environmental-conditions`, `general`, `current-wind-waves`, `position-and-course`, `save-scenario-and-continue`, `save.success`, `popup.scenario-no-ownship.*`, `validator.scenario-must-have-name`. Portuguese resolves at runtime from the i18n locale bundle (not in this tree). One Portuguese inline code comment exists: `// TODO: Adicionar skeleton e tirar o defaultData inicial` in `ScenarioInformation/index.tsx`.

## Key file paths
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/app/Root/index.tsx` (routes)
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/app/Home/NewScenario/index.tsx` + `functions.ts`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/features/newScenario/panels/NewScenarioPanel/{index.tsx,schemas.ts,types.ts}`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/app/EditScenario/index.tsx`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/features/editScenario/panels/{VesselSelection,BollardSelection,ScenarioInformation,EnvironmentalConditions,Settings}/index.tsx`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/features/editScenario/handlers/BuildHandler/index.tsx`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/features/editScenario/sheets/{Ownship,Target,Tug,Bollard}/index.tsx`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/features/editScenario/cards/{PositionAndCourseCard,ShipDimensionsCard,AisCard,ColregCard}/index.tsx`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/store/editScenario/{useEditScenarioStore,types,useVesselFilterStore,useBollardFilterStore,useEvaluationStore,useDirtyChanges}.ts`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/store/panelStorage/types.ts` + `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/hooks/usePanel.ts`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/components/FormStepper/types.ts`