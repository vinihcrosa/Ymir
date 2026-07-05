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
#include <ymir/physics/forces/WindForces.h>

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

/// WindForces config for vessel1 (source: wind block in vessel1.json).
/// Wind loads are pure SI (areas m², dimensionless Cd) — no tonne scaling.
inline ymir::naval::WindForces::Config vessel1Wind()
{
    ymir::naval::WindForces::Config c{};
    c.model           = ymir::naval::WindModel::REGULAR;
    c.frontalArea     = 2405.0; // wind.area.frontal
    c.lateralArea     = 4164.0; // wind.area.lateral
    c.frontalHeight   = 32.5;   // wind.area.frontalHeight
    c.lateralHeight   = 45.0;   // wind.area.lateralHeight
    c.midshipDistance = 0.0;
    c.length_BP       = 350.0;
    c.beam            = 63.0;
    c.draft           = 23.0;

    // wind.coefficients: [angle_deg, cdx, cdy, cdz] rows, 0..360 step 10.
    c.angles = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140,
                150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260, 270,
                280, 290, 300, 310, 320, 330, 340, 350, 360};
    c.cdx = {0.75, 0.77, 0.74, 0.65, 0.515, 0.39, 0.29, 0.21, 0.135, 0.04, -0.07,
             -0.19, -0.33, -0.48, -0.615, -0.725, -0.825, -0.905, -0.955, -0.905,
             -0.825, -0.725, -0.615, -0.48, -0.33, -0.19, -0.07, 0.04, 0.135, 0.21,
             0.29, 0.39, 0.515, 0.65, 0.74, 0.77, 0.75};
    c.cdy = {0, 0.12, 0.28, 0.425, 0.54, 0.625, 0.675, 0.705, 0.715, 0.72, 0.705,
             0.68, 0.635, 0.555, 0.43, 0.31, 0.195, 0.09, 0, -0.09, -0.195, -0.31,
             -0.43, -0.555, -0.635, -0.68, -0.705, -0.72, -0.715, -0.705, -0.675,
             -0.625, -0.54, -0.425, -0.28, -0.12, 0};
    c.cdz = std::vector<double>(c.angles.size(), 0.0);
    return c;
}

} // namespace ymir::test
