# Scenario Creator — Tasks

**Spec**: `docs/specs/features/scenario-creator/spec.md`  
**Design**: `docs/specs/features/scenario-creator/design.md`  
**Status**: Pending  

---

## Dependências externas a instalar antes de começar

```bash
pnpm --filter @ymir/web add leaflet react-leaflet @types/leaflet
```

---

## Mapa de dependências

```
T01 (geo.ts)
  └─► T04 (AreaMapView)
        └─► T07 (ScenarioCreatorPage)

T02 (useScenarioStore)
  └─► T03 (useAreas / useVessels hooks)
        └─► T04 (AreaMapView)
        └─► T05 (ScenarioForm)
        └─► T06 (VesselList)

T05, T06, T08 (SimulationControls) ──► T09 (Sidebar)
  └─► T07 (ScenarioCreatorPage)

T10 (extend simulationStore) ──► T07
T11 (extend simulation.worker) ──► T10

T07 ──► T12 (App.tsx swap)
T07 ──► T13 (unit tests)
T12 ──► T14 (E2E tests)
```

---

## Tasks

---

### T01 — `geo.ts`: conversão lat/lng ↔ metros

**Requisitos cobertos**: SC-P1-01, SC-P1-02

**O que**: Criar utilitário de projeção equiretangular local.

**Onde**:
- `apps/web/src/lib/geo.ts` (criar)
- `apps/web/src/lib/geo.test.ts` (criar)

**Depende de**: nada

**Reutiliza**: nada

**Interfaces**:

```typescript
export function latlngToMeters(
  lat: number, lng: number,
  origin: { latitude: number; longitude: number }
): { x: number; y: number }

export function metersToLatLng(
  x: number, y: number,
  origin: { latitude: number; longitude: number }
): [number, number]  // [lat, lng]

export function isInsidePolygon(
  x: number, y: number,
  polygonMeters: Array<{ x: number; y: number }>
): boolean
```

**Done when**:
- [ ] `latlngToMeters` e `metersToLatLng` são inversas (round-trip error < 1m em 100km)
- [ ] `isInsidePolygon` usa ray-casting algorithm
- [ ] Cobertura ≥ 80% no módulo

**Gate**: `pnpm --filter @ymir/web test -- src/lib/geo.test.ts`

---

### T02 — `useScenarioStore`: estado do rascunho de cenário

**Requisitos cobertos**: SC-P1-02, SC-P1-03, SC-P2-01

**O que**: Criar Zustand store para o cenário em construção.

**Onde**:
- `apps/web/src/features/scenario-creator/store.ts` (criar)
- `apps/web/src/features/scenario-creator/store.test.ts` (criar)

**Depende de**: `@ymir/types` (AreaDTO, CreateScenarioDTO)

**Reutiliza**: padrão de `stores/simulationStore.ts`

**Interface do store**:

```typescript
interface ScenarioDraftVessel {
  vesselId: number
  name: string
  x: number         // metros, relativo à origem da área
  y: number
  headingDeg: number
}

interface ScenarioStore {
  name: string
  areaId: number | null
  area: AreaDTO | null
  vessels: ScenarioDraftVessel[]

  setName: (name: string) => void
  setArea: (area: AreaDTO) => void
  addVessel: (v: Pick<ScenarioDraftVessel, 'vesselId' | 'name'>) => void
  removeVessel: (vesselId: number) => void
  updateVesselPosition: (vesselId: number, x: number, y: number) => void
  updateVesselHeading: (vesselId: number, headingDeg: number) => void
  reset: () => void
  toCreateScenarioDTO: () => CreateScenarioDTO
}
```

**Done when**:
- [ ] Todas as ações mudam estado corretamente
- [ ] `toCreateScenarioDTO()` serializa `initialConditions` com `psi = headingDeg * π/180`
- [ ] Nome padrão gerado como `"Cenário ${new Date().toLocaleDateString('pt-BR')}"`
- [ ] Cobertura ≥ 80%

**Gate**: `pnpm --filter @ymir/web test -- src/features/scenario-creator/store.test.ts`

---

### T03 — Hooks de dados: `useAreas` e `useVessels`

**Requisitos cobertos**: SC-P1-01, SC-P1-02

**O que**: Dois hooks que buscam dados da API.

**Onde**:
- `apps/web/src/features/scenario-creator/hooks/use-areas.ts` (criar)
- `apps/web/src/features/scenario-creator/hooks/use-vessels.ts` (criar)
- `apps/web/src/features/scenario-creator/hooks/use-areas.test.ts` (criar)
- `apps/web/src/features/scenario-creator/hooks/use-vessels.test.ts` (criar)

**Depende de**: `@ymir/types`

**Interfaces**:

```typescript
// use-areas.ts
export function useAreas(): {
  areas: AreaDTO[]
  loading: boolean
  error: string | null
  retry: () => void
}

// use-vessels.ts
export function useVessels(): {
  vessels: VesselDTO[]
  loading: boolean
  error: string | null
}
```

**Done when**:
- [ ] Fetch via `VITE_API_URL` env var (fallback `http://localhost:3000`)
- [ ] Estado `loading` correto durante fetch
- [ ] `error` populado em caso de falha de rede
- [ ] `retry()` reexecuta o fetch
- [ ] Testes com `fetch` mockado cobrem: success, loading, error
- [ ] Cobertura ≥ 80%

**Gate**: `pnpm --filter @ymir/web test -- src/features/scenario-creator/hooks/`

---

### T04 — `AreaMapView` + `VesselMarker`: mapa com marcadores

**Requisitos cobertos**: SC-P1-01, SC-P1-02, SC-P1-03, SC-P2-02

**O que**: Componente Leaflet com polígono da área e marcadores draggáveis.

**Onde**:
- `apps/web/src/features/scenario-creator/components/AreaMapView.tsx` (criar)
- `apps/web/src/features/scenario-creator/components/VesselMarker.tsx` (criar)
- `apps/web/src/features/scenario-creator/components/AreaMapView.test.tsx` (criar)

**Depende de**: T01 (geo.ts), T02 (useScenarioStore), `react-leaflet`, `leaflet`

**Instalar antes**:
```bash
pnpm --filter @ymir/web add leaflet react-leaflet @types/leaflet
```

**CSS do Leaflet**: importar `leaflet/dist/leaflet.css` em `main.tsx`.

**Done when**:
- [ ] Mapa renderiza tile OSM centrado na origem da área selecionada
- [ ] Polígono da área desenhado em destaque (cor distinta, sem preenchimento sólido)
- [ ] Bounds do mapa ajustados ao polígono na seleção da área
- [ ] Cada vessel em `useScenarioStore.vessels` tem marcador no mapa
- [ ] Drag do marcador chama `updateVesselPosition` com coords em metros
- [ ] Durante simulação ativa (`status === 'running'`): marcadores seguem `SimulationStateDTO`; drag desabilitado
- [ ] Marcadores com heading rotacionado visualmente
- [ ] Cobertura ≥ 80% (mock `react-leaflet` em testes unitários)

**Nota**: Leaflet precisa de `height` explícito no container CSS — usar `height: 100%` com parent `100vh`.

**Gate**: `pnpm --filter @ymir/web test -- src/features/scenario-creator/components/AreaMapView.test.tsx`

---

### T05 — `ScenarioForm`: nome do cenário + seletor de área

**Requisitos cobertos**: SC-P1-01, SC-P2-01

**O que**: Formulário com campo de nome e dropdown de áreas.

**Onde**:
- `apps/web/src/features/scenario-creator/components/ScenarioForm.tsx` (criar)
- `apps/web/src/features/scenario-creator/components/ScenarioForm.test.tsx` (criar)

**Depende de**: T02 (useScenarioStore), T03 (useAreas)

**Done when**:
- [ ] Input de nome controlado por `useScenarioStore.setName`
- [ ] Dropdown lista áreas de `useAreas()` — cada option tem `value={area.id}`
- [ ] Seleção de área chama `useScenarioStore.setArea(selectedArea)`
- [ ] Estado `loading` exibe "Carregando áreas..."
- [ ] Estado `error` exibe mensagem + botão retry
- [ ] Cobertura ≥ 80%

**Gate**: `pnpm --filter @ymir/web test -- src/features/scenario-creator/components/ScenarioForm.test.tsx`

---

### T06 — `VesselList`: lista de vessels + seletor

**Requisitos cobertos**: SC-P1-02, SC-P1-03

**O que**: Lista vessels adicionados ao cenário; permite adicionar, remover e editar heading.

**Onde**:
- `apps/web/src/features/scenario-creator/components/VesselList.tsx` (criar)
- `apps/web/src/features/scenario-creator/components/VesselList.test.tsx` (criar)

**Depende de**: T02 (useScenarioStore), T03 (useVessels)

**Done when**:
- [ ] Botão "Adicionar vessel" desabilitado quando `area === null`
- [ ] Clique abre seletor com vessels de `useVessels()`
- [ ] Seleção chama `addVessel({ vesselId, name })`
- [ ] Cada item mostra: nome, x/y arredondados, input de heading (0–360)
- [ ] Input de heading chama `updateVesselHeading` e normaliza para [0, 360)
- [ ] Botão remover chama `removeVessel(vesselId)`
- [ ] Cobertura ≥ 80%

**Gate**: `pnpm --filter @ymir/web test -- src/features/scenario-creator/components/VesselList.test.tsx`

---

### T07 — `SimulationControls`: iniciar / parar + status

**Requisitos cobertos**: SC-P1-04

**O que**: Botões de controle da simulação e indicador de status.

**Onde**:
- `apps/web/src/features/scenario-creator/components/SimulationControls.tsx` (criar)
- `apps/web/src/features/scenario-creator/components/SimulationControls.test.tsx` (criar)

**Depende de**: T02 (useScenarioStore), `stores/simulationStore.ts`

**Done when**:
- [ ] "Iniciar" desabilitado quando `areaId === null` ou `vessels.length === 0`
- [ ] "Iniciar" chama `POST /scenarios` com `toCreateScenarioDTO()`, depois `simulationStore.start()`
- [ ] "Parar" chama `simulationStore.stop()`
- [ ] Indicador de status reflete `simulationStore.status` (idle/loading/running/error)
- [ ] Exibe tempo de simulação `t` em segundos quando `running`
- [ ] Cobertura ≥ 80%

**Gate**: `pnpm --filter @ymir/web test -- src/features/scenario-creator/components/SimulationControls.test.tsx`

---

### T08 — `Sidebar`: compõe ScenarioForm + VesselList + SimulationControls

**Requisitos cobertos**: todos (composição)

**O que**: Painel lateral que agrupa os três subcomponentes.

**Onde**:
- `apps/web/src/features/scenario-creator/components/Sidebar.tsx` (criar)

**Depende de**: T05, T06, T07

**Done when**:
- [ ] Renderiza `<ScenarioForm />`, `<VesselList />`, `<SimulationControls />` na ordem correta
- [ ] Largura fixa (~320px), altura 100vh, scroll interno se necessário
- [ ] Sem lógica de negócio — apenas composição e layout

**Gate**: inspeção visual (sem gate de teste unitário — lógica está nos filhos)

---

### T09 — `ScenarioCreatorPage`: layout raiz

**Requisitos cobertos**: todos

**O que**: Container full-screen que posiciona Sidebar + AreaMapView lado a lado.

**Onde**:
- `apps/web/src/features/scenario-creator/ScenarioCreatorPage.tsx` (criar)

**Depende de**: T04, T08

**Done when**:
- [ ] Layout `display: flex; height: 100vh` — Sidebar fixa + mapa ocupa o restante
- [ ] Sem estado próprio — lê stores

**Gate**: inspeção visual

---

### T10 — Extender `useSimulationStore` com `loadScenario`

**Requisitos cobertos**: SC-P1-04

**O que**: Adicionar `loadScenario(vessels)` ao store existente para substituir o `addVessel(1)` hardcoded.

**Onde**:
- `apps/web/src/stores/simulationStore.ts` (modificar)

**Depende de**: nada (T07 consome isso)

**Done when**:
- [ ] `loadScenario(vessels: ScenarioDraftVessel[])` armazena vessels no store
- [ ] `start()` usa vessels do store (não mais `addVessel(1)` hardcoded no worker init)
- [ ] `reset()` limpa vessels

**Gate**: `pnpm --filter @ymir/web test -- src/stores/simulationStore.test.ts` (criar junto)

---

### T11 — Extender `simulation.worker.ts` com suporte a múltiplos vessels

**Requisitos cobertos**: SC-P1-04

**O que**: Worker recebe lista de vessels com condições iniciais reais.

**Onde**:
- `apps/web/src/workers/simulation.worker.ts` (modificar)

**Depende de**: T10

**Done when**:
- [ ] Worker aceita mensagem `{ type: 'loadScenario', vessels: ScenarioDraftVessel[] }`
- [ ] Para cada vessel: chama `simulation.addVessel(id)` e (quando disponível) `simulation.setInitialConditions(id, x, y, psi)`
- [ ] Remove chamada hardcoded `simulation.addVessel(1)` do `initWasm()`

**Gate**: `pnpm --filter @ymir/web test:e2e` smoke test passa

---

### T12 — Atualizar `App.tsx`

**Requisitos cobertos**: todos (entry point)

**O que**: Substituir o `<TelemetryPanel />` atual por `<ScenarioCreatorPage />`.

**Onde**:
- `apps/web/src/App.tsx` (modificar)
- `apps/web/src/main.tsx` — adicionar import de `leaflet/dist/leaflet.css`

**Depende de**: T09

**Done when**:
- [ ] `App.tsx` renderiza apenas `<ScenarioCreatorPage />`
- [ ] `main.tsx` importa `leaflet/dist/leaflet.css` antes de `App`
- [ ] `tsc --noEmit` sem erros

**Gate**: `pnpm --filter @ymir/web build`

---

### T13 — Testes unitários das utilidades e stores

**Requisitos cobertos**: Regra de 80% de cobertura

**O que**: Testes unitários para todos os módulos lógicos não cobertos em T01–T11.

**Onde**:
- `apps/web/src/lib/geo.test.ts`
- `apps/web/src/features/scenario-creator/store.test.ts`
- `apps/web/src/stores/simulationStore.test.ts`
- `apps/web/src/features/scenario-creator/hooks/use-areas.test.ts`
- `apps/web/src/features/scenario-creator/hooks/use-vessels.test.ts`

**Depende de**: T01, T02, T03, T10

**Done when**:
- [ ] `pnpm --filter @ymir/web test` passa com cobertura ≥ 80% em todos os thresholds
- [ ] Nenhum teste usa implementação interna — apenas API pública dos módulos

**Gate**: `pnpm --filter @ymir/web test` — exit 0

---

### T14 — Testes E2E com Playwright

**Requisitos cobertos**: SC-P1-01 a SC-P1-04 (happy paths)

**O que**: Spec E2E cobrindo o fluxo completo do instrutor.

**Onde**:
- `apps/web/e2e/scenario-creator.spec.ts` (criar)

**Depende de**: T12 (app funcionando), API rodando com seed

**Pré-condição**: `pnpm --filter @ymir/api dev` rodando com `vessel1.json` e `area.json`.

**Cenários obrigatórios**:

1. **Seleção de área**: selecionar "baia_de_guanabara" → mapa visível → polígono presente
2. **Adicionar vessel**: clicar "Adicionar vessel" → selecionar VLCC → marcador aparece no mapa
3. **Iniciar simulação**: clicar "Iniciar" → status muda para "running"
4. **Parar simulação**: clicar "Parar" → status muda para estado parado

**Done when**:
- [ ] Todos os 4 cenários passam em Chromium headless
- [ ] `pnpm --filter @ymir/web test:e2e` — exit 0

**Gate**: `pnpm --filter @ymir/web test:e2e`

---

## Ordem de execução recomendada

```
[P] T01 + T02         # paralelos — sem dependência entre si
       ↓
[P] T03 + T10         # paralelos
       ↓
[P] T04 + T05 + T06 + T07 + T11   # paralelos após T01-T03-T10
       ↓
    T08               # depende de T05, T06, T07
       ↓
    T09               # depende de T04, T08
       ↓
    T12               # depende de T09
       ↓
[P] T13 + T14         # paralelos — testes finais
```

**Estimativa**: 14 tasks, ~8 sessões de implementação com subagentes paralelos.

---

## Status

| Task | Descrição | Status |
|------|-----------|--------|
| T01 | geo.ts | Pending |
| T02 | useScenarioStore | Pending |
| T03 | useAreas / useVessels | Pending |
| T04 | AreaMapView + VesselMarker | Pending |
| T05 | ScenarioForm | Pending |
| T06 | VesselList | Pending |
| T07 | SimulationControls | Pending |
| T08 | Sidebar | Pending |
| T09 | ScenarioCreatorPage | Pending |
| T10 | extend simulationStore | Pending |
| T11 | extend simulation.worker | Pending |
| T12 | App.tsx swap | Pending |
| T13 | Unit tests | Pending |
| T14 | E2E Playwright | Pending |
