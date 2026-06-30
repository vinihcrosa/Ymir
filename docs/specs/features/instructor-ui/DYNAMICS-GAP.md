No mooring/fender/anchor/memory-function force models exist in Ymir core. I have enough to confirm the analysis. Let me verify how ThrustForces handles the propeller Kt/Kq table (MATLAB uses J-based interpolation) — already confirmed Ymir uses a linear `Kt = 0.4*PD - 0.1` approximation, no Kq/torque/power, no advance ratio J. I have all I need.

The MATLAB EOM uses `o.totalMass` (with off-diagonal added-mass coupling) and a full 12x12 rotation in the RK4 derivative evaluation; Ymir uses `invTotalMass_` (full 6x6 invert, good) but the kinematic transform is in `rhs` (correct approach), so that part is actually fine. Let me confirm the inertial Coriolis term sign matches and produce the report.

Comparison complete. Findings below.

---

# Ymir vs MATLAB Dynamics — Naval Physics Comparison

Scope: physics/force model only. Sources read in full: MATLAB `VesselFastTime.m` (EOM + force methods), `rudder.m`, `thruster.m`, `suction.m`, `Environment.m`, `funLivre.m`, `dynamics.m`; Ymir `core/libs/physics/src/forces/*`, `RigidBody6DOF.cpp`, `RK45Integrator.cpp`, `world/src/wave/WaveForces.cpp`.

## (1) Mapping table — MATLAB module → Ymir equivalent

| MATLAB force term (source) | Ymir equivalent | Status |
|---|---|---|
| `inertialForce` (VesselFastTime.m:571) Coriolis/centripetal | `InertialForces.cpp:14` | **Present** (faithful) |
| `currentObokata` Obokata section integration (VesselFastTime.m:588) | `CurrentForces.cpp:31` computeObokata | **Partial** (see D1) |
| Current REGULAR + VIM (no MATLAB equivalent found) | `CurrentForces.cpp:96` computeRegular + Van der Pol | **Extra in Ymir** (not in reference) |
| `dampingForces` potential+linear+quadratic (VesselFastTime.m:846) | `DampingForces.cpp:12` | **Present** (faithful) |
| `squatForce` ICORELS (VesselFastTime.m:859) | `SquatForces.cpp:21` | **Present** (Cs table differs slightly, D5) |
| `windForce` (VesselFastTime.m:892) | `WindForces.cpp:14` | **Present** (faithful) |
| `hydrostaticRestoringForce` (VesselFastTime.m:952) | `RestoringForces.cpp:24` | **Present** (faithful) |
| `waveForce` — 1st-order excitation RAO (VesselFastTime.m:792) | `WaveForces.cpp:25` | **Partial** (D2) |
| `waveForce` — mean drift (meanDrift, VesselFastTime.m:778) | — | **Missing** (D2) |
| `waveForce` — slow drift (slowDrift, VesselFastTime.m:782) | — | **Missing** (D2) |
| `waveForce` — wave drift damping (VesselFastTime.m:805) | — | **Missing** (D2) |
| `waveForce` — RAO of motion (raoMovement) | — | **Missing** (used by MATLAB for visual/coupling) |
| `thruster.advanceStep` Kt/Kq table, J, torque, power, paddle, astern eff. (thruster.m:152) | `ThrustForces.cpp:36` | **Partial** (D3 — major) |
| `rudder.advanceStep` Cl/Cd table + slipstream p1/p2 (rudder.m:65) | `RudderForces.cpp:26` | **Partial** (D4 — major) |
| `suction.advanceStep` ship-to-ship / bank suction (suction.m:271) | — | **Missing** (D6) |
| Mooring lines `Cabo.m` / `funLivre.m` catenary | — | **Missing** (D6) |
| Fenders `fender.m` | — | **Missing** (D6) |
| Anchor `Anchor.m` | — | **Missing** (D6) |
| Tug `Tug.m` (parametric pull/push tables) | `TugForces.cpp:15` | **Present** (simplified) |
| Memory-function (convolution) 1st-order radiation model (`waveCoefficients` MemoryFunction branch, Vessel.m:380) | — | **Missing** (D7) |
| 6-DOF EOM `mass\forces` + RK4 w/ rotation (VesselFastTime.m:537,548) | `RigidBody6DOF.cpp:57` + `RK45Integrator.cpp` | **Present** (different integrator, D8) |

## (2) Physics discrepancies / missing force terms

**D1 — Obokata current (partial).** MATLAB integrates per-section with *per-section* current sampled from the field at each section's global position (VesselFastTime.m:616-619) and adds a yaw term with `cd(3)` cross-flow plus the `midshipDistance` arm (VesselFastTime.m:647-650). Ymir (`CurrentForces.cpp:44-89`) assumes uniform current across sections (`water_x=-vc_x` constant), omits the `cdz` cross-flow yaw augmentation and the roll/pitch arms (f[3],f[4]) that MATLAB applies via `frontalHeight`/`lateralHeight` (VesselFastTime.m:652-658). Ymir sets only f[0],f[1],f[5].

**D2 — Wave second-order forces missing (significant).** MATLAB sums 5 wave contributions: excitation, RAO-motion, mean drift, slow drift, wave-drift-damping (VesselFastTime.m:519-523) and applies excitation masked by `[0 0 1 1 1 0]` (VesselFastTime.m:531 — surge/sway/yaw excitation zeroed, only heave/roll/pitch kept). Ymir `WaveForces.cpp:73` computes only 1st-order RAO excitation on all 6 DOF with no DOF mask, and has **no mean drift, no slow drift, no wave-drift damping**. For maneuvering/station-keeping these 2nd-order terms dominate; this is the largest environmental gap.

**D3 — Thruster/propeller (major).** MATLAB uses a real Kt/Kq curve interpolated on advance ratio J = Va/(pitch·n·D) (thruster.m:335-354,267), computes torque, power, power-saturation loop (thruster.m:243-250), first-order RPM lag filter (thruster.m:263-264), transverse-speed reduction factor `reduc` (thruster.m:270-273), astern efficiency (thruster.m:281-283), and paddle/side-wash force (thruster.m:284-297). Ymir `ThrustForces.cpp:55` replaces all of this with `Kt = 0.4*P/D − 0.1` clamped to [0.05,0.5], with **no J dependence, no Kq/torque/power, no RPM lag, no transverse reduction, no astern efficiency, no paddle effect**. Thrust magnitude will diverge from reference especially at speed and astern.

**D4 — Rudder (major).** MATLAB interpolates Cl/Cd from a table vs incidence angle β computed from actual inflow including current (rudder.m:133-144), and models propeller slipstream with `p1` geometric factor and `p2` actuator term (rudder.m:54,119-121). Ymir `RudderForces.cpp:54-57` uses analytic thin-airfoil `CL=2π·AR/(AR+2)·α` and induced drag `CL²/(πAR)` — **ignores the coefficient table entirely** and uses a simplified actuator-disk wake (`RudderForces.cpp:46`) without the `p1`/`hullEfficiency` factors. Also `RudderForces.cpp:70` has `r.position[1]*0.0` (roll arm from fx dropped) — matches MATLAB which also zeros those, acceptable.

**D5 — Squat Cs table.** MATLAB (VesselFastTime.m:864-872) ordering: `Cb>1→Cb`, `<0.7→1.7`, `>=0.8→2.4`, else 2.0. Ymir `SquatForces.cpp:15-18` drops the `Cb>1→Cb` branch and uses `<0.7→1.7, <0.8→2.0, else 2.4`. Minor, but the `Cb>1` passthrough is absent.

**D6 — Coupling/contact forces fully missing.** No ship-to-ship/bank **suction** (`suction.m`), **mooring lines** (`Cabo.m`+`funLivre.m` catenary), **fenders** (`fender.m`), or **anchor** (`Anchor.m`) force models exist in Ymir (confirmed: no source matches under `core/libs`, only vendored JSON lib hits). MATLAB sums these at VesselFastTime.m:533 (`lines`, `fenders`, `suctions`). TugForces exists but is simplified vs `Tug.m`.

**D7 — First-order radiation memory-function model missing.** MATLAB supports a convolution/retardation ("MemoryFunction") added-mass+radiation model (Vessel.m:380). Ymir uses only constant added mass (`RigidBody6DOF.cpp:17`) + frequency-independent potential damping. Acceptable if reference runs in non-memory mode, but the capability is absent.

**D8 — Integrator/EOM.** MATLAB: fixed-step RK4 with a 12×12 rotation applied to the *state* between stages (VesselFastTime.m:552-562) and `eq(1:6)=Y(7:12)` (body-frame velocities copied directly to position rates — note MATLAB does *not* rotate velocity→position inside `dynamicEquation`, the `rot` is applied to the increment). Ymir uses adaptive DOPRI5 and correctly rotates body→inertial in `rhs` (`RK45Integrator.cpp:113-119`). The numerical schemes differ; results will not be bit-identical but the kinematic transform in Ymir is arguably more correct. Mass handling matches (full 6×6 `totalMass`, `RigidBody6DOF.cpp:17-18`).

## (3) File:line pointers (Ymir) per force term

- Inertial/Coriolis: `core/libs/physics/src/forces/InertialForces.cpp:14-37`
- Current (Obokata): `core/libs/physics/src/forces/CurrentForces.cpp:31-91`; (Regular+VIM, extra): `:96-148`
- Damping (pot/lin/quad): `core/libs/physics/src/forces/DampingForces.cpp:12-46`
- Squat: `core/libs/physics/src/forces/SquatForces.cpp:21-55`
- Wind: `core/libs/physics/src/forces/WindForces.cpp:14-58`
- Hydrostatic restoring: `core/libs/physics/src/forces/RestoringForces.cpp:24-59`
- Wave (1st-order RAO only): `core/libs/world/src/wave/WaveForces.cpp:25-78`
- Thrust/propeller: `core/libs/physics/src/forces/ThrustForces.cpp:36-79` (Kt approx `:55`)
- Rudder: `core/libs/physics/src/forces/RudderForces.cpp:26-76` (CL/CD `:54-57`, slipstream `:41-48`)
- Tug: `core/libs/physics/src/forces/TugForces.cpp:15-60`
- EOM assembly / mass: `core/libs/physics/src/RigidBody6DOF.cpp:8-60`; force summation `core/libs/physics/src/integrator/RK45Integrator.cpp:104-123`
- Suction / mooring lines / fenders / anchor / memory-function: **no files exist** (force header dir has only the 9 listed in CurrentForces.h…WindForces.h).

## Progress / status

- **D5 (squat Cs table)** — ✅ fixed (`SquatForces.cpp`), parity test added. Commit `186b24c`.
- **D1 (Obokata current)** — ⏳ partial: roll/pitch arm moments `f[3]`/`f[4]` added per MATLAB 652-658 (`CurrentForces.cpp`), test added, commit `7a5092e`. Remaining: per-section spatial current-field sampling, submerged-depth area basis `(wavesOriginPosition_z − z)·(L/N)`, and the `cdz` cross-flow yaw term (647-650) — these depend on the `wavesOriginPosition(3)` sign/draft convention and need validation against MATLAB reference output before porting.
- **D3 (thruster Kt/Kq+J)** — handed to a dedicated worktree session (chip started).
- **D4 (rudder Cl/Cd table + slipstream)** — handed to a dedicated worktree session (chip started).
- **D2 (2nd-order wave forces + DOF mask)** — handed to a dedicated worktree session (chip started).
- **D6/D7 (suction, mooring lines, fenders, anchor, radiation memory-function)** — deferred; gated on UI features (anchors/towlines) that are themselves deferred.

Note: D1's remaining terms, D2, D3 and D4 are faithful-reimplementation tasks that must be checked against MATLAB numeric reference runs; they are not driven by the JS test suite.

## Bottom line
Faithful: inertial, damping, wind, hydrostatic restoring. Partial/diverging: current-Obokata (D1), wave (1st-order only, D2), thruster (D3), rudder (D4), squat (D5). Missing entirely: wave mean/slow drift + wave-drift damping (D2), ship-to-ship/bank suction, mooring lines, fenders, anchor (D6), radiation memory-function (D7). The highest-impact gaps for matching the reference are **D3 (propeller Kt/Kq+J), D4 (rudder coefficient table+slipstream), and D2 (2nd-order wave forces)**.