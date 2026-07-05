---
provider: manual
pr: 2
round: 1
round_created_at: 2026-07-05T14:57:07Z
status: resolved
file: apps/web/src/lib/simulation-socket.ts
line: 25
severity: low
author: claude-code
provider_ref:
---

# Issue 004: Default WS URL hardcodes ws://localhost:3000 and ignores HTTPS

## Review Comment

`DEFAULT_URL` falls back to `ws://localhost:3000/ws` when `VITE_SIM_WS_URL` is
unset. Two problems for any non-local deployment: (1) the host/port is
hardcoded to localhost:3000, and (2) a page served over HTTPS must use `wss://`
or the browser blocks the mixed-content upgrade. Relying on the env var being
set everywhere is fragile.

Suggested fix: derive the default from the current page origin when available —
e.g. `${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}/ws` —
and keep `VITE_SIM_WS_URL` as an explicit override. This makes the client work
under TLS and behind reverse proxies without per-env configuration.

## Triage

- Decision: `VALID`
- Notes:
Fixed: defaultWsUrl() derives ws/wss from location.host, with VITE_SIM_WS_URL as explicit override. Test: "defaultWsUrl derives a ws/wss URL from the page origin".
