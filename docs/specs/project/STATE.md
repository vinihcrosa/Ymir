# Ymir — State

## Decisions

| ID | Decisão | Rationale |
|----|---------|-----------|
| D-MONO-01 | pnpm + Turborepo (não NX) | NX não tem suporte nativo C++/CMake; Turborepo é mais simples para 3 apps |
| D-MONO-02 | RK45 Dormand-Prince (não SUNDIALS WASM) | Cross-compilar SUNDIALS é alto risco; naval physics com dt típico não é stiff o suficiente para BDF |
| D-MONO-03 | Embind para bindings WASM | AGENTS.md já exige C API boundary; Embind gera `.d.ts` automaticamente com `--emit-tsd` |
| D-MONO-04 | Fastify v4 + TypeBox | TypeBox schema = validação runtime + tipos TS sem camada extra; integra direto no Fastify |
| D-MONO-05 | Web Worker obrigatório para WASM | Simulation loop bloquearia main thread sem Worker; SharedArrayBuffer requer COOP/COEP headers |
| D-MONO-06 | SQLite + Drizzle ORM | Low volume, read-heavy; alinha com "future SQLite" já mencionado no AGENTS.md |
| D-MONO-07 | Zustand para state management | Leve, sem boilerplate, adequado para estado técnico de simulação a 10-60Hz |

---

## Blockers

_Nenhum no momento._

---

## Todos

- [ ] Validar com usuário: spec + design do M0 (monorepo migration)
- [ ] Após aprovação: criar tasks para M0

---

## Deferred Ideas

- **Three.js/WebGPU para visualização 3D** — aguarda M2 estabilizar
- **SUNDIALS como fallback para stiff scenarios** — se RK45 não for suficiente, interface IIntegrator permite plugar CVODE de volta no target nativo
- **Mooring e ancoragem** — fora de scope do roadmap atual
- **Multi-usuário e autenticação** — decisão adiada até ter produto funcionando

---

## Preferences

_Nenhuma registrada ainda._

---

## Lessons Learned

- Motor C++ atual tem arquitetura ideal para WASM: sem I/O na integration loop, sem heap no hot path, C API boundary já planejada no AGENTS.md
- Phase 3 (WebSocket API) do roadmap original era um stub puro — zero custo de migração
- Melhor janela para pivotar: agora, antes de implementar WebSocket no servidor nativo
