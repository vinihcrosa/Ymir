I have all the detail I need. Here is the complete UI spec.

## Informacoes do cenario e da simulacao

- **Purpose / when shown**: Instructor-facing screen for a running ("Simulating") naval simulation. Full-bleed nautical map fills the viewport; a right-hand "General information" drawer shows scenario metadata, area/bathymetry selection, and debriefing/recording options. Shown while a simulation is loaded — instructor can review scenario info, toggle recording for debriefing, and add notes. (The board also contains an "Editing" variant of the same layout.)

### Layout regions (frame is 1920×972)
- **TopBar** (top, full width, height ~56px, white, bottom border): app logo + title at left, action toolbar centered, clock at far right.
- **Map / canvas** (fills entire content area, ~1920×916 under topbar): interactive nautical map. The right panel overlays on top of it.
- **Right panel — "General information"** (right side, width ~504px, full content height, white card with subtle border + drop shadow, scrollable). Occupies ~26% of width.
- **Map overlay controls** (bottom-center of map): a small "Simulation Controller" pill (~185×46) and bottom-right a "Map actions" control (~64×32 zoom buttons).
- **Modal (DEFER)**: `SaveToGenerateGMDSSFileAlertDialog` (344×184) — alert dialog, not part of the default view.

### Components

**TopBar (left→right):**
- Cube logo icon (blue) + dropdown chevron.
- Title text: `Scenario name` • `Simulation name` (bullet separator).
- Toolbar icon buttons: a solid blue **"+"** button (primary, filled #1a8ff6), then a row of ghost icon buttons — move/select (4-arrows + chevron), pencil (edit), video camera (record), an **info "1" button shown active** (light-blue active fill `#bbeaff`), cloud, gear/settings.
- Clock at far right: `00:00:00` (monospace-ish, gray).

**Right panel "General information"** (header row: title `General information` + close **×** icon button). Three grouped sections, each a bordered subtle-gray group box:

1. **Scenario information** group
   - Field `Scenario name` — text input, placeholder `Scenario name`.
   - Field `Date and time` — datetime input showing value `12/12/2012  12:25` with trailing calendar icon.
   - Field `Scenario description` — multiline textarea (resizable, large).

2. **Scenario area** group
   - Field `Scenario area` — select/combobox, placeholder `Selected area`, chevron-down; trailing **(i) info icon button** in its own bordered box.
   - Field `Scenario bathymetry` — select/combobox, placeholder `Selected bathymetry`, chevron-down.
   - (Editing variant adds `Bathymetry/ComboboxList`, `NoBathymetryAlert`, `InvalidBathymetryAlert` — DEFER these alert states.)

3. **Simulation information** group
   - Three **checkboxes** (shown checked, blue check):
     - `Save maneuver data on text backup for Debriefing`
     - `Save maneuver data for Debriefing`
     - `Evaluate participants`
   - Field `Debriefing notes` — multiline textarea.
   - Field `Backup Observations` — multiline textarea.

**Map overlay:** zoom-in/zoom-out control (bottom-right), a play/simulation controller pill (bottom-center).

### Exact text & labels (verbatim)
- TopBar: `Scenario name`, `Simulation name`, `00:00:00`
- Panel title: `General information` (close: `×`)
- Section: `Scenario information`
  - `Scenario name` (placeholder `Scenario name`)
  - `Date and time` (value `12/12/2012  12:25`)
  - `Scenario description`
- Section: `Scenario area`
  - `Scenario area` (placeholder `Selected area`)
  - `Scenario bathymetry` (placeholder `Selected bathymetry`)
- Section: `Simulation information`
  - `Save maneuver data on text backup for Debriefing`
  - `Save maneuver data for Debriefing`
  - `Evaluate participants`
  - `Debriefing notes`
  - `Backup Observations`
- Board annotation (not UI): `Painel de informações gerais da simulação`

Note: UI labels are in **English** in this design despite the Portuguese board/screen name. Field labels render in a muted gray, indicating disabled/read-only appearance during Simulating state.

### Design tokens (from get_variable_defs + observed)
Colors:
- `primary/600` / `surface/accent/hc-primary`: `#1a8ff6` (primary blue — + button, logo)
- `surface/accent/active`: `#bbeaff` (active toolbar button fill)
- `content/default/primary`: `#101013` (primary text)
- `content/default/secondary`: `#2f3033`
- `content/default/tertiary`: `#4a4e5a`
- `content/default/sutil`: `#6b7280` (muted/placeholder/disabled labels)
- `surface/default/primary` / `white/100%`: `#ffffff` (panel, topbar)
- `surface/default/secondary`: `#f9fafb` (group box / subtle fills)
- `border/default/primary`: `#e5e7eb` (borders, dividers)
- `surface/default/hc-primary`: `#25272d`; `content/default/hc-primary`: `#ffffff`; `border/default/hc-primary`: `#383a42` (dark/high-contrast set — e.g. the dark controller pill)
- Negative/error: `content/negative/primary` `#c81e1e`, `border/negative/primary` `#fbd5d5` (used by bathymetry alerts — DEFER)
- Shadow `shadow/down/small`: `drop-shadow(0 1px 2px rgba(0,0,0,0.05))`

Layout/typography (observed): Panel width ~504px (inner), ~20px outer padding; group boxes with ~1px `#e5e7eb` border + light fill, internal padding ~16–20px. Section headings bold ~14–15px `#101013`. Field labels ~13–14px in muted gray, left-aligned in a ~2-column label/control row. Inputs full-width with ~1px border, ~6–8px radius, ~36px height; textareas larger with resize handle (bottom-right corner). Checkboxes ~16px, blue when checked.

### Interactions / flows
- **+ button**: primary add action (likely add component/vessel to scenario).
- **Toolbar icons**: move/select tool, edit (pencil), record video, **info toggle (currently active)** opens this General information panel, cloud (save/sync), settings (gear).
- **× (close)**: dismisses the General information panel.
- **Scenario area (i)**: opens info about the selected area.
- **Combobox area/bathymetry**: open dropdown list to pick area then bathymetry (Editing variant shows `Bathymetry/ComboboxList` and validation alerts `NoBathymetryAlert` / `InvalidBathymetryAlert`).
- **Checkboxes**: toggle debriefing recording + participant evaluation flags.
- **Textareas**: free-text debriefing notes & backup observations.
- During **Simulating**, fields appear read-only/disabled (muted labels); editing happens in the **Editing** variant.

### Notes
- **DEFER — Map rendering**: real nautical/marine map tiles (Leaflet/MapLibre style); use a placeholder map initially.
- **DEFER — Simulation Controller pill** (play/pause/time) and **Map actions** (zoom) overlays — separate components.
- **DEFER — Debriefing / recording** features: the three checkboxes plus notes/backup observations imply a debriefing/replay pipeline not yet implemented.
- **DEFER — Bathymetry combobox + validation alerts** (`NoBathymetryAlert`, `InvalidBathymetryAlert`) and the `SaveToGenerateGMDSSFileAlertDialog` modal.
- **DEFER — Towline/flow**: the board's "Flow" section shows multi-screen navigation (not a single screen).
- The `get_variable_defs` call on node 1:4 itself failed ("nothing selected"); tokens above were retrieved successfully from sub-node `6041:5929`.

Relevant downloaded references: `/tmp/figma_1_4.png` (full board), `/tmp/figma_main.png` (main screen), `/tmp/figma_panel.png` (right panel detail), `/tmp/figma_topbar.png` (topbar detail).