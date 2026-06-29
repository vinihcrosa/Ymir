I have enough detail. The anchor panel lower portion (Corrente section) mirrors the Guincho section. I have all tokens, text, and layout. Let me compile the spec.

## Embarcacoes, ancoras e linhas

**Source frames** (Figma file `N5vsY5EB5TnSCvZGiZjV62`, canvas "💻 UI v. MAP"). The requested node `6194:2969` is the whole flow-map page; this screen is the **Ownship vessel-editing panel** with 4 tabs plus a detached anchor editor. Representative frames:
- `6256:28060` Painel de Ownship - Informações (panel instance `6256:28061`)
- `6256:28155` Painel de Ownship - AIS/RIPEAM (`6256:28156`)
- `6256:28174` Painel de Ownship - Âncoras (`6256:28175`)
- `6256:28193` Painel de Ownship - Linhas (`6256:28194`)
- `6256:28212` Painel de âncora (`6256:28213`)

### Purpose / when shown
Scenario-editing screen. After a vessel (Ownship) is placed on the map, the instructor opens its right-side editor to configure identity/dimensions/position (Informações), AIS & RIPEAM signals, mooring anchors (Âncoras), and tow/mooring lines (Linhas). Selecting "Editar" on an anchor opens a dedicated full anchor editor panel (replaces the tabbed panel).

### Layout regions
- **TopBar** (full width, height 56px): app logo + scenario-name dropdown on left; centered action cluster; "Sair do cenário" + "Salvar cenário" buttons on right.
- **Map / canvas**: fills the entire content area (1920×916) behind the panel — pannable nautical chart (Rio de Janeiro / Guanabara bay tiles) with the vessel rendered as a red ship glyph.
- **Right panel** (floating card, ~398×848, anchored top-right ~24px inset): the vessel editor. White rounded card, subtle border + drop shadow. This is the only foreground UI region; ~21% of screen width.
- **Bottom-center map controls**: "Build" pill button, play (▶) button — part of the scenario shell, not this panel.
- **Bottom-right**: zoom +/- control.

### Components

**Panel header (all tabbed variants)**
- Eyebrow label `Embarcação Ownship` (small, tertiary/grey).
- Title (bold, ~18px): `Comboio_norsul_L154_b22_T0440123_lemes_separados` (wraps 2 lines).
- Close `×` icon, top-right.
- Two action buttons below title: a "focus/recenter" icon button (square, crosshair glyph) and a `•••` overflow icon button (both white, bordered, rounded ~8px).

**Tab bar** — leading hamburger/list icon (`≡`) then 4 text tabs: `Informações`, `AIS/RIPEAM`, `Âncoras`, `Linhas`. Active tab = blue text (#147cec / #1a8ff6) with blue underline; inactive = dark grey, no underline. Thin divider line under the row.

**Informações tab**
- Group `Geral` in a bordered rounded card: rows `Tipo` (select dropdown, value "Ownship"), `Nome` (text, "Comboio_norsul"), `MMSI` (text "477423500" + pencil/edit icon button), `ID` (read-only "0").
- Group `Dimensões da embarcação` (bordered card, single row, 3 inline fields): `LOA (m)` 58m · `Beam (m)` 20m · `Draft (m)` 2m.
- Group `Posição e direções` (bordered card): `Lat` (-26° 52' 53" N), `Long` (-26° 52' 53" N), `Heading` (192°), `Speed` (numeric "20" + unit dropdown "knts").
- Labels are grey, left-aligned; values/inputs right side in monospace-looking font.

**AIS/RIPEAM tab**
- Group `AIS` (bordered card): `Ship name` Comboio_norsul · `Call Sign` VRKB8 · `IMO` 9526887 · `MMSI` 477423500 (+ edit icon) · `Type` 50 - Pilot ship · `AIS status` (dropdown "0 = Under way using engine").
- Group `RIPEAM`: a checked checkbox + label `Alterar RIPEAM automaticamente utilizando status de AIS`. Below, three disabled (greyed) dropdowns: `Lights`, `Shapes`, `Sounds`, each prefilled "0 = Under way using engine" (disabled because the checkbox is on).

**Âncoras tab**
- List of anchor cards (2 shown). Each card has a chip/legend label on the top border: `Âncora boreste`. Rows: `Nome` (truncated "Nome da âncora longo só pra m…"), `Tipo` ("Tipo da âncora"), `Corrente` ("2 de 16 quarteladas"), then a `Lat` / `Long` row (-26° 52' 53" N / -26° 52' 53" N).
- Each card has an `Editar` button (top-right, white bordered) → opens the anchor editor panel.

**Linhas tab**
- List of line cards (2 shown). Border chip label `Linha 1`. Rows: `Material` (Nylon), `Diâmetro` (20m), `Comprimento` (20m); a dashed horizontal divider; then `Corpo A` ("Nome longo do corpo A para most…", truncated) and `Corpo B` ("Nome longo do corpo B para most…").
- Each card has an `Editar` button top-right.

**Anchor editor panel** (`Painel de âncora`, replaces tabbed panel)
- Header: back button `‹ Ver embarcação` (white bordered pill) + close `×`.
- Eyebrow `Âncora boreste`; title `Ancora_P3D`.
- Group `Informações da âncora` (card): `Nome` (Ancora_P3D), `Tipo` (dropdown "Tipo da âncora"), `Peso` (placeholder "120kg", greyed).
- Subsection `Holding powers (ton)`: 2×2 grid of fields `Mud` / `Sand` / `Gravel` / `Rock`, all placeholder "120.23".
- Group `Posição e direções`: `Latitude`, `Longitude` (-26° 52' 53" N), `Ângulo` (20°), `Comprimento` (numeric "2" + suffix text "de 16 quarteladas"), `Profundidade` (placeholder "30m").
- Group `Informações do guincho e da corrente`: card `Guincho` with a circular reset/refresh icon button; `Tipo` (dropdown "Tipo do guincho"), `Winch speed` ("12" + unit "m/min"), `Pull force` ("200" + "kN"), `Break force` ("290" + "ton"). Below begins a `Corrente` card with the same reset icon (mirrors the Guincho block — chain/current properties). Panel scrolls.

### Exact text & labels (verbatim Portuguese/English as shown)
- Topbar: `Scenario name`, `Sair do cenário`, `Salvar cenário`, `Build`
- Eyebrow: `Embarcação Ownship` · Title: `Comboio_norsul_L154_b22_T0440123_lemes_separados`
- Tabs: `Informações` · `AIS/RIPEAM` · `Âncoras` · `Linhas`
- Groups: `Geral`, `Dimensões da embarcação`, `Posição e direções`, `AIS`, `RIPEAM`, `Informações da âncora`, `Holding powers (ton)`, `Informações do guincho e da corrente`
- Field labels: `Tipo`, `Nome`, `MMSI`, `ID`, `LOA (m)`, `Beam (m)`, `Draft (m)`, `Lat`, `Long`, `Heading`, `Speed`, `Ship name`, `Call Sign`, `IMO`, `Type`, `AIS status`, `Lights`, `Shapes`, `Sounds`, `Material`, `Diâmetro`, `Comprimento`, `Corpo A`, `Corpo B`, `Corrente`, `Latitude`, `Longitude`, `Ângulo`, `Profundidade`, `Peso`, `Mud`, `Sand`, `Gravel`, `Rock`, `Winch speed`, `Pull force`, `Break force`
- RIPEAM checkbox: `Alterar RIPEAM automaticamente utilizando status de AIS`
- Card chips: `Âncora boreste`, `Linha 1`
- Buttons: `Editar`, `Ver embarcação` (with `‹`)
- Sample values/units: `knts`, `192°`, `-26° 52' 53" N`, `Nylon`, `2 de 16 quarteladas`, `50 - Pilot ship`, `0 = Under way using engine`, `m/min`, `kN`, `ton`, `120.23`, `120kg`, `30m`, `de 16 quarteladas`, `Tipo da âncora`, `Tipo do guincho`

### Design tokens (from get_variable_defs + observed)
Colors:
- `surface/default/primary` = `#ffffff` (panel/cards/buttons)
- `content/default/primary` = `#101013` (titles, values)
- `content/default/secondary` = `#2f3033`
- `content/default/tertiary` = `#4a4e5a`
- `content/default/sutil` = `#6b7280` (field labels, eyebrow, disabled text)
- `border/default/primary` = `#e5e7eb` (card/input borders, dividers)
- `content/accent/primary` = `#147cec` ; `primary/600` / `surface/accent/hc-primary` = `#1a8ff6` (active tab, links, focus)
- High-contrast set (likely dark-mode): `surface/default/hc-primary` = `#25272d`, `border/default/hc-primary` = `#383a42`, `content/default/hc-primary` = `#ffffff`
- Effect `shadow/down/small` = drop shadow `#0000000D`, offset (0,1), blur 2, spread 0 (panel & button elevation)

Typography: sans-serif for labels/titles; values render in a monospace-style face (tabular). Title ~18px bold; group headings ~13–14px bold dark; field labels ~12–13px grey; values ~13px. Inline 3-up dimension fields use smaller labels.

Spacing / shape: panel inset ~24px from edges; panel/card radius ~12px; button/input radius ~8px; card padding ~16px; row vertical rhythm ~14–16px; topbar height 56px; panel width ~398px.

### Interactions / flows
- Tabs switch the panel body in place (Informações / AIS/RIPEAM / Âncoras / Linhas). Active state = blue text + underline.
- `×` closes the panel (returns to map). Focus/recenter icon recenters map on vessel; `•••` opens overflow menu.
- `Tipo` selects, `AIS status`, `Lights/Shapes/Sounds`, anchor `Tipo`, winch `Tipo`, `Speed` unit are dropdowns.
- AIS/RIPEAM checkbox controls whether Lights/Shapes/Sounds derive from AIS status; when checked they are disabled (greyed). Unchecking should enable manual editing.
- MMSI pencil icon enables editing the MMSI (related screens `MMSI Conflit - Alert/Panel` exist for conflict validation — flag for later).
- `Editar` on an anchor/line card opens its detail editor. Anchor `Editar` → anchor panel; `‹ Ver embarcação` returns to the vessel's tabbed panel.
- Long names truncate with ellipsis (Nome, Corpo A/B).

### Notes (DEFER / not-yet-implementable)
- **Anchors (Âncoras tab + full anchor panel)**: holding powers, winch/chain (guincho/corrente) physics, quarteladas chain length — DEFER unless anchor dynamics exist in the MATLAB Dynamics reference.
- **Lines (Linhas tab)**: towline/mooring-line material/diameter/length and Corpo A/B endpoints — DEFER (towline physics).
- **RIPEAM auto-derivation** (Lights/Shapes/Sounds from AIS status) — needs the AIS-status→RIPEAM mapping table; DEFER the lookup logic.
- **MMSI conflict validation** (`MMSI Conflit - Alert/Panel` frames `7478:74649/74661/74673`) — separate flow, not in these frames; DEFER.
- **Map base layer** (real nautical chart tiles) — provider/tiles not specified here.
- get_variable_defs on the page root failed ("nothing selected"); tokens above were pulled successfully from the panel sub-nodes instead. No spacing/typography variables were exposed by Figma (only colors + one shadow effect) — pixel values above are measured from the 402px-wide high-detail crops.

Relevant downloaded references (local): `/tmp/fig_panel_info.png`, `/tmp/p_ais.png`, `/tmp/p_anchors.png`, `/tmp/p_lines.png`, `/tmp/p_anchorpanel.png`, full-screen `/tmp/fig_info.png`.