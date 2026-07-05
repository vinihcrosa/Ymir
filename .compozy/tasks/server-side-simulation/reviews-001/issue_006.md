---
provider: manual
pr: 2
round: 1
round_created_at: 2026-07-05T14:57:07Z
status: resolved
file: apps/api/src/simulation/worker-protocol.ts
line: 27
severity: low
author: claude-code
provider_ref:
---

# Issue 006: Dead VesselConfig message path — defined but never emitted

## Review Comment

The `vesselConfig` WorkerEvent (worker-protocol.ts:27) and the `VesselConfig`
ServerMessage (packages/types/src/protocol.ts:65), plus its mapping in
`toServerMessage` (message-router.ts:19-20), form a complete path that is never
exercised: no code ever produces a `{ type: 'vesselConfig' }` event. The
SimulationRunner reads no vessel config, and the web client still fetches config
over REST (`GET /vessels/:id/config` in VesselPanel). This is the deferred item
noted in task_03 subtask 3.5.

Suggested fix: either (a) wire it — have the runner/worker emit `vesselConfig`
after `loadScenario` by reading `vessel-service.findConfig`, and consume it in
the store — or (b) remove the unused event/message variants until that feature
is actually built, to avoid dead contract surface. Prefer (b) for now since the
REST path already covers config.

## Triage

- Decision: `VALID`
- Notes:
Fixed (option b): removed the unused vesselConfig WorkerEvent, VesselConfig ServerMessage variant, router mapping, and store case. Config still served via REST.
