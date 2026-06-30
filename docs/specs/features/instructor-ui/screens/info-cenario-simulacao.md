I now have everything needed. The labels in the design are in English (the Figma uses English UI copy), while the screen/tab are named in Portuguese. I'll report the actual visible English strings since that's what the pixels show, and note the Portuguese screen/tab names from metadata.

## Condicoes ambientais do cenario e da simulacao

### Purpose / when shown
Instructor-side configuration of the **environmental conditions** for a scenario/simulation: time of day, water/seabed, meteorology, and the dynamic fields that drive the naval physics (current, wind, waves, swell, visibility). Shown as a right-docked side panel overlaying a full-screen nautical map. The instructor sets these before/during a simulation run.

### Layout regions (full screen = 1920×972)
- **TopBar** (top, full width, 56px tall, white): app title/breadcrumb left ("Condições ambientais"), centered map-tool toggle buttons (3 small square buttons), and a primary blue button top-right ("Save scenario"). On the advanced "edit area" variants the top bar also hosts a `ToolsContainer` (label "Editing: Area 1", divider, wind/waves/current toggle icon-buttons, and a small button).
- **Map / canvas** (fills the rest, left ~75%): full-bleed light street map (OpenStreetMap-style). Floating overlays: bottom-center **SimulationController** (~185×46), bottom-right **MapActions** (~64×32 vertical icon group), and on advanced variants a red current/vector arrow drawn on the water + selectable area polygon.
- **Right panel — EnvironmentalConditionsPanel** (docked right, **484px** wide, full content height 916px, white, left border). This is the primary subject of this screen.
- **Modals (DEFER)**: `CurrentPlanModal` (696×847, many states) and `CurrentMapCard` exist as separate components for the map/plan current editing flow — not part of the base panel.

### Right panel structure
Header row: title **"Environmental conditions"** (bold) + close **×** icon. Below: a 3-item **tab bar** with a blue underline on the active tab. Content changes per tab.

Tabs (visible English copy / Portuguese metadata names):
1. **General** (metadata: `Geral`)
2. **Current, wind and waves** (metadata: `correnteza ondas swell e vento`) — active in the hero shot; also an **advanced** variant
3. **Visbility** [sic — typo in design] (metadata: `Visibilidade`)

#### Tab "General"
Stacked label-on-left / control-on-right rows:
- **Date and time** — text/datetime input, value `12/12/2012  12:25`, trailing calendar icon.
- **Water color** — text input value `#F2208A`, trailing circular "reset/refresh" icon button.
- **Sea bed** — select dropdown, value `Gravel` (chevron-down).
- **Clouds** — select dropdown, value `Clear` (chevron-down).
- Section subheader (blue, small caps-ish): **Meteorological instruments** — a bordered card (accent border) with 4 input rows, label left + value + unit suffix:
  - **Thermometer dry** — `12 °C`
  - **Thermometer mercury** — `13 °C`
  - **Barometer** — `2 mmHg` (placeholder/greyed)
  - **Hygrometer** — `20 %`

#### Tab "Current, wind and waves"
- **Advanced mode** — checkbox (unchecked) + label. Toggles default ↔ advanced card variants.
- **Beaufort scale value** — label + select dropdown, value `None` (chevron-down).
- Four **collapsible accordion cards** (full width, light grey `surface/secondary` fill, rounded, chevron-right when collapsed): **Current**, **Wind**, **Waves**, **Swell**.
- Expanded card (e.g. **Current**, advanced): header with chevron-down, then a **segmented radio row** `Uniform` / `Map` / `Plan` (Uniform selected = filled black dot), then a 3-column input grid:
  - **Direction** — `180 °`
  - **Speed** — `180 knts` (placeholder grey)
  - **Tide** — `2 m`
  - (Wind/Waves/Swell expose analogous Direction/Speed plus their own params; `type=map` and `type=plan…` variants swap the body for map-area / plan editors — those plan/map editors are DEFER.)

#### Tab "Visbility"
- **Rain** — number input `60 %` + horizontal **slider** (blue track + filled blue thumb, ~60%).
- **Fog** — number input `40 NM` + horizontal **slider** (blue thumb, ~40%).

### Components inventory & states
- **Tabs**: active (blue text `#147cec` + 2px blue underline) / inactive (grey `#6b7280`).
- **Accordion card**: `isOpen=false` (chevron-right) / `isOpen=true` (chevron-down); modes `default` / `advanced`; `type` = `uniform | map | plan(Unconfigured|Configured)`.
- **Checkbox**: unchecked square (Advanced mode).
- **Radio group**: Uniform/Map/Plan (single select).
- **Text inputs**: with optional unit suffix (`°`, `knts`, `m`, `°C`, `mmHg`, `%`, `NM`), filled vs placeholder (grey) states.
- **Select/dropdown**: chevron-down (Sea bed, Clouds, Beaufort).
- **Color field**: hex text + circular reset icon button.
- **Datetime field**: calendar icon.
- **Slider**: blue track, circular thumb (Rain, Fog).
- **Buttons**: primary blue ("Save scenario"), icon/menu buttons (TopBar), small secondary button.
- **Map overlays**: SimulationController, MapActions (icon stack), red current arrow / area polygon.

### Exact text & labels (verbatim from pixels)
Screen/tab metadata is Portuguese; rendered UI copy is **English**:
- Panel title: `Environmental conditions` · close `×`
- Tabs: `General` · `Current, wind and waves` · `Visbility` (typo as-is)
- General: `Date and time`, `12/12/2012  12:25`; `Water color`, `#F2208A`; `Sea bed`, `Gravel`; `Clouds`, `Clear`; subheader `Meteorological instruments`; `Thermometer dry` `12 °C`; `Thermometer mercury` `13 °C`; `Barometer` `2 mmHg`; `Hygrometer` `20 %`
- Current/wind/waves: `Advanced mode`; `Beaufort scale value` `None`; cards `Current`, `Wind`, `Waves`, `Swell`; radios `Uniform` / `Map` / `Plan`; inputs `Direction` `180 °`, `Speed` `180 knts`, `Tide` `2 m`
- Visbility: `Rain` `60 %`; `Fog` `40 NM`
- TopBar/map: `Save scenario`; `Editing: Area 1`

### Design tokens (from get_variable_defs + observed)
- Colors:
  - Text primary `#101013`, secondary `#2f3033`, tertiary `#4a4e5a`, subtle/placeholder `#6b7280`
  - Accent (links/active tab/sliders) `#147cec`; accent surface `#1a8ff6`; accent border `#bbeaff`
  - Surface primary `#ffffff`, secondary (cards/inputs) `#f9fafb`; HC surface `#25272d`
  - Border default `#e5e7eb`
- Shadow: `shadow/down/small` = drop-shadow `#0000000D`, offset (0,1), blur 2, spread 0
- Typography (observed): panel title ~16px bold; tab labels ~14px; field labels ~14px regular; unit suffix smaller/grey; section subheader ~12px blue.
- Spacing/sizing: panel width **484px**; TopBar height **56px**; collapsed card row height **44px**; expanded card ~154px; input height ~40px; card corner radius ~8px; consistent ~16px panel padding / ~12–16px row gaps.

### Interactions / flows
- Tab bar switches panel body between General / Current-wind-waves / Visbility.
- **Advanced mode** checkbox reveals advanced card bodies (extra params, Beaufort coupling, map/plan current types).
- Accordion cards expand/collapse (chevron). Inside Current/Wind/Waves/Swell: Uniform = manual Direction/Speed/(Tide) numeric entry; Map = draw/edit a spatial field on the map; Plan = configured time-varying plan.
- Sliders (Rain/Fog) drag to set percent / nautical-mile value, mirrored in the numeric input.
- `Save scenario` persists the configuration. Close `×` dismisses the panel.
- Map tool toggles (TopBar wind/waves/current icons) switch which field overlay is shown/edited; `Editing: Area 1` indicates the active spatial sub-area.

### Notes — DEFER (not yet cleanly implementable)
- **Map "Map" and "Plan" current/wind/wave types** — require spatial field drawing, area polygons, and the red vector-arrow overlay (`CurrentMapCard`, `Vector 31/32`, `ColorSelector`). DEFER.
- **CurrentPlanModal** (7 states incl. `changeTimeAndUsage`, `planSelected`, etc.) — time-varying plan editor modal. DEFER.
- **SimulationController** behavior (play/scrub timeline) and **MapActions** overlay actions — DEFER pending the map/simulation runtime.
- The actual basemap tiles (OpenStreetMap) are placeholder; integrate the project's real map layer separately.
- Tab label `Visbility` is misspelled in the design — recommend rendering correct copy (PT `Visibilidade` / EN `Visibility`) in code.

Relevant Figma node IDs: full screen `4472:51949`; panel component `4472:41983` (tabs `4472:41984` Geral, `4472:42015` current/waves, `4484:34161` advanced, `4472:42037` Visibilidade); cards `CurrentCard 4484:30778`, `WindCard 4472:42281`, `WaveCard 4472:42297`. Local screenshots: `/tmp/figma_fullscreen.png`, `/tmp/figma_panel.png`, `/tmp/figma_tab_geral.png`, `/tmp/figma_tab_vis.png`, `/tmp/figma_current_adv.png`.