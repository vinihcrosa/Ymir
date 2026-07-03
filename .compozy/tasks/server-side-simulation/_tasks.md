# Server-Side Simulation — Task List

## Tasks

| # | Title | Status | Complexity | Dependencies |
|---|-------|--------|------------|--------------|
| 01 | Pacote `@ymir/wasm` + build Emscripten alvo Node | completed | high | — |
| 02 | Protocolo `ClientMessage`/`ServerMessage` em `packages/types` (add-only) | completed | low | — |
| 03 | `SimulationWorker` — loop 20 Hz em worker_thread | completed | high | task_01, task_02 |
| 04 | `SimulationManager` — ciclo de vida, `sim_id`, TTL, limite | completed | high | task_03 |
| 05 | `WsGateway` + `MessageRouter` + rota `/ws` | completed | high | task_02, task_04 |
| 06 | `SimulationSocket` (web) — WS client, reconexão, `localStorage` | completed | medium | task_02, task_05 |
| 07 | Religar `simulationStore` ao `SimulationSocket` | pending | medium | task_06 |
| 08 | Remoção do motor no cliente (worker, mock, public/wasm, tipos) | pending | medium | task_07 |
| 09 | CI: build WASM (emsdk) + cache antes dos testes | completed | medium | task_01 |
</content>
