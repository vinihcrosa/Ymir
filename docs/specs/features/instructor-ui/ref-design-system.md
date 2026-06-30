I have enough to write the inventory.

# Instructor Design-System Inventory

## Architecture overview
- **Stack**: React 19 + TypeScript, styled-components 6, Radix UI primitives, framer-motion for panel/tab animations, react-hook-form + zod for forms, i18next for i18n.
- **Two layers**:
  1. `@technomar/react-components` (v2.0.15) — the shared design system. Ships the theme (`defaultTheme`/`darkTheme`, type `TechnomarTheme`), `GlobalStyle`, `IconProvider`, toast system, and base primitives (Button, Input, Select, Modal, Sheet, Tabs, Checkbox, Radio, Switch, Tooltip, Alert, Uploader, etc.).
  2. Local `src/renderer/src/components/*` — app-specific wrappers and compositions built on top of the library + Radix, all themed via styled-components `theme`.
- **Wrapper pattern**: local components either (a) thinly re-export a library component widening props (`Button`, `Toogle` extends `@technomar` `Button`), or (b) compose Radix primitives styled with theme tokens (`RadioGroup`, `Tabs`). Local styled files use the `$transientProp` convention (e.g. `$active`, `$fullHeight`, `$horizontal`).

## Provider / app wiring (`App.tsx`)
Nesting order: `MapProvider` → `Theme` → `TooltipProvider` → `Language` → `UnitProvider` → `ErrorBoundary` → `HashRouter` → `WebsocketProvider` → `Root`, with `ToastContainer` (position `bottom-right`) and `<AppStyles/>` siblings.
- **Theme** (`providers/Theme.tsx`): `styled-components` `ThemeProvider` fed `defaultTheme` (light) or `darkTheme`, selected from `useParameterization` zustand store (`theme` = `'light' | 'dark'`).
- **TooltipProvider**: Radix `TooltipProvider delayDuration={0}`.
- **Language**: calls `i18n.changeLanguage` reactively from store.
- `styles/styled.d.ts` augments styled-components `DefaultTheme extends TechnomarTheme`, so `theme.*` is fully typed in every styled block.

## Styling approach
- **styled-components** with template literals; colors/spacing always pulled from `theme.colors.tokens.*` — almost no hard-coded colors (shadows are the exception, inline `rgba`). Example token paths used: `theme.colors.tokens.surface.default.main.{primary,secondary,active}`, `...border.default.main.{primary,active}`, `...content.default.main.{primary,sutil}`.
- `AppStyles.tsx` imports Inter (400/500/600/700) + JetBrains Mono fonts via `@fontsource`, library `GlobalStyle`, the toastify CSS, and `global.css` (only override: `div[data-radix-popper-content-wrapper] { z-index: 999 }`).
- Spacing in styled files is mostly literal px multiples of 4/8 (8, 16, 24); the theme also exposes a `sizes` scale (rem-based, key→value e.g. `4 → 1rem/16px`).

## Theme tokens (`TechnomarTheme`)
- **colors.background**: `primary`, `secondary`.
- **colors.text**: `primary`, `secondary`.
- **colors.scrollbar**.
- **colors.palette**: ramps `primary`, `green`, `lime`, `red`, `yellow`, `gray` each `50…1000`; plus `black(amount)` / `white(amount)` functions (0–1 → hex).
- **colors.tokens** (semantic, the primary API): `surface`, `border`, `content` — each with intents `default | negative | positive | warning | accent`, each a `Token<{main, highContrast?}>`. Sub-fields seen in use: surface `.main.{primary,secondary,active}`, border `.main.{primary,active}`, content `.main.{primary,sutil}`.
- **colors.components**: per-component theme slices (button, input, checkbox, radio, select, sheet, switch, tabs, tooltip, modal, alert, icons…). E.g. `ButtonTheme` has `background.{default,danger,success}.{fill,border}` state sets + `text.*`.
- **sizes**: numeric-key spacing scale `0.5 → 96` mapping to rem/px (2px…384px).
- **Typography**: Inter (UI) + JetBrains Mono (numeric/mono); component `font` prop type `InputFontType = 'JetBrains Mono' | 'Inter'`. Font sizes are literal px in styles (12 sub/label, 14 body/title).

## Reusable components (name → purpose → key props/variants)

Library primitives (`@technomar/react-components`):
- **Button** → actions. `variant: primary|secondary|outlined|text`, `type: default|danger|success|accent`, `size: small|regular|large`, `icon`+`iconPosition: leading|trailing`, `fullWidth`, `loading`, `enableRipple`, `buttonType`.
- **Input** → text field. `size`, `type: regular|error`, `label`, `required`, `helperText`, `leadingIcon`/`trailingIcon` (+`*IconProps`), `trailingAction`, `fullWidth`, `wrapperStyle`.
- **TextArea** → `size`, `type: regular|error`, `label`, `helperText`, `fullWidth`, `required`.
- **Select / Combobox** → `Root`+`Option` compound; `size`, `type: default|error`, `optionsPosition`, `helperText`, controlled `value`/`onChange`.
- **Checkbox / CheckboxGroup, Radio / RadioGroup** (library's own group), **Switch** (`leftLabel`/`rightLabel`).
- **Modal** (compound `Root/Header/Content/Footer`, `width`, `closeModal`), **Sheet** (compound), **Tooltip** (`label`, `side`, `align`, `sideOffset`, `delayDuration`).
- **Alert** (`type/text/title/icon`) + `AlertProvider`/`useAlert`, **ConfirmationDialog** (`mainAction`/`secondaryAction`, `type: default|success|danger`).
- **Uploader**, **IconProvider** (`icon`, `size`, `color`), **toast** + **ToastContainer**, **GlobalStyle**.

Local components (`components/*`):
- **PanelContainer** → renders docked left/right panels from `usePanel` hook with framer-motion slide/fade (spring, stiffness 300/damping 50). No props.
- **Panel** → docked panel shell: `title`, `name`, `leftActions`, `rightActions`, `children`, `fullHeight=true`, `onClose`, `showClose`. Styled container 432px wide, 8px radius, surface bg, `backdrop-filter: blur(25px)`, close via library Button `variant="text" icon="close"`.
- **Tabs** (local) → animated horizontal tabs with overflow "more" dropdown (framer-motion). `tabs: {id,label,content}[]`, `defaultActiveTab`, `showList`, `headerSpacing`, `contentTabSpacing`, `style`/`contentStyle`. Active underline + fade gradient on overflow.
- **Card** → compound `{ Root, Header, Body }`; styled variants via `$direction: row|column`, `$backgroundColor: primary|secondary`, 4px radius, bordered.
- **FormStepper** → generic multi-step modal wizard `<Item>`: `open`, `steps: FormStep<Item>[]`, `initialContext`, `onFinish`, `onClose`. Each step: `title|content` (value or render-fn with `{ctx,setCtx,goNext}`), `icon`, `width/height`, `showFooter`, `onNext/onBack`, `isNextEnabled`. Built on library `Modal`/`Overlay` + local `Button`.
- **ActionButton** → split button + dropdown (`ActionMenu` = styled Radix dropdown). `items: {icon,onSelect,labelKey,separatorBefore,isDisabled}[]`, `buttonIcon`, `isActive`/`onToggleActive`, `tooltipKey`, `t` (passes i18n `TFunction` in). Uses library `Tooltip`+`IconProvider`.
- **EditorInput** → inline edit-on-focus wrapper over local `Input`: `value`, `onUpdate`, `required`, `hasError`, `disabled` + all `InputProps`. Commits on Enter, reverts on Escape/blur-with-error.
- **RadioGroup** (local) → styled Radix `RadioGroup.Root`, prop `horizontal` (`$horizontal` toggles flex-direction), forwards `value`/`defaultValue`/`onValueChange`.
- **Toogle** (note misspelling) → toggle button extending library `Button`; controlled `active` or uncontrolled `defaultActive`, `onToggle(active)`. Styled with active surface/border tokens + inset shadow, min-height 63px.
- **Button** (local) → thin pass-through of library `Button`.
- Other locals: Accordion, ActionMenu, Collapsible, ColorInput, CircularSlider, DateTimeInput, DisplayField(/Form), Form (+ InputForm/SelectForm/SwitchForm/TextAreaForm/MultiSelectForm — react-hook-form bindings), Header, Label, Map, Popover, Sheet, Spinner, ThreeDotsAnimation, TooltipSpan, RequiredAsterisk, ButtonToggleText.
- Local `InputProps` extends library with extra app fields: `unit`, `labelSize`, `font`, `inputSize`, `horizontal`, `autoFocusOnMount`, `isDirty`, `leadingIcon`/`trailingIcon` as `ReactNode`.

## i18n setup (`config/intl/`)
- `i18n.ts`: `i18next` + `react-i18next` (`initReactI18next`). `fallbackLng: 'en'`, `resources: { en: enUS, pt: ptBR }` (translation namespace), `interpolation.escapeValue: false`, `saveMissing` in dev with a console.warn missing-key handler.
- Translation files: `translations-en-US.json` (~39KB), `translations-pt-BR.json` (~41KB), plus `translations.json` (~8KB). Note: code/commits in English, but per project memory all docs/PRD artifacts are Portuguese.
- Language switched reactively via `providers/Language.tsx` from the `useParameterization` zustand store; components consume `useTranslation()` (`t`), and some accept `t`/`labelKey` props (e.g. ActionButton) so translation is injected.

## Guidance for building matching primitives (Ymir web)
- Mirror the **semantic token** model: `surface/border/content` × intent × `{main,highContrast}`, plus palette ramps `50…1000` and `black()/white()` helpers; expose a `sizes` rem scale. Wire via styled-components `ThemeProvider` + a `DefaultTheme` augmentation.
- Reuse the prop vocabulary: `variant`, `type` (default|danger|success|accent), `size` (small|regular|large), compound components for Card/Modal/Sheet/Select, `$transient` styled props.
- Fonts: Inter (UI) + JetBrains Mono (numeric). Radius 4px (cards/inputs) / 8px (panels). Standard gaps 8/16/24px.
- Animations via framer-motion springs (stiffness ~300, damping ~50) for panels/tabs.
- i18n through `react-i18next` with `en`/`pt` resources, `escapeValue:false`, store-driven language switching.

Key reference paths (all absolute):
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/node_modules/@technomar/react-components/dist/index.d.ts` (theme + all component prop types; `TechnomarTheme` at line 805, tokens 291-313, Button 175, Input 494)
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/styles/{AppStyles.tsx,styled.d.ts,global.css}`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/providers/{Theme,Language,TooltipProvider}.tsx`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/config/intl/i18n.ts`
- `/Users/viniciusrosa/Documents/GitHub/technomar/instructor/src/renderer/src/components/{Panel,PanelContainer,Tabs,Card,FormStepper,ActionButton,EditorInput,RadioGroup,Toogle,Button}/`