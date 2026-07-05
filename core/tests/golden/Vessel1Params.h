#pragma once

// -----------------------------------------------------------------------------
// Vessel1Params — force-model configs for the golden validation vessel.
//
// Vessel: 3R_GUAMARE_VLCC_LOADED_L340B60T23 (VLCC, L=350 B=63 T=23), the vessel
// used by every reference scenario in the TMS Dynamics 3.0 golden dataset.
//
// UNIT CONVENTION (important): the source vessel1.json expresses inertia and
// stiffness in tonne-based units (mass in t, forces in kN, stiffness in kN/m).
// The golden CSV forces are in SI (N, N·m). Every mass/weight/stiffness value
// below is therefore the JSON value multiplied by kTonneToSI = 1000. This was
// verified empirically: with the ×1000 scaling Ymir's restoring Fr_z matches the
// golden to ~3e-5 relative error.
// -----------------------------------------------------------------------------

#include <ymir/physics/forces/DampingForces.h>
#include <ymir/physics/forces/RestoringForces.h>

namespace ymir::test
{

/// t -> kg, kN -> N, kN/m -> N/m. See file header.
inline constexpr double kTonneToSI = 1000.0;

/// RestoringForces config for vessel1 (source: dimensions.bouiancy in vessel1.json).
inline ymir::naval::RestoringConfig vessel1Restoring()
{
    ymir::naval::RestoringConfig c{};
    c.draft            = 23.0;
    c.mass             = 450150.0 * kTonneToSI;       // massMatrix[0][0]
    c.volumetricWeight = 4355971.5 * kTonneToSI;      // displacement.weight
    c.cg               = {0.691, 0.0, 19.2};          // mass.centerOfGravity
    c.cf               = {0.6909, 0.0, -7.09715};     // bouiancy.floatCenter

    // dimensions.bouiancy.hydrostaticRestoring (diagonal terms used by Ymir).
    c.hydro_rest[2][2] = 213018.0 * kTonneToSI;
    c.hydro_rest[3][3] = 36249900.0 * kTonneToSI;
    c.hydro_rest[4][4] = 1952870000.0 * kTonneToSI;
    return c;
}

/// DampingForces config for vessel1 (source: damping block in vessel1.json).
inline ymir::naval::DampingForces::Config vessel1Damping()
{
    ymir::naval::DampingForces::Config c{};
    c.linearDampingCoeff = 0.5;

    // damping.potential (6x6, radiation damping).
    const double pot[6][6] = {
        {1.96952, 0, -2.76792, 0, 823.503, 0},
        {0, 8.02247, 0, -51.0285, 0, 1.80281},
        {-1.47476, 0, 201088.9, 0, 2779191, 0},
        {0, -50.7984, 0, 3291989, 0, -11.0002},
        {836.414, 0, 2779092, 0, 1393379000, 0},
        {0, 2.05521, 0, -12.6653, 0, 20.7451},
    };
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            c.potential[i][j] = pot[i][j] * kTonneToSI;

    // damping.linear + damping.quadratic (only non-zero terms).
    c.linear[0][0]    = 3.0 * kTonneToSI;
    c.linear[3][3]    = 12854400.0 * kTonneToSI;
    c.quadratic[5][5] = 16446722225.0 * kTonneToSI;
    return c;
}

} // namespace ymir::test
