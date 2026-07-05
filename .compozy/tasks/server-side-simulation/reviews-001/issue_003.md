---
provider: manual
pr: 2
round: 1
round_created_at: 2026-07-05T14:57:07Z
status: resolved
file: apps/web/src/lib/simulation-socket.ts
line: 86
severity: low
author: claude-code
provider_ref:
---

# Issue 003: Reconnect has no backoff/cap and the command queue is unbounded

## Review Comment

On an unexpected close the socket reschedules `connect()` after a fixed
`reconnectDelayMs` (default 1000 ms) with no exponential backoff and no maximum
attempt cap. If the server is down, the client hammers it every second forever.
Separately, `send()` pushes to `this.queue` whenever the socket is not OPEN
(line 101) with no bound, so a long offline period accumulates an unbounded
backlog that all flushes at once on reconnect.

Suggested fix: apply capped exponential backoff (e.g. `min(maxDelay, base * 2^n)`
with jitter) and reset it on a successful open; bound the queue (drop-oldest or
cap length) so a disconnected client cannot grow memory without limit. Neither
is catchable by tests today because tests drive a single open.

## Triage

- Decision: `VALID`
- Notes:
Fixed: capped exponential backoff (reconnectDelayMs*2^n, reset on open, maxReconnectDelayMs cap) + bounded queue (maxQueueSize, drop-oldest). Tests: "grows the reconnect delay...", "caps the offline command queue...".
