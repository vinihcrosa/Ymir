---
provider: manual
pr: 2
round: 2
round_created_at: 2026-07-05T15:16:15Z
status: resolved
file: apps/web/src/stores/simulationStore.ts
line: 141
severity: medium
author: claude-code
provider_ref:
---

# Issue 001: reset() does not clear the persisted sim_id

## Review Comment

`reset()` sends `Reset`, closes the socket, and clears the in-memory store
state (`simId: null`), but it never calls `socket.clearSimId()`. The
`ymir.simId` entry therefore survives in `localStorage` (it is only cleared on
an "unknown simulation" error — simulation-socket.ts:162).

Consequence: the next `play()` builds a fresh `SimulationSocket`, whose `simId`
getter reads the stale id from `localStorage`. The store then takes the
"resume" branch — sends `Play{oldSimId}` instead of `CreateSimulation` — and the
new socket auto-attaches (on open) to the *old* simulation, which is still alive
server-side within its 30-min TTL. So a user who clicks Reset and then Play
reconnects to the previous run rather than starting a new one, contradicting the
store's own doc ("End the simulation ... and clear local state").

Suggested fix: clear the persisted id during reset, before dropping the socket:

```ts
reset() {
  const { socket, simId } = get()
  if (socket && simId) socket.send({ type: 'Reset', simId })
  socket?.clearSimId()
  socket?.close()
  pendingPlayDt = null
  set({ status: 'idle', /* ... */ simId: null, scenarioVessels: [] })
}
```

Add a store test asserting that after `reset()` a subsequent `play()` emits
`CreateSimulation` (not `Play`).

## Triage

- Decision: `VALID`
- Notes: Confirmed — `clearSimId()` was only called on the "unknown simulation" error, never on reset, so localStorage kept the id and the next play() re-attached to the old sim. Fixed: `reset()` now calls `socket.clearSimId()` before closing. Test added: "clears the persisted sim id so the next play starts a fresh simulation" asserts the next play() emits CreateSimulation, not AttachSimulation. Full suite green (248 web tests, turbo 7/7).
