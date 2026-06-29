I have all the information needed. Here is the complete UI spec.

## Layout do instrutor (app shell / topbar / panels)

- **Purpose / when shown**: Main instructor workspace shown after a scenario is opened/created. It is a full-bleed nautical map workspace where the instructor places/edits scenario entities (ownship, vessels, markers) and builds/runs a simulation. Variants in the file: default, panel-disabled states, "Loading scenario…" state, and an unsaved-changes alert.

### Layout regions (1920 × 972 reference frame)
- **Topbar (header)** — full width, height 56px (frame 60px incl. bottom border), white surface with 1px bottom border. Three logical zones:
  - **Left (~0–360px)**: Logo button (cube glyph + chevron-down dropdown) → breadcrumb "Pasta pai" › "Nome do cenário" (parent folder › scenario name).
  - **Center (centered group ~880–1040px)**: 4 icon buttons — primary blue "+" (add), an info/panel icon, a cloud icon, a settings gear.
  - **Right (~1700–1900px)**: secondary "Sair do cenário" button + dark primary "Salvar cenário" button (save icon + label).
- **Map / canvas** — everything below the topbar (full remaining viewport, ~1920×916). A pannable nautical/street map (land beige, water blue) is the dominant surface. No persistent left/right side panels in these frames — the UI is map-first with floating overlays.
- **Floating overlays on the map**:
  - **Bottom-center**: `SimulationControl` pill — dark "Build simulation" button (with link/chain icon) grouped with a Play (▷) button and a Stop (▢) button in bordered white cells.
  - **Bottom-right**: `MapActions` zoom control — white pill with "−" and "+" zoom buttons.
- **Modals**: centered `AlertDialog` (unsaved-changes / no-ownship warning), ~304px wide.

### Components & visible states
- **TopBar/LogoButton** (cube + chevron): states default / hover / active. Opens scenario/folder menu.
- **Breadcrumb / ScenarioName** (`TopBar/ScenarioName`): states default / hover / active (editable scenario title; "active" = editing).
- **TopBar/Button** (icon buttons): `type=default` and `type=addButton` (the blue +), each with states default / hover / active / disabled. The "+" add button is filled primary blue; the other three (info, cloud, settings) are ghost/outline icon buttons.
- **TopBar actionsDisabled** variants: `none`, `center` (center icon group greyed), `centerAndRight` (center + save/right greyed) — used when scenario not ready/loading.
- **TopBar/UnsavedChangesWarning**: small warning indicator, `isShowingTooltip = false / true`.
- **Buttons**: "Sair do cenário" (secondary, white, bordered), "Salvar cenário" (primary, dark `#25272d`, white text + save icon). When disabled they render greyed (see Loading state).
- **SimulationControl** (`6068:4329`): segmented control — primary dark "Build simulation" + Play + Stop, each a separate cell; Play/Stop appear in inactive/outline state until a simulation is built.
- **MapActions**: zoom −/+ pill.
- **AlertDialog**: title row with "?" badge + title + "×" close; body paragraph; footer with "Cancelar" (secondary) and "Salvar mesmo assim" (dark primary).
- **Loading state**: 3-dot progress indicator + "Loading scenario…" centered; topbar icons/save shown disabled.
- **Map markers**: a magenta/pink vessel/heading marker and a pink curved track/path drawn on the water (entity + its trajectory).

### Exact text & labels (verbatim)
- Breadcrumb: `Pasta pai` › `Nome do cenário` (also appears in English mock as `Scenario name`).
- Topbar right buttons: `Sair do cenário` / `Salvar cenário` (English mock variant: `Exit scenario` / `Salvar cenário`).
- Simulation pill: `Build simulation`.
- Loading: `Loading scenario...`
- AlertDialog title: `Seu cenário não possui um ownship`
- AlertDialog body: `Seu cenário não possui um ownship, portanto não será possível utilizá-lo em uma cabine. Tem certeza que deseja salvar?`
- AlertDialog buttons: `Cancelar` / `Salvar mesmo assim`
- (Note: file mixes PT and EN placeholder strings; ship the Portuguese ones.)

### Design tokens (from get_variable_defs + observed)
Colors:
- `primary/600` / `surface/accent/hc-primary` = `#1a8ff6` (blue — add button, active accents)
- `content/default/primary` = `#101013` (primary text)
- `content/default/secondary` = `#2f3033`
- `content/default/tertiary` = `#4a4e5a`
- `content/default/sutil` = `#6b7280` (muted/breadcrumb)
- `surface/default/primary` = `#ffffff` (topbar, cards, pills)
- `surface/default/hc-primary` = `#25272d` (dark primary button bg — "Salvar cenário", "Build simulation")
- `content/default/hc-primary` = `#ffffff` (text on dark buttons)
- `border/default/primary` = `#e5e7eb` (1px borders/dividers)
- `border/default/hc-primary` = `#383a42`
- `white/100%` = `#ffffff`
Effects:
- `shadow/down/small` = drop-shadow `#0000000D`, offset (0,1), blur 2, spread 0 (use on topbar, floating pills, dialog).
Geometry (measured): topbar content height ≈ 56px; icon buttons 40×40; buttons radius ≈ 8px; floating pills radius ≈ 10–12px; dialog ≈ 304px wide. Typography: system/sans, labels ~13–14px medium; breadcrumb ~14px regular; title text dark `#101013`.

### Interactions / flows
- Logo/chevron → opens folder/scenario navigation menu. Breadcrumb is informational; scenario name field is inline-editable (hover/active states).
- `+` (blue) → primary add action (add entity to scenario/map). Info, cloud, settings → side functions (details panel, cloud/sync, settings).
- "Build simulation" → compiles scenario into a runnable sim; then Play starts and Stop halts playback.
- Zoom −/+ → map zoom out/in; map is pan/zoom enabled.
- "Salvar cenário" → save; if scenario has no ownship, the AlertDialog appears: "Cancelar" aborts, "Salvar mesmo assim" saves anyway. "Sair do cenário" exits the workspace.
- `actionsDisabled` topbar variants gate center/save actions while loading or when prerequisites are unmet (mirrors the "Loading scenario…" disabled state).

### Notes (DEFER / not yet implementable)
- **Map tile rendering** (real street/nautical basemap) — needs a map provider/library; treat basemap as DEFER, but build the topbar + floating control shells now.
- **Simulation playback** (Build/Play/Stop wired to dynamics engine) — UI shell now; live sim control = DEFER until sim runtime integration.
- **Map entity markers & trajectory paths** (the pink vessel + curved track) — DEFER editable-marker/towline/anchor interactions; the static markers shown are placeholders.
- **Cloud icon** (sync/cloud save) and **UnsavedChangesWarning tooltip** behavior — DEFER until backend sync defined.

Relevant downloaded reference images (absolute paths): `/tmp/figma_layout_4538.png` (main layout), `/tmp/figma_layout_4716.png` (with zoom control), `/tmp/figma_topbar.png` (topbar detail), `/tmp/figma_dialog.png` (alert), `/tmp/figma_loading.png` (loading + disabled topbar), `/tmp/figma_simctl.png` (simulation control), `/tmp/figma_mapactions.png` (zoom). No tool calls failed except the initial `get_variable_defs` on the canvas node (resolved by querying concrete child nodes 6068:4538 and 6068:3388).