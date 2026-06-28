# Frontend Conventions — apps/web

## Stack

- **Framework**: React 18 + TypeScript
- **Build**: Vite 6
- **Estado global**: Zustand
- **Tipos compartilhados**: `@ymir/types`
- **Workers**: Web Workers nativos (ES modules)

---

## Estrutura de Pastas

```
src/
├── features/              # Código organizado por domínio
│   ├── simulation/
│   │   ├── components/    # Componentes específicos da feature
│   │   ├── hooks/         # Hooks específicos da feature
│   │   └── store.ts       # Zustand store da feature
│   └── vessels/
│       ├── components/
│       ├── hooks/
│       └── store.ts
├── components/            # Componentes genéricos reutilizáveis (sem domínio)
├── hooks/                 # Hooks genéricos compartilhados
├── stores/                # Stores globais (cross-feature)
├── workers/               # Web Workers
├── lib/                   # Utilitários puros (sem React)
└── main.tsx
```

Features são verticais autossuficientes: um componente de `simulation/` não importa de `vessels/` diretamente — dados compartilhados passam por store global.

---

## Componentes

**Componentes são burros**: recebem dados via props e disparam ações via callbacks ou stores.
Sem fetching, sem lógica de negócio, sem efeitos colaterais dentro de componentes.

```tsx
// ✅ correto
function VesselCard({ vessel, onSelect }: VesselCardProps) {
  return <div onClick={() => onSelect(vessel.id)}>{vessel.name}</div>
}

// ❌ errado — componente buscando dados
function VesselCard({ id }: { id: number }) {
  const [vessel, setVessel] = useState(null)
  useEffect(() => { fetch(`/vessels/${id}`).then(...) }, [id])
  ...
}
```

Lógica de busca de dados fica em **custom hooks**:

```tsx
// hooks/use-vessel.ts
export function useVessel(id: number) {
  const [vessel, setVessel] = useState<VesselDTO | null>(null)
  useEffect(() => {
    fetch(`/api/vessels/${id}`)
      .then(r => r.json())
      .then(setVessel)
  }, [id])
  return vessel
}
```

---

## Estado

**Regras:**

| Tipo de estado | Onde fica |
|----------------|-----------|
| Estado de UI puro (modal aberto, hover, input temporário) | `useState` local |
| Estado de feature (simulação rodando, lista de vessels) | Zustand store da feature |
| Estado cross-feature | Zustand store global em `stores/` |
| Cache de dados remotos | Custom hook com `useState` (ou SWR futuramente) |

Sem prop drilling além de 2 níveis — extrair para store.
Sem Context API para estado de aplicação — usar Zustand.

**Estrutura de store:**

```typescript
// features/simulation/store.ts
import { create } from 'zustand'

interface SimulationStore {
  // estado
  status: 'idle' | 'running' | 'error'
  // ações
  start: () => void
  stop: () => void
}

export const useSimulationStore = create<SimulationStore>((set, get) => ({
  status: 'idle',
  start: () => set({ status: 'running' }),
  stop:  () => set({ status: 'idle' }),
}))
```

---

## Custom Hooks

- Prefixo `use` obrigatório
- Co-localizados com a feature quando específicos
- Em `hooks/` quando reutilizáveis entre features
- Encapsulam lógica — componentes não fazem `useEffect` com fetch direto

```typescript
// ✅ hook com responsabilidade clara
export function useVesselList(): { vessels: VesselDTO[]; loading: boolean } {
  ...
}

// ✅ hook de store — apenas re-exporta slice relevante
export function useSimulationStatus() {
  return useSimulationStore(s => s.status)
}
```

---

## Naming

| Coisa | Padrão | Exemplo |
|-------|--------|---------|
| Arquivos de componente | `PascalCase.tsx` | `TelemetryPanel.tsx` |
| Arquivos de hook | `use-kebab-case.ts` | `use-vessel-list.ts` |
| Arquivos de store | `store.ts` (na feature) | `features/simulation/store.ts` |
| Arquivos utilitários | `kebab-case.ts` | `format-number.ts` |
| Componentes | `PascalCase` | `VesselCard` |
| Hooks | `use[Ação/Entidade]` | `useVesselList`, `useSimulationStatus` |
| Stores | `use[Domain]Store` | `useSimulationStore` |
| Props types | `[Componente]Props` | `VesselCardProps` |

---

## TypeScript

- `import type` obrigatório para importações tipo-only
- Sem `any` — exceção: boundary com WASM (documentar com comentário)
- Props sempre tipadas com interface ou type alias explícito — sem inline objects grandes
- Sem `!` non-null assertion onde pode ser evitado
- Sem `enum` — usar `as const` ou union types

```typescript
// ✅
import type { VesselDTO } from '@ymir/types'

interface VesselCardProps {
  vessel: VesselDTO
  onSelect: (id: number) => void
}

// ❌
function VesselCard(props: any) { ... }
```

---

## Organização de Arquivos de Componente

Para componentes simples: um arquivo `.tsx`.
Para componentes com lógica complexa: co-locar hook e tipos:

```
features/simulation/components/
├── TelemetryPanel.tsx       # componente
├── TelemetryPanel.hook.ts   # lógica extraída (quando > ~30 linhas)
└── TelemetryPanel.types.ts  # tipos locais (quando extensos)
```

---

## Regras Gerais

- Arquivo máximo: **~200 linhas** — dividir se passar
- Sem barrel `index.ts` dentro de features (importar direto do arquivo)
- Barrel `index.ts` permitido em `components/` e `hooks/` globais
- Sem `console.log` em código de produção
- Ordenação de imports: `externos → @ymir/* → features/ → components/ → relativos`
- Componentes funcionais apenas — sem class components
