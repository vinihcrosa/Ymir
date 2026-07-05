# Golden fixtures — TMS Dynamics 3.0 cross-engine validation

Downsampled reference data used by `ymir_golden_tests`
(`core/tests/golden/`). Source: the TMS Dynamics 3.0 golden dataset
(`vessel-drift-test-data/reference/`), vessel `vessel1`
(`3R_GUAMARE_VLCC_LOADED_L340B60T23`, VLCC L=350 B=63 T=23).

Each scenario isolates one force type. Rows are kept every 25th step
(0.1 s dt → 2.5 s spacing) to keep the fixtures small; the full-resolution
CSVs live in the source dataset.

## Files per scenario

| File | Columns |
|------|---------|
| `state.csv` | `Time` + 6-DOF position (`surge,sway,heave,roll,pitch,yaw`) + 6-DOF velocity (`vel_x..vel_zz`) |
| `loads.csv` | `Time` + per-force golden loads for that scenario |

The tests feed each `state.csv` row into the matching Ymir force model and
compare the output against `loads.csv` — integrator-independent, so it
validates the force *formula*, not the time stepping.

## Units

Forces/moments in the golden CSVs are SI (N, N·m). The source `vessel1.json`
expresses inertia/stiffness in tonne-based units; the test configs in
`Vessel1Params.h` scale those by `kTonneToSI = 1000`.

## Scenarios present

- `08_restoring` — hydrostatic restoring (`Fr`) + damping (`Fd`); heave offset
  +0.5 m, environment zeroed.
