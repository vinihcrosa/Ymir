---
provider: manual
pr: 2
round: 1
round_created_at: 2026-07-05T14:57:07Z
status: resolved
file: apps/api/src/simulation/simulation-manager.ts
line: 44
severity: medium
author: claude-code
provider_ref:
---

# Issue 001: Worker thread errors/exits are not surfaced or cleaned up

## Review Comment

`defaultWorkerFactory` wires only `worker.on('message', ...)`; the manager never
listens for the worker_thread's `error` or `exit` events. If a simulation
worker throws during boot (e.g. the WASM runtime fails to load) or crashes
mid-run, the failure is silent: connected clients receive no `Error`
ServerMessage, and the `SimEntry` lingers in the registry with a dead worker
until its TTL expires (or forever while a client stays attached).

Suggested fix: extend `SimWorkerHandle` with an error channel and have the
manager fan out a synthetic error event + tear the sim down when the worker
errors/exits. For example in `defaultWorkerFactory`:

```ts
on: (event, listener) => worker.on(event, listener),
// plus, in create():
handle.on?.('error', (err) => {
  this.fanOut(simId, { type: 'error', message: String(err) })
  void this.stop(simId)
})
```

This closes the operational gap where a crashed engine leaves clients hanging.

## Triage

- Decision: `VALID`
- Notes:
Fixed: added `onError` to SimWorkerHandle; defaultWorkerFactory wires worker error/exit; manager fans out an Error event and stops the sim on failure. Test: "surfaces a worker error to clients and tears the sim down".
