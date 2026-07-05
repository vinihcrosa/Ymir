---
provider: manual
pr: 2
round: 1
round_created_at: 2026-07-05T14:57:07Z
status: resolved
file: apps/api/src/simulation/message-router.ts
line: 119
severity: medium
author: claude-code
provider_ref:
---

# Issue 002: AttachSimulation delivers no immediate snapshot on reconnect

## Review Comment

The PRD/ADR-006 promise that a reopened client "reconnects and resumes the
current snapshot". On `AttachSimulation`, the router calls
`manager.attach(simId, client)` and binds the connection, but sends nothing
back — no `SimulationCreated`/`Status`/`State`. A reconnecting client therefore
sees a blank scene until the next server tick. For a **running** sim that gap is
~50 ms (tolerable), but for a **paused** sim no tick ever fires, so the client
stays blank indefinitely.

Suggested fix: on successful attach, reply with the current status and the last
snapshot. The manager can cache the most recent `WorkerSnapshot`/`state` per sim
(update it in `fanOut`) and the router can push it to the newly attached client,
plus a `Status` frame reflecting running/paused. This makes reconnect
deterministic regardless of run state.

## Triage

- Decision: `VALID`
- Notes:
Fixed: SimEntry now caches lastStatus/lastState (updated in fanOut); attach() replays them to the (re)attaching client. Test: "replays the latest status and snapshot to a client that attaches later".
