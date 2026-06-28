# Claude — Ymir

See [AGENTS.md](AGENTS.md) for all project rules, architecture constraints, and coding standards.

## Quick orientation

- **Language**: C++17
- **Domain**: 6-DOF naval physics — ships, thrusters, rudders, waves, tugs
- **Reference docs**: `docs/legacy/` (legacy project — context only, do not copy)
- **No code exists yet** — we are building from scratch

## Working style

- Read `AGENTS.md` before writing any code
- Check `docs/legacy/` when you need to understand domain behavior or physics intent
- Do not port legacy code — understand the intent, implement it correctly

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
