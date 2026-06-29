# PRD: Environmental Conditions

**Feature:** Environmental Conditions for Simulation Scenarios  
**Status:** Draft  
**Date:** 2026-06-29  
**Author:** Derived from [env-conditions.md](../../.discussions/env-conditions.md) and dynamics reference (`Environment.m`)

---

## Overview

Ymir simulations currently run with no environmental forces — vessels move only under propulsion and rudder. This feature gives instructors the ability to configure ocean current, wind, and wave conditions for a simulation scenario. The physics engine already models how vessels respond to these forces; this feature exposes that capability to the instructor through the UI.

The target user is a **simulation instructor** who is setting up a training scenario for a vessel operator. The instructor needs to replicate the environmental conditions of a specific port, waterway, or open-sea exercise — including conditions that change over time.

---

## Goals

- Enable instructors to add current, wind, and wave conditions to a scenario before starting a simulation.
- Support multiple simultaneous conditions of the same type that combine as vector sums (e.g., tidal current + river discharge current).
- Support temporal evolution: conditions that change over the course of a simulation session, defined as a sequence of keyframes with linear interpolation.
- Ensure that all environmental conditions in the UI use nautical degree convention (0 = North, clockwise), consistent with how mariners communicate direction.
- Establish the pattern and pipeline (data model → scenario serialization → WASM binding → physics) that Phase 2 (grid maps) and Phase 3 (real-time control) will extend.

---

## User Stories

### Instructor — Scenario Setup

- As an instructor, I want to add a current condition to a scenario with a direction and speed, so that simulated vessels experience hydrodynamic drag from water flow.
- As an instructor, I want to add a wind condition to a scenario, so that vessels with large superstructures (e.g., VLCCs) experience aerodynamic forces.
- As an instructor, I want to add a wave condition to a scenario with height, period, and direction, so that vessels experience wave excitation and drift forces.
- As an instructor, I want to add multiple current conditions that combine vectorially, so that I can represent overlapping current systems (e.g., tidal + river discharge at a port entrance).
- As an instructor, I want to configure a current that changes over time using keyframes, so that I can simulate tidal cycles during a long berthing exercise.
- As an instructor, I want to configure wind that intensifies over the course of a scenario (e.g., storm buildup), so that trainees experience progressively challenging conditions.
- As an instructor, I want to remove an environmental condition from a scenario, so that I can adjust the setup without starting from scratch.
- As an instructor, I want to see all configured conditions for a scenario in one place, so that I can review the environmental setup before starting the simulation.

### Instructor — During Simulation (Phase 3 only)

- As an instructor, I want to modify environmental conditions while a simulation is running, so that I can introduce unexpected events (e.g., sudden wind shift) to test trainee responses.

---

## Core Features

### F1 — Condition Management

Instructors can add, configure, and remove environmental conditions for a scenario. Each condition belongs to one of three types: **current**, **wind**, or **wave**. Multiple conditions of the same type are allowed and their effects compose (see F3).

A condition has:
- **Type**: current | wind | wave
- **Format**: uniform (Phase 1) | grid map (Phase 2)
- **Temporal definition**: single snapshot, or a sequence of keyframes (see F4)

### F2 — Uniform Condition Input

A uniform condition applies a single direction and speed across the entire simulation area.

**Current and Wind:**
- Direction: nautical degrees (0–360, where the condition comes *from*; 90 = from East)
- Speed: m/s (non-negative)

**Wave (uniform):**
- Direction: nautical degrees (from where the waves come)
- Significant wave height (Hs): meters
- Peak period (Tp): seconds
- Spectrum type: JONSWAP | PIERSON | REGULAR
- Peak factor (gamma): applicable for JONSWAP (default 3.3; 0 = auto-calculate)

All direction values displayed and entered in nautical degrees. An anti-corruption layer converts to the physics engine's internal convention (mathematical, 0 = East, counter-clockwise) at the boundary.

### F3 — Vectorial Composition

When multiple conditions of the same type exist in a scenario, their effects combine as vector sums at each simulation step.

- **Uniform + uniform**: the direction/speed vectors are added component-wise.
- **Grid + grid** (Phase 2): each grid is evaluated independently at the vessel's position using bilinear interpolation, and the resulting vectors are summed.
- **Uniform + grid**: not permitted in this version. Collections of the same type must be homogeneous (all uniform or all grid-mapped).

Example: current A at 1.0 m/s from North (compass 0°) plus current B at 0.5 m/s from South (compass 180°) produces a resultant of 0.5 m/s from North.

### F4 — Temporal Keyframes

Any condition (uniform or grid) can be assigned a simulation timestamp. A collection of conditions of the same type ordered by timestamp defines a temporal evolution:

- At simulation time `t`, the active value is linearly interpolated between the two keyframes that bracket `t`.
- Before the first keyframe: first keyframe value applies.
- After the last keyframe: last keyframe value applies.
- A collection with a single keyframe (or all keyframes with the same timestamp) is a static condition — no temporal evolution.

Interpolation is shortest-path for angles (avoids wrapping through 180°). Speed and spectral parameters interpolate linearly.

This matches the behavior implemented in `Environment.advanceStep()` and `currentValue()` in the `dynamics` MATLAB reference.

### F5 — Scenario Persistence

Environmental conditions are part of the scenario data and are saved and loaded with it. An instructor can set up conditions once and reuse the scenario across multiple training sessions.

### F6 — Grid Map Condition Input (Phase 2)

An instructor can import a spatial grid file to define a current or wave field that varies across the simulation area.

**Grid format (current map):**
- `gridCoordinatesX[nx]` — X coordinates of grid nodes (meters, scenario-local frame)
- `gridCoordinatesY[ny]` — Y coordinates of grid nodes (meters, scenario-local frame)
- `speedX[ny][nx]` — X component of velocity (m/s) at each node
- `speedY[ny][nx]` — Y component of velocity (m/s) at each node

**Grid format (wave map):**
- Same spatial grid structure
- `heightMultiplier[ny][nx]` — multiplier applied to the base Hs at each node (0.0–1.0+ range)
- `direction[ny][nx]` — wave direction (nautical degrees) at each node

Vessel positions outside the grid's extent receive zero environmental contribution from that grid (no edge extrapolation).

Reference data format: `current.json` (Baía de Guanabara dataset).

---

## User Experience

### Primary Flow — Adding an Environmental Condition (Phase 1)

1. Instructor opens the scenario editor.
2. Instructor navigates to the "Environmental Conditions" section.
3. Instructor clicks "Add Condition" and selects a type (current, wind, or wave).
4. A form appears with inputs for direction (nautical degrees) and speed (m/s), plus wave-specific fields if applicable.
5. Instructor fills in the values and confirms.
6. The condition appears in the list for that type.
7. Instructor can add a second condition of the same type; the list shows both with a note indicating they compose vectorially.
8. To configure temporal evolution, the instructor assigns a simulation time to each condition. The list re-orders by time and shows the interpolation will occur between them.

### Primary Flow — Grid Map Import (Phase 2)

1. Instructor selects "Grid Map" format for a current or wave condition.
2. Instructor uploads a `.json` file conforming to the grid format.
3. The UI confirms file dimensions and coordinate extent.
4. The grid appears as a layer in the scenario map view, showing velocity vectors or wave height as a heatmap.

### Direction Input Convention

All direction inputs accept and display **nautical degrees** (0 = North, clockwise, "from where"). The input label reads "Direction (°)" with a note "0 = North". A compass rose widget provides visual feedback.

### Condition List Layout

Conditions are grouped by type (Current, Wind, Waves). Within each group, conditions are listed in chronological order by their keyframe timestamp. Each list item shows direction, speed, and timestamp. A single condition with no timestamp is labeled "Static".

---

## High-Level Technical Constraints

- Environmental conditions must not be modifiable while the simulation step loop is executing. Changes made through the UI take effect between steps, not during a step. (Real-time instructor control during simulation is Phase 3.)
- The physics behavior for wind force, current drag, and wave excitation/drift must match the `dynamics` MATLAB reference implementation (force models: Obokata current, WindForces aerodynamic model, WAMIT-based wave excitation).
- All direction values stored in the scenario data must use nautical degrees. Conversion to the physics engine's internal convention (mathematical degrees) happens at the system boundary, not in the data model.
- Wave spectrum computation (JONSWAP, PIERSON, REGULAR) must produce the same spectral parameters as `Environment.waveSpectrum()` in the dynamics reference.

---

## Non-Goals (Out of Scope)

- **Wind maps**: wind is uniform-only in this version. No spatial grid for wind.
- **Mixing uniform and grid conditions in the same collection**: not permitted in this version.
- **Real-time condition changes during simulation**: Phase 3 only. Phase 1 conditions are set before the simulation starts.
- **Tide management**: tide is a separate concern and is not part of this feature.
- **Bathymetric depth maps**: water depth is a single scalar per scenario.
- **Wind turbulence spectrum** (API/NPD): the dynamics reference implements a wind speed fluctuation model (`windSpectrum()`); this is not exposed in Phase 1 — wind speed per keyframe is constant.
- **Slow drift and drift damping coefficients**: these are vessel properties, not environmental inputs.
- **Wave spreading function (Mitsuyasu cos-2S)**: directional spreading is applied inside the physics engine; instructors only set bulk wave parameters (Hs, Tp, direction, spectrum type).
- **Scattered (non-structured) interpolants** for current grids: only regular rectangular grids are supported (matching the `current.json` format).

---

## Phased Rollout Plan

### Phase 1 — Uniform Conditions with Temporal Evolution (MVP)

**Scope:**
- Add/remove uniform current, wind, and wave conditions to a scenario.
- Configure direction, speed, and wave spectral parameters.
- Assign keyframe timestamps for temporal evolution (linear interpolation).
- Multiple conditions of the same type compose vectorially.
- Scenario persistence (save/load).

**Success criteria:**
- An instructor can create a scenario with a 1-knot current from East and a 10 m/s wind from North, start the simulation, and observe the VLCC vessel drifting in the expected direction.
- An instructor can define a tidal cycle (current keyframes at t=0, t=900, t=1800) and observe the vessel response changing over the simulation.
- Conditions survive a save/load cycle without data loss.

### Phase 2 — Grid-Mapped Conditions

**Scope:**
- Grid map format for current and wave conditions.
- File import from `.json` (current format) and equivalent wave grid format.
- Map layer visualization (velocity vectors or heatmap) in the scenario editor.
- Grid conditions support the same temporal keyframe model as uniform conditions.
- Multiple grid conditions of the same type compose vectorially (per ADR-003).

**Success criteria:**
- An instructor can import a real current map (e.g., Baía de Guanabara dataset) and observe vessels responding to spatially varying current as they navigate through the grid.
- A vessel exiting the grid area experiences zero current contribution from that grid.

### Phase 3 — Real-Time Instructor Control

**Scope:**
- Instructor can modify current, wind, and wave conditions while the simulation is running.
- Changes take effect at the next simulation step.
- UI provides immediate feedback that the change is active.

**Success criteria:**
- An instructor can introduce a sudden wind shift (direction change by 90°) mid-exercise and observe the vessel's heading controller responding within the expected physics timescale.

---

## Success Metrics

- **Simulation realism validation**: A VLCC simulation with a 1-knot beam current produces a measurable lateral drift rate consistent with the physics model (CurrentForces, Obokata sectional integration).
- **Wave excitation validation**: A scenario with Hs=2m, Tp=10s JONSWAP wave produces roll/pitch motion within expected RAO bounds for the configured vessel.
- **Data integrity**: 100% of environmental condition values survive a scenario save/load cycle without precision loss.
- **Instructor task time**: An instructor can configure a three-condition tidal cycle scenario from scratch in under 3 minutes.
- **Zero physics regression**: All existing simulation tests pass with the new environmental input pipeline in place.

---

## Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Direction convention bugs (nautical vs. mathematical) cross the ACL boundary | High | High | Enforce ACL as a dedicated conversion module with unit tests covering all four quadrants; never store or pass raw mathematical degrees to the UI |
| Keyframe interpolation for angles wraps incorrectly (350° → 10° goes through 180°) | Medium | Medium | Use shortest-path angular interpolation; validate with a test case that crosses 0°/360° |
| Instructors confused by "from where" direction convention | Medium | Low | Compass rose input widget with live vector arrow; tooltip on the input field |
| Phase 2 grid import format diverges from dynamics reference | Low | High | Document the exact JSON schema as a contract; validate against `current.json` as the canonical example |
| Wave spectral parameters produce different force magnitudes than dynamics reference | Low | High | Cross-validate a JONSWAP scenario against dynamics reference output before shipping Phase 1 |

---

## Architecture Decision Records

- [ADR-001: Phased Rollout — Uniform Conditions Before Grid Maps](adrs/adr-001.md) — Deliver uniform + keyframes in Phase 1; grid maps in Phase 2; real-time control in Phase 3.
- [ADR-002: Homogeneous Condition Collections](adrs/adr-002.md) — Uniform and grid conditions cannot be mixed in the same collection; architecture must allow future extension.
- [ADR-003: Independent Evaluation + Vector Sum for Multiple Grid Maps](adrs/adr-003.md) — Each grid is evaluated at the vessel position independently; results summed; outside grid = zero.
- [ADR-004: Temporal Keyframe Evolution for All Condition Types](adrs/adr-004.md) — Current, wind, and wave all use the same keyframe interpolation model from the dynamics reference.

---

## Open Questions

- **Wave map Hs field**: the Phase 2 grid format uses `heightMultiplier` (relative to a base Hs) rather than absolute Hs per cell. This matches the `dynamics` reference (`fieldMultiplier`). Confirm whether the import tooling and UI should expose base Hs + multiplier grid, or allow absolute Hs per cell.
- **Figma designs**: the instructor UI wireframes at the Figma links in `env-conditions.md` were not accessible during PRD drafting. UX section above is based on domain knowledge; validate against the final Figma designs before implementation.
