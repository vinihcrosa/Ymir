#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <ymir/core/Types.h>

namespace ymir::naval
{

// ---------------------------------------------------------------------------
// Dimensions
// ---------------------------------------------------------------------------

struct DimensionsConfig
{
    double length_BP        = 0.0;   // m
    double beam             = 0.0;   // m
    double draft            = 0.0;   // m
    double height           = 0.0;   // m — total above keel
    double blockCoefficient = 0.75;
    double volumetricWeight = 0.0;   // N  (= displacement × g)
    double mass             = 0.0;   // kg (= volumetricWeight / g)

    std::array<double, 3> cg{};                // m — centre of gravity from WAMIT origin
    std::array<double, 3> cf{};                // m — centre of flotation from WAMIT origin
    std::array<double, 3> wavesOriginPosition{}; // m — WAMIT reference origin
};

// ---------------------------------------------------------------------------
// Hydrostatics
// ---------------------------------------------------------------------------

struct HydrostaticConfig
{
    Matrix6x6 hydro_rest{};  // N/m or N·m/rad; only diagonal used
};

// ---------------------------------------------------------------------------
// Damping
// ---------------------------------------------------------------------------

struct DampingConfig
{
    Matrix6x6 potential{};           // radiation damping (N·s/m)
    Matrix6x6 linear{};              // viscous linear
    Matrix6x6 quadratic{};           // Morison quadratic (N·s²/m²)
    double    linearDampingCoeff = 0.1;  // exponential modulation factor
};

// ---------------------------------------------------------------------------
// Current
// ---------------------------------------------------------------------------

enum class CurrentModel { OBOKATA, REGULAR };

struct CurrentConfig
{
    CurrentModel model    = CurrentModel::OBOKATA;
    int          n_sections = 50;

    // Section longitudinal positions in body frame [m] — pre-built by NavalSimulation
    // from [-L/2 .. +L/2]; populated if empty at construction.
    std::vector<double> sectionLocalPositions;

    double frontalArea     = 0.0;  // m² submerged frontal
    double lateralArea     = 0.0;  // m² submerged lateral
    double frontalHeight   = 0.0;  // m — centroid z relative to WAMIT origin
    double lateralHeight   = 0.0;  // m
    double midshipDistance = 0.0;  // m — midship x relative to WAMIT origin

    // Drag coefficient table: each entry = {angle_deg, Cx, Cy, Cz}
    std::vector<std::array<double, 4>> cdcTable;

    // Van der Pol VIM parameters (used only with REGULAR model)
    double st   = 0.2;   // Strouhal number
    double ez   = 0.3;   // VDP damping coeff
    double az   = 0.1;   // VDP forcing coeff
    double clO  = 1.0;   // lift coefficient multiplier
};

// ---------------------------------------------------------------------------
// Wind
// ---------------------------------------------------------------------------

enum class WindModel { REGULAR, ACSINKAGE };

struct WindConfig
{
    WindModel model         = WindModel::REGULAR;
    double    frontalArea   = 0.0;  // m² above waterline
    double    lateralArea   = 0.0;
    double    frontalHeight = 0.0;  // m — centroid z of frontal area from WAMIT origin
    double    lateralHeight = 0.0;
    double    midshipDistance = 0.0;

    // Drag coefficient table: each entry = {angle_deg, Cx, Cy, Cz}
    std::vector<std::array<double, 4>> cdwdTable;
};

// ---------------------------------------------------------------------------
// Waves (WAMIT tables)
// ---------------------------------------------------------------------------

enum class SpectrumType { JONSWAP, PIERSON, REGULAR };

struct WavesConfig
{
    std::vector<double> omega;   // rad/s — frequency array for WAMIT tables
    std::vector<double> angles;  // deg   — incidence angle array for WAMIT tables

    // [6_dof][N_omega][N_angle]
    std::vector<std::vector<std::vector<double>>> wvForcesAmplitude;
    std::vector<std::vector<std::vector<double>>> wvForcesPhase;
    std::vector<std::vector<std::vector<double>>> raoAmplitude;
    std::vector<std::vector<std::vector<double>>> raoPhase;
    std::vector<std::vector<std::vector<double>>> mdForceAmplitude;
    std::vector<std::vector<std::vector<double>>> mdForcePhase;

    uint32_t     seed          = 0;     // 0 = deterministic reference
    double       gamma         = 3.3;   // JONSWAP peak enhancement factor
    double       alfa          = 0.0;   // Phillips parameter (0 = auto from Hs)
    double       s_max         = 10.0;  // Mitsuyasu exponent
    bool         independentDirPhase = false;
};

// ---------------------------------------------------------------------------
// Thruster
// ---------------------------------------------------------------------------

struct ThrusterConfig
{
    int    id                     = 0;
    std::array<double, 4> position{};  // {x, y, z, azimuth_rad} from WAMIT origin

    double diameter               = 1.0;   // m
    double rotationSpeedNominal   = 100.0; // RPM
    double rotationSpeedMax       = 150.0; // RPM
    double pitchRatio             = 1.0;
    double rotationTime           = 50.0;  // s — 1st-order time constant
    double azimuthSpeed           = 0.087; // rad/s (~5 deg/s)
    double maximumPowerW          = 5e6;   // W
    double hullEfficiency         = 0.95;
    double asternEfficiency       = 0.50;
    double transversalSpeedLimit  = 2.0;   // m/s
    double transversalReductionCoeff = 0.5;
    std::array<double, 3> paddleCoeffs{};  // {c0, c1, c2}

    // Open-water curve: each entry = {J, Kq, Kt}
    std::vector<std::array<double, 3>> openWaterCurve;
};

// ---------------------------------------------------------------------------
// Rudder
// ---------------------------------------------------------------------------

struct RudderConfig
{
    int    id                   = 0;
    std::array<double, 3> position{};  // {x, y, z} from WAMIT origin

    double area                 = 10.0;   // m²
    double angleMaximum         = 0.61;   // rad (~35 deg)
    double angleSpeed           = 0.052;  // rad/s (~3 deg/s)
    int    associatedThrusterId = -1;     // -1 if standalone
    double slipStreamFactor     = 0.7;

    // Foil table: each entry = {angle_deg, Cl, Cd}
    std::vector<std::array<double, 3>> coefficients;
};

// ---------------------------------------------------------------------------
// Tug
// ---------------------------------------------------------------------------

enum class TugMode { PUSH, PULL, ESCORTING };

struct TugConfig
{
    int    id         = 0;
    double bollardPull = 500000.0;  // N
    double sMax        = 4.0;       // m/s — max speed
    bool   conventional = true;     // false = Z-drive/azimuth
    double thrustTime  = 10.0;      // s — 1st-order time constant
    double dirSpeedMax = 15.0;      // deg/min

    std::array<double, 2> fairlead{};  // {x, y} — body frame attachment point (m)

    // fpush 1D: angle_deg → normalised force [0..1]
    std::vector<double> fpushAngles;
    std::vector<double> fpushForce;

    // fpull 2D: [angle_deg × speed_knots] → normalised force [0..1]
    std::vector<double> fpullAngles;
    std::vector<double> fpullSpeeds;
    std::vector<std::vector<double>> fpullForce;  // [N_angles][N_speeds]
};

// ---------------------------------------------------------------------------
// VesselConfig
// ---------------------------------------------------------------------------

struct VesselConfig
{
    DimensionsConfig   dims;
    HydrostaticConfig  hydrostatics;
    DampingConfig      damping;
    CurrentConfig      current;
    WindConfig         wind;
    WavesConfig        waves;

    std::vector<ThrusterConfig> thrusters;
    std::vector<RudderConfig>   rudders;
    std::vector<TugConfig>      tugs;
};

} // namespace ymir::naval
