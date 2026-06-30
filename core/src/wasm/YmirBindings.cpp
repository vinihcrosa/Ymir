/**
 * YmirBindings.cpp — Emscripten Embind bindings for the Ymir naval physics engine.
 *
 * Exposes a YmirSimulation class to JavaScript that wraps the World + NavalDomain
 * pipeline. State is returned via emscripten::val to avoid the need for additional
 * Embind value-object registrations for nested arrays.
 *
 * Physical parameters are taken directly from the VLCC reference vessel (vessel1.json):
 *   - LOA 350 m, B 63 m, T 23 m, displacement ~4.36 MN
 *   - Mass matrix, added mass, hydrostatic restoring, potential/linear/quadratic damping
 *   - Single CPP propeller: D=9.2 m, P/D=0.6145, max 81 RPM
 *   - Single balanced rudder: A=60 m², AR≈1.5, linked to thruster 0
 *
 * Usage from a Web Worker:
 *
 *   const sim = new Module.YmirSimulation();
 *   sim.addVesselAt(0, x, y, psi);
 *   sim.setThrusterCommand(0, 0, 80.0, 0.0);  // 80% full-ahead
 *   sim.step(0.1);
 *   const state = sim.getState();  // { t, vessels: [{id,x,y,z,phi,theta,psi,u,v,r},...] }
 *   sim.delete();
 */

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <ymir/simulation/NavalDomain.h>
#include <ymir/world/World.h>
#include <ymir/world/Environment.h>
#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/physics/BodyState.h>
#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/physics/forces/RudderForces.h>
#include <ymir/physics/forces/DampingForces.h>
#include <ymir/physics/forces/RestoringForces.h>
#include <ymir/physics/forces/InertialForces.h>
#include <ymir/physics/forces/CurrentForces.h>
#include <ymir/world/wave/WaveForces.h>
#include <ymir/world/wave/WaveSpectrum.h>
#include "WaveRaoData.h"
#include <ymir/common/Types.h>

#include <cmath>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using emscripten::val;

// ---------------------------------------------------------------------------
// VLCC physical parameters — sourced from vessel1.json
// ---------------------------------------------------------------------------

// Pure body mass matrix (diagonal). vessel1.json dimensions.mass.massMatrix.
static ymir::Matrix6x6 vlccMassMatrix()
{
    ymir::Matrix6x6 M{};
    M[0][0] = 450150.0;           // surge  (kg)
    M[1][1] = 450150.0;           // sway   (kg)
    M[2][2] = 450150.0;           // heave  (kg)
    M[3][3] = 232980000.0;        // roll   (kg·m²)
    M[4][4] = 3505800000.0;       // pitch  (kg·m²)
    M[5][5] = 3738800000.0;       // yaw    (kg·m²)
    return M;
}

// Frequency-independent added mass matrix. vessel1.json dimensions.mass.addedMass.
static ymir::Matrix6x6 vlccAddedMass()
{
    ymir::Matrix6x6 A{};
    // Row 0 (surge)
    A[0][0] = 53272.7;    A[0][2] = 29425.9;   A[0][4] = 20994100.0;
    // Row 1 (sway)
    A[1][1] = 223849.0;   A[1][3] = -2762560.0; A[1][5] = 21202.0;
    // Row 2 (heave)
    A[2][0] = 29621.6;    A[2][2] = 3618741.0;  A[2][4] = 27637240.0;
    // Row 3 (roll)
    A[3][1] = -2744050.0; A[3][3] = 67394070.0; A[3][5] = 58850800.0;
    // Row 4 (pitch)
    A[4][0] = 20987100.0; A[4][2] = 27550340.0; A[4][4] = 9546841000.0;
    // Row 5 (yaw)
    A[5][1] = 25475.8;    A[5][3] = 58905000.0; A[5][5] = 2809140000.0;
    return A;
}

// ---------------------------------------------------------------------------
// Body factory — uses real VLCC inertia so the integrator sees correct DOF scaling
// ---------------------------------------------------------------------------

static std::unique_ptr<ymir::RigidBody6DOF> makeBodyAt(int id, double x, double y, double psi)
{
    ymir::Matrix6x6 mass  = vlccMassMatrix();
    ymir::Matrix6x6 added = vlccAddedMass();

    ymir::Vector6 q{};
    q[0] = x;   // surge position (m)
    q[1] = y;   // sway  position (m)
    q[5] = psi; // yaw heading   (rad)
    // q[2] (heave) is settled to floating equilibrium by NavalDomain::initialize()
    // via RestoringForces::applyStaticEquilibrium — no hardcoding needed here.
    ymir::Vector6 qdot{};  // at rest

    ymir::RK45Config cfg{};
    cfg.reltol   = 1e-4;
    cfg.abstol   = 1e-6;
    cfg.maxStep  = 0.005;  // 5ms max internal step: prevents divergence on max-RPM+rudder impulse
    cfg.maxSteps = 100000;

    return std::make_unique<ymir::RigidBody6DOF>(id, mass, added, q, qdot, cfg);
}

// ---------------------------------------------------------------------------
// Force-model factories — all parameters from vessel1.json
// ---------------------------------------------------------------------------

// Build a wave spectrum from scenario sea-state params (spectrum: 0=JONSWAP,
// 1=PIERSON, 2=REGULAR). Hs=0 yields a calm sea (~no excitation).
static ymir::naval::WaveSpectrum makeWaveSpectrum(double Hs, double Tp, double dirDeg, double gamma, int spectrum)
{
    ymir::naval::WaveSpectrum::Config c{};
    c.spectrumType = spectrum == 2 ? ymir::naval::WaveSpectrumType::REGULAR
                   : spectrum == 1 ? ymir::naval::WaveSpectrumType::PIERSON
                                   : ymir::naval::WaveSpectrumType::JONSWAP;
    c.significantWaveHeight = Hs;
    c.peakPeriod            = Tp > 0.0 ? Tp : 8.0;
    c.gamma                 = gamma > 0.0 ? gamma : 3.3;
    c.mainDirection_deg     = dirDeg;
    return ymir::naval::WaveSpectrum(c);
}

static ymir::naval::InertialConfig vlccInertialConfig()
{
    ymir::naval::InertialConfig cfg{};
    ymir::Matrix6x6 M = vlccMassMatrix();
    ymir::Matrix6x6 A = vlccAddedMass();
    // totalMass = massMatrix + addedMass (element-wise)
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            cfg.totalMass[i][j] = M[i][j] + A[i][j];
    cfg.addedMass = A;
    cfg.cg   = {0.691, 0.0, 19.2};
    cfg.mass = 450150.0;
    return cfg;
}

static ymir::naval::DampingForces::Config vlccDampingConfig()
{
    ymir::naval::DampingForces::Config cfg{};
    cfg.linearDampingCoeff = 0.5;

    // Potential damping (radiation). vessel1.json damping.potential.
    cfg.potential[0][0] = 1.96952;    cfg.potential[0][2] = -2.76792;   cfg.potential[0][4] = 823.503;
    cfg.potential[1][1] = 8.02247;    cfg.potential[1][3] = -51.0285;   cfg.potential[1][5] = 1.80281;
    cfg.potential[2][0] = -1.47476;   cfg.potential[2][2] = 201088.9;   cfg.potential[2][4] = 2779191.0;
    cfg.potential[3][1] = -50.7984;   cfg.potential[3][3] = 3291989.0;  cfg.potential[3][5] = -11.0002;
    cfg.potential[4][0] = 836.414;    cfg.potential[4][2] = 2779092.0;  cfg.potential[4][4] = 1393379000.0;
    cfg.potential[5][1] = 2.05521;    cfg.potential[5][3] = -12.6653;   cfg.potential[5][5] = 20.7451;

    // Linear (viscous). vessel1.json damping.linear.
    cfg.linear[0][0] = 3.0;
    cfg.linear[3][3] = 12854400.0;

    // Quadratic. vessel1.json damping.quadratic (only yaw diagonal non-zero).
    cfg.quadratic[5][5] = 16446722225.0;

    return cfg;
}

static ymir::naval::RestoringConfig vlccRestoringConfig()
{
    ymir::naval::RestoringConfig cfg{};
    // Hydrostatic restoring matrix. vessel1.json dimensions.bouiancy.hydrostaticRestoring.
    cfg.hydro_rest[2][2] = 213018.0;
    cfg.hydro_rest[3][3] = 36249900.0;
    cfg.hydro_rest[4][4] = 1952870000.0;
    cfg.wavesOriginPosition = {0.0, 0.0, 0.0};
    cfg.draft            = 23.0;
    cfg.mass             = 450150.0;
    cfg.volumetricWeight = 4355971.5;  // N  (vessel1.json bouiancy.displacement.weight)
    cfg.cg               = {0.691, 0.0, 19.2};
    cfg.cf               = {0.6909, 0.0, -7.09715};
    return cfg;
}

// Hull resistance — drag coefficients from vessel1.json current.coefficients.
// Obokata method integrates sectional drag along the hull.
// This is the primary surge/sway deceleration force when propulsion is cut.
static ymir::naval::CurrentForces::Config vlccCurrentConfig()
{
    ymir::naval::CurrentForces::Config cfg{};
    cfg.model         = ymir::naval::CurrentModel::OBOKATA;
    cfg.length_BP     = 350.0;
    cfg.beam          = 63.0;
    cfg.frontalHeight = 11.5;
    cfg.lateralHeight = 11.5;
    cfg.midshipDistance = 0.0;
    cfg.n_sections    = 50;
    cfg.wavesOriginPosition = {0.0, 0.0, 0.0};

    // vessel1.json current.coefficients: [angle_deg, Cdx, Cdy, Cdz]
    cfg.angles = {  0,  10,  20,  30,  40,  50,  60,  70,  80,  90,
                  100, 110, 120, 130, 140, 150, 160, 170, 180, 190,
                  200, 210, 220, 230, 240, 250, 260, 270, 280, 290,
                  300, 310, 320, 330, 340, 350, 360};

    cfg.cdx = { 0.0100,  0.0069, -0.0016, -0.0130, -0.0239, -0.0312, -0.0324, -0.0267, -0.0151,  0.0000,
                0.0151,  0.0267,  0.0324,  0.0312,  0.0239,  0.0130,  0.0016, -0.0069, -0.0100, -0.0069,
                0.0016,  0.0130,  0.0239,  0.0312,  0.0324,  0.0267,  0.0151,  0.0000, -0.0151, -0.0267,
               -0.0324, -0.0312, -0.0239, -0.0130, -0.0016,  0.0069,  0.0100};

    cfg.cdy = { 0.0000,  0.1223,  0.3055,  0.5298,  0.7697,  0.9966,  1.1826,  1.3041,  1.3452,  1.3000,
                1.3452,  1.3041,  1.1826,  0.9966,  0.7697,  0.5298,  0.3055,  0.1223,  0.0000, -0.1223,
               -0.3055, -0.5298, -0.7697, -0.9966, -1.1826, -1.3041, -1.3452, -1.3000, -1.3452, -1.3041,
               -1.1826, -0.9966, -0.7697, -0.5298, -0.3055, -0.1223,  0.0000};

    cfg.cdz = { 0.0000, -0.0271, -0.0537, -0.0770, -0.0941, -0.1027, -0.1010, -0.0882, -0.0654, -0.0351,
               -0.0027,  0.0263,  0.0484,  0.0616,  0.0652,  0.0594,  0.0455,  0.0250,  0.0000, -0.0250,
               -0.0455, -0.0594, -0.0652, -0.0616, -0.0484, -0.0263,  0.0027,  0.0351,  0.0654,  0.0882,
                0.1010,  0.1027,  0.0941,  0.0770,  0.0537,  0.0271,  0.0000};

    return cfg;
}

static ymir::naval::ThrustForces::Config vlccThrustConfig()
{
    ymir::naval::ThrustForces::Config cfg{};
    ymir::naval::ThrustForces::ThrusterConfig tc{};
    tc.position    = {-170.0, 0.0, 4.5};
    tc.diameter    = 9.2;
    tc.pitchRatio  = 0.6145;   // vessel1.json thruster[0].pitchDiameterRelation
    tc.nominalRPM  = 81.0;     // 1.35 rev/s × 60 = 81 RPM (rotationSpeedMax)
    tc.azimuth_deg = 0.0;
    tc.initialRPM  = 0.0;
    cfg.thrusters.push_back(tc);
    return cfg;
}

static ymir::naval::RudderForces::Config vlccRudderConfig()
{
    ymir::naval::RudderForces::Config cfg{};
    ymir::naval::RudderForces::RudderConfig rc{};
    rc.position         = {-171.0, 0.0, 6.0};
    rc.area             = 60.0;   // vessel1.json rudder[0].area (m²)
    rc.thrusterIdx      = 0;
    rc.thrusterDiameter = 9.2;    // matches vlccThrustConfig() propeller diameter
    rc.hullEfficiency   = 0.95;   // vessel1.json thruster[0].hullEfficiency

    // Geometric slipstream factor (rudder.m ctor): p1 = 0.5 + 0.5/(1 + 0.15·X/D),
    // X = planar rudder↔thruster spacing.
    const double thrusterX = -170.0, thrusterY = 0.0;
    const double X  = std::sqrt((thrusterX - rc.position[0]) * (thrusterX - rc.position[0])
                              + (thrusterY - rc.position[1]) * (thrusterY - rc.position[1]));
    rc.p1 = 0.5 + 0.5 / (1.0 + 0.15 * (X / rc.thrusterDiameter));

    // Representative balanced-rudder coefficient table {angle_deg, Cl, Cd} over
    // the full inflow range: Cl = 1.3·sin 2β (Cl_max ≈ 1.3),
    // Cd = 0.04 + 0.10·(1 − cos 2β) (Cd ≈ 0.11 at 35°).
    for (int a = 0; a <= 360; a += 5)
    {
        const double ar = a * M_PI / 180.0;
        rc.coefficients.push_back({ static_cast<double>(a),
                                    1.3 * std::sin(2.0 * ar),
                                    0.04 + 0.10 * (1.0 - std::cos(2.0 * ar)) });
    }

    cfg.rudders.push_back(rc);
    return cfg;
}

// ---------------------------------------------------------------------------
// YmirSimulation — thin facade over World + NavalDomain
// ---------------------------------------------------------------------------

class YmirSimulation
{
public:
    YmirSimulation()
        : world_(std::make_unique<ymir::World>())
        , domain_(nullptr)
    {
        auto domainPtr = std::make_unique<ymir::NavalDomain>("naval");
        domain_ = domainPtr.get();
        world_->addDomain(std::move(domainPtr));
    }

    void addVessel(int id)
    {
        addVesselAt(id, 0.0, 0.0, 0.0);
    }

    void addVesselAt(int id, double x, double y, double psi)
    {
        domain_->addBody(id, makeBodyAt(id, x, y, psi));

        // Coriolis / gyroscopic coupling (body-frame equations)
        domain_->addNavalForceModel(
            id, std::make_unique<ymir::naval::InertialForces>(vlccInertialConfig()));

        // Hydrodynamic damping (potential radiation + linear viscous + quadratic)
        domain_->addNavalForceModel(
            id, std::make_unique<ymir::naval::DampingForces>(vlccDampingConfig()));

        // Hydrostatic restoring (Archimedes + metacentric stability)
        domain_->addNavalForceModel(
            id, std::make_unique<ymir::naval::RestoringForces>(vlccRestoringConfig()));

        // Hull resistance (Obokata sectional drag — primary surge/sway deceleration)
        domain_->addNavalForceModel(
            id, std::make_unique<ymir::naval::CurrentForces>(vlccCurrentConfig()));

        // Propulsion
        auto thrustModel = std::make_unique<ymir::naval::ThrustForces>(vlccThrustConfig());
        ymir::naval::ThrustForces* thrustPtr = thrustModel.get();
        domain_->addNavalForceModel(id, std::move(thrustModel));
        thrusters_[id] = thrustPtr;

        // Rudder (linked to propulsor slipstream)
        auto rudderModel = std::make_unique<ymir::naval::RudderForces>(
            vlccRudderConfig(), thrustPtr);
        rudders_[id] = rudderModel.get();
        domain_->addNavalForceModel(id, std::move(rudderModel));

        // Wave excitation (1st-order RAO from vessel1.json). Starts calm; the sea
        // state is applied via setWaveConditions when the scenario loads waves.
        auto waveModel = std::make_unique<ymir::naval::WaveForces>(
            ymir::naval::vlccWaveConfig(), makeWaveSpectrum(0.0, 8.0, 0.0, 3.3, 0));
        waves_[id] = waveModel.get();
        domain_->addNavalForceModel(id, std::move(waveModel));

        domain_->initialize();
        initialized_ = true;
    }

    /**
     * Set rudder angle.
     * @param angleDeg  Degrees, positive = starboard (±35° max for VLCC).
     */
    void setRudderAngle(int vesselId, int rudderId, double angleDeg)
    {
        auto it = rudders_.find(vesselId);
        if (it == rudders_.end()) return;
        constexpr double kDegToRad = M_PI / 180.0;
        it->second->setActuatorState(
            static_cast<std::size_t>(rudderId),
            ymir::naval::RudderForces::RudderCommand{angleDeg * kDegToRad});
    }

    /**
     * Set thruster command.
     * @param powerPct    0–100% of nominalRPM (81 RPM for VLCC).
     * @param azimuthDeg  0 = forward (fixed for VLCC; non-zero only for AZP thrusters).
     */
    void setThrusterCommand(int vesselId, int thrusterId, double powerPct, double azimuthDeg)
    {
        auto it = thrusters_.find(vesselId);
        if (it == thrusters_.end()) return;
        constexpr double kNominalRPM = 81.0;
        const double rpm = (powerPct / 100.0) * kNominalRPM;
        it->second->setActuatorState(
            static_cast<std::size_t>(thrusterId),
            ymir::naval::ThrustForces::ThrusterCommand{rpm, azimuthDeg, 0.6145});
    }

    void step(double dt)
    {
        world_->step(dt);
    }

    val getState() const
    {
        const std::string json = domain_->serializeStateJson();
        val JSON = val::global("JSON");
        return JSON.call<val>("parse", val(json));
    }

    void reset()
    {
        domain_->reset();
    }

    double getTime() const
    {
        return world_->time();
    }

    /**
     * Load the full environment timeline from a JSON string.
     * Empty string or bare "{}" are accepted as no-ops — the timeline stays empty.
     * @param json  Serialized EnvironmentProfileDTO (currentSeries / windSeries / waveSeries).
     */
    void loadEnvironment(const std::string& json)
    {
        if (json.empty() || json == "{}") return;
        world_->timeline().loadJson(json);
    }

    /**
     * Apply a scenario sea state to every vessel's wave model. Drives the 6-DOF
     * wave excitation (heave/roll/pitch + surge/sway/yaw) so vessels respond to
     * waves instead of translating rigidly. spectrum: 0=JONSWAP,1=PIERSON,2=REGULAR.
     */
    void setWaveConditions(double Hs, double Tp, double dirDeg, double gamma, int spectrum)
    {
        for (auto& [id, w] : waves_)
            w->setSpectrum(makeWaveSpectrum(Hs, Tp, dirDeg, gamma, spectrum));
    }

private:
    std::unique_ptr<ymir::World> world_;
    ymir::NavalDomain*           domain_;
    bool                         initialized_ = false;

    // Non-owning pointers — models owned by domain_ via addNavalForceModel.
    std::map<int, ymir::naval::ThrustForces*> thrusters_;
    std::map<int, ymir::naval::RudderForces*> rudders_;
    std::map<int, ymir::naval::WaveForces*>   waves_;
};

// ---------------------------------------------------------------------------
// Embind registration
// ---------------------------------------------------------------------------

EMSCRIPTEN_BINDINGS(ymir)
{
    emscripten::class_<YmirSimulation>("YmirSimulation")
        .constructor<>()
        .function("addVessel",          &YmirSimulation::addVessel)
        .function("addVesselAt",        &YmirSimulation::addVesselAt)
        .function("setRudderAngle",     &YmirSimulation::setRudderAngle)
        .function("setThrusterCommand", &YmirSimulation::setThrusterCommand)
        .function("step",               &YmirSimulation::step)
        .function("getState",           &YmirSimulation::getState)
        .function("reset",              &YmirSimulation::reset)
        .function("getTime",            &YmirSimulation::getTime)
        .function("loadEnvironment",    &YmirSimulation::loadEnvironment)
        .function("setWaveConditions",  &YmirSimulation::setWaveConditions);
}
