---
provider: manual
pr: 2
round: 1
round_created_at: 2026-07-05T14:57:07Z
status: resolved
file: apps/api/src/simulation/simulation-runner.ts
line: 49
severity: low
author: claude-code
provider_ref:
---

# Issue 005: loadScenario frees the engine before the new one is built

## Review Comment

`loadScenario` does `this.engine.delete()` (line 49) and then
`this.engine = await this.engineFactory()` (line 50). If the factory rejects
(module load failure, OOM), the old engine is already freed and `this.engine`
still points at a deleted Embind handle. Any subsequent `tick`/`getState`/
`dispose` then throws "instance already deleted", and the error is not
recoverable for that runner.

Suggested fix: build the replacement first, then swap and delete the old one:

```ts
const next = await this.engineFactory()
this.engine.delete()
this.engine = next
```

This keeps the runner in a valid state if engine creation fails.

## Triage

- Decision: `VALID`
- Notes:
Fixed: loadScenario builds the replacement engine before deleting the old one, keeping the runner valid on factory failure.
