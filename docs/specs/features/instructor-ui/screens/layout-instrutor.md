I have all the data needed. Here is the complete UI spec.

## Meus cenarios (scenario list / management)

- **Purpose / when shown**: Landing screen of the Instructor app. Lists all scenarios the user has created so they can open, search, create, rename, duplicate, or delete them. Default view is **grid**; an alternate **list** view exists. Has empty, loading (skeleton), search-empty, and error states. (Note: the live UI mockups use **English** strings; the Portuguese strings in the file are documentation/annotation captions, not UI copy. I copy both below.)

- **Layout regions** (canvas is 1920×972):
  - **Left sidebar** (~280px wide, white, right border): brand block "INSTRUCTOR" + caret at top; nav item "My scenarios" (active, light gray pill); "Settings" with gear icon pinned to bottom.
  - **Topbar** (~64px tall, inside content area): page title "My scenarios" (left); right cluster = search input + view-toggle (grid/list segmented control) + "Load replay" button + "+ New scenario" primary button.
  - **Main content** (fills remaining ~1640px): scrollable. Grid view = 5 columns of cards. List view = full-width table with a header row + rows. Thin scrollbar on right edge.
  - **Modals/overlays**: centered card on a semi-transparent dark scrim (`black/50%` #10101380) — Rename, Duplicate, Delete confirmation, Load backup. Context menus (right-click + per-row "...").

- **Components & states**:
  - **Sidebar nav item**: default / active (active = `surface/default/active` #e5e7eb pill). Icon + label.
  - **Search input**: leading magnifier icon, placeholder text, rounded, gray border `#e5e7eb`. Focus/active border `#9ca3af`. Toolbar variant `location=search` exists.
  - **View toggle**: 2-icon segmented control (grid icon / list-rows icon). Selected icon highlighted (blue/active fill); unselected gray.
  - **Buttons**: "Load replay" = secondary (white bg, border, leading icon). "+ New scenario" = primary (blue `#1a8ff6`, white text, leading + icon). Modal buttons: "Cancel" = ghost/secondary, "Rename"/"Duplicate" = dark/primary (#101013 / #25272d in dark mode).
  - **Scenario card (grid, 292×263)**: top icon-stat row (7 icons each with a count: `1` ship/ownship, `2` target/contacts ◎, `6` route ⛟, `6` anchor ⚓, `6` path ✎ — counts/icons are placeholders); section "Ownships" with up to 2 ship rows ("Nome do ownship 1 - Resto do nome gra…" truncated, "Nome do ownship 2"); section "Environmental conditions" with 2-col grid: wind `12° 100knts` + "Wave plan 1", current "Current plan 1" + "Swell plan 3"; below card: bold title ("Nome que está listado para testar um nome grande", 2-line clamp) + subtitle area + date ("Mucuripe • 12/12/2024 12:40"). States: default, hover, skeleton1/skeleton2 (pulsing load), loadingInfo, infoskeleton1/2.
  - **List row**: small ship thumbnail, "Nome do item", content icon-stat row (same 7 icons), date+time "21/05/2024 18:05:37", area "Mucuripe", trailing "Open" button + "..." overflow menu. Header row labels: **Name / Content / Date and time / Area**. Row states: default / hover / active.
  - **Context menu (right-click empty area)**: single item "New scenario" (147×32 — `MyScenariosContextMenu`).
  - **Scenario options menu (per card/row "...")**: ~147×97, ~3 actions (Rename / Duplicate / Delete) — `ScenarioOptions`.
  - **Rename modal** (481×183): pencil icon + title "Rename scenario", close ×; label "Scenario name"; text input (placeholder "Scenario name"); footer "Cancel" + "Rename". Error variant shows red border + helper text (name required).
  - **Duplicate modal**: same shape, title "Duplicate scenario", prefilled `{name} {cópia}`; "Cancel" + "Duplicate"; error variant.
  - **Delete confirmation dialog** (304×144): destructive (red) confirm button.
  - **Load backup modal** (`OpenAutosavedDraft`, 380×178): recover-draft prompt. DEFER.

- **Exact text & labels**:
  - UI (English): `INSTRUCTOR`, `My scenarios`, `Search for scenario` (placeholder), `Load replay`, `+ New scenario`, `Settings`, `Ownships`, `Environmental conditions`, `Nome do ownship 1 - Resto do nome gra…`, `Nome do ownship 2`, `12° 100knts`, `Current plan 1`, `Wave plan 1`, `Swell plan 3`, `Nome que está listado para testar um nome grande`, `Mucuripe • 12/12/2024 12:40`. List headers: `Name`, `Content`, `Date and time`, `Area`; row: `Nome do item`, `21/05/2024 18:05:37`, `Mucuripe`, `Open`. Rename modal: `Rename scenario`, `Scenario name`, `Cancel`, `Rename`.
  - Documentation captions (PT, for behavior reference only): "A visualização padrão dos meus cenários é em grade… ordem de criação, do mais novo para o mais velho." / "Ao clicar no botão do menu superior, o usuário consegue alternar entre… grade e em lista." / "Ao clicar no botão '...' de um cenário, um menu de ação aparece…" / "Ao digitar o nome de um cenário, uma listagem filtrada… Ao apagar a busca ou clicar no botão de voltar, a busca é apagada…" / "Ao clicar com o botão direito na listagem… a única ação dele é criar um novo cenário." / "Quando os cenários estiverem carregando, aparecerão skeletons que piscam suavemente." / "Quando acontecer um erro de carregamento, deve ser mostrada uma mensagem ao usuário." / "Quando o usuário estiver em 'Meus cenários' e não possuir nenhum cenário, um estado vazio… dando a indicação de criar um cenário." / "Quando o usuário buscar… e nenhum aparecer, deve ser mostrado um estado vazio." / Rename: "nome é obrigatório e o cenário não pode ser salvo sem nome." / Duplicate: "Por padrão, aparece o mesmo nome do cenário e '{cópia}' na frente dele." / Delete: "um confirmation dialog… perguntando se o usuário deseja mesmo apagar o cenário." / Backup: "Caso o instrutor… tenha fechado abruptamente… modal para carregar backup… O backup não é salvo automaticamente no backend."

- **Design tokens** (from get_variable_defs):
  - Colors: accent/primary blue `#1a8ff6` (`surface/accent/hc-primary`, `content/accent/secondary`); accent border `#bbeaff`. Text: primary `#101013`, secondary `#2f3033`, tertiary `#4a4e5a`, subtle `#6b7280`, hc-subtle `#9ca3af`. Surfaces: primary `#ffffff`, secondary `#f9fafb`, active `#e5e7eb`. Borders: primary `#e5e7eb`, active `#9ca3af`. Dark-mode surfaces (used in dark variants): `#25272d` surface, `#383a42` border, white text `#ffffff`. Scrim: `black/50%` `#10101380`.
  - Shadow: `shadow/down/small` = drop shadow color `#0000000D`, offset (0,1), blur 2, spread 0.
  - Typography (observed): page title ~16px semibold; section labels ("Ownships", "Environmental conditions") ~12px medium subtle; card title ~14px semibold 2-line clamp; card meta/date ~12px subtle; list cells ~13–14px. Sans-serif (Inter-like).
  - Radii: inputs/buttons ~6–8px; cards ~8–12px; modal ~12px.
  - Spacing: card grid 5 cols with ~16–24px gutters; card internal padding ~16px; toolbar gap ~8–12px between controls.

- **Interactions / flows**:
  - Grid is default sort: newest → oldest by creation date.
  - View toggle switches grid ↔ list (state persists in toolbar; both views share the same data/search).
  - Search filters list by name as you type; clearing the text or pressing back removes the filter and returns to full listing (works in both views). Empty-search result → search-empty state.
  - "+ New scenario" and right-click context menu "New scenario" both create a scenario.
  - Card/row "..." opens action menu (Rename / Duplicate / Delete). "Open" (list) / clicking a card opens the scenario.
  - Rename: opens modal; name required → empty name blocks save and shows error variant.
  - Duplicate: opens modal prefilled with `{original} cópia`; name required; error variant on empty.
  - Delete: opens confirmation dialog; confirm deletes.
  - Loading state shows pulsing skeletons; load failure shows an error message screen.

- **Notes / DEFER**:
  - **Load replay** button — replay system not yet implementable. DEFER.
  - **Load backup / autosaved draft** modal (`OpenAutosavedDraft`) — depends on draft/autosave backend not yet built; doc notes backend does not auto-persist. DEFER.
  - **Folders** — folder item/options components exist in the library (`type=folder`, `FolderOptions`, `CreateFolderModal`) but are all hidden in this screen; not part of current scope. DEFER.
  - Card **icon-stat counts and icons** (ships/targets/routes/anchors/paths) are placeholder values; real mapping to scenario data TBD.
  - UI copy in mockups is **English**; if the app should ship in Portuguese, strings need localization (the PT documentation captions are not the button labels).

Relevant artifacts saved: `/tmp/figma_247_39821.png` (full page), `/tmp/figma_grid.png` (grid default), `/tmp/figma_list.png` (list), `/tmp/figma_rename.png` (rename modal), `/tmp/figma_card.png` (single grid card). Key node IDs: grid `247:120437`, list `247:120719`, rename overlay `247:120761`, single card `247:67412`, context menu `250:47484`, scenario options `250:47439`.