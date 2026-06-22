#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <ymir/core/Body.h>
#include <ymir/core/ForceModel.h>
#include <ymir/core/Simulation.h>
#include <ymir/core/integrator/CvodeConfig.h>

#include <cmath>
#include <memory>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using namespace ymir;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static Matrix6x6 diagonalMass(double surge, double sway = 0.0, double heave = 0.0)
{
    Matrix6x6 M{};
    M[0][0] = surge;
    M[1][1] = sway  > 0.0 ? sway  : surge;
    M[2][2] = heave > 0.0 ? heave : surge;
    M[3][3] = surge * 100.0;
    M[4][4] = surge * 100.0;
    M[5][5] = surge * 100.0;
    return M;
}

// Force model that applies a constant 6-DOF force
struct ConstantForce : ForceModel
{
    explicit ConstantForce(Forces f) : f_(f) {}
    Forces compute(const BodyState&) override { return f_; }
    std::string name() const override { return "constant"; }

private:
    Forces f_;
};

// Force model: F = -k * x[dof]  (spring in one DOF)
struct SpringForce : ForceModel
{
    SpringForce(double k, int dof) : k_(k), dof_(dof) {}
    Forces compute(const BodyState& s) override
    {
        Forces f;
        f.f[dof_] = -k_ * s.q()[dof_];
        return f;
    }

private:
    double k_;
    int    dof_;
};

// ---------------------------------------------------------------------------
// Test: zero force → constant velocity
// ---------------------------------------------------------------------------

TEST_CASE("Integrator: zero force preserves constant velocity", "[integrator]")
{
    const double m   = 1000.0;
    const double u0  = 2.0;   // m/s initial surge
    const double dt  = 0.1;
    const int    N   = 100;   // 10 seconds

    Matrix6x6 mass = diagonalMass(m);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};
    qdot[0] = u0;

    CvodeConfig cfg;
    cfg.reltol = 1e-9;
    cfg.abstol = 1e-11;

    auto sim = std::make_unique<Simulation>(
        std::make_unique<Body>(mass, added, q, qdot), cfg);

    sim->addForceModel(std::make_unique<ZeroForceModel>());
    sim->initialize();

    for (int i = 0; i < N; ++i)
        sim->step(dt);

    BodyState s = sim->state();

    // x(10s) ≈ u0 * 10 = 20 m  (but CVODE integrates in body frame then rotates)
    // With yaw=0: inertial x == body surge integral
    REQUIRE_THAT(s.x(), WithinRel(u0 * N * dt, 1e-6));
    REQUIRE_THAT(s.u(), WithinRel(u0, 1e-6));
}

// ---------------------------------------------------------------------------
// Test: constant surge force → x = 0.5 * a * t²
// ---------------------------------------------------------------------------

TEST_CASE("Integrator: constant force gives x = 0.5*a*t^2", "[integrator]")
{
    const double m   = 1000.0;
    const double F   = 100.0;   // N
    const double a   = F / m;   // 0.1 m/s²
    const double T   = 10.0;    // simulate 10 s
    const double dt  = 0.1;

    Matrix6x6 mass = diagonalMass(m);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};

    Forces force;
    force.f[0] = F;

    CvodeConfig cfg;
    auto sim = std::make_unique<Simulation>(
        std::make_unique<Body>(mass, added, q, qdot), cfg);
    sim->addForceModel(std::make_unique<ConstantForce>(force));
    sim->initialize();

    int steps = static_cast<int>(T / dt);
    for (int i = 0; i < steps; ++i)
        sim->step(dt);

    double x_expected = 0.5 * a * T * T;  // = 5 m
    BodyState s = sim->state();

    REQUIRE_THAT(s.x(), WithinRel(x_expected, 1e-3));
    REQUIRE_THAT(s.u(), WithinRel(a * T, 1e-3));       // v = a*t = 1 m/s
}

// ---------------------------------------------------------------------------
// Test: simple harmonic oscillator — period T = 2π√(m/k)
// ---------------------------------------------------------------------------

TEST_CASE("Integrator: SHO period matches 2pi*sqrt(m/k)", "[integrator]")
{
    const double m  = 10.0;
    const double k  = 10.0;
    const double x0 = 1.0;  // initial displacement in surge

    const double T_expected = 2.0 * std::acos(-1.0) * std::sqrt(m / k);  // ≈ 6.283 s
    const double dt         = 0.05;
    const int    N_periods  = 3;
    const int    steps      = static_cast<int>(N_periods * T_expected / dt) + 1;

    Matrix6x6 mass = diagonalMass(m);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};
    q[0] = x0;

    CvodeConfig cfg;
    cfg.reltol = 1e-9;
    cfg.abstol = 1e-11;

    auto sim = std::make_unique<Simulation>(
        std::make_unique<Body>(mass, added, q, qdot), cfg);
    sim->addForceModel(std::make_unique<SpringForce>(k, 0));
    sim->initialize();

    // Track zero-crossings to measure period (linear interpolation for sub-step accuracy)
    double prev_x = x0;
    double t      = 0.0;
    int    crossings = 0;
    double t_first_cross = 0.0;
    double t_last_cross  = 0.0;

    for (int i = 0; i < steps; ++i)
    {
        sim->step(dt);
        t += dt;
        double curr_x = sim->state().x();

        if (prev_x > 0.0 && curr_x <= 0.0)
        {
            // interpolate exact crossing: t_cross = t - dt + dt * prev_x / (prev_x - curr_x)
            double t_cross = t - dt + dt * prev_x / (prev_x - curr_x);
            crossings++;
            if (crossings == 1) t_first_cross = t_cross;
            t_last_cross = t_cross;
        }
        prev_x = curr_x;
    }

    // Downward zero-crossings are spaced exactly one period apart; N_periods = 3 → 6 crossings
    REQUIRE(crossings >= 2);
    double measured_period = (t_last_cross - t_first_cross) / (crossings - 1);
    REQUIRE_THAT(measured_period, WithinRel(T_expected, 1e-3));
}

// ---------------------------------------------------------------------------
// Test: Simulation time tracks correctly
// ---------------------------------------------------------------------------

TEST_CASE("Simulation time accumulates correctly", "[simulation]")
{
    const double dt = 0.1;
    const int    N  = 50;

    Matrix6x6 mass = diagonalMass(1000.0);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};

    auto sim = std::make_unique<Simulation>(
        std::make_unique<Body>(mass, added, q, qdot));
    sim->addForceModel(std::make_unique<ZeroForceModel>());
    sim->initialize();

    for (int i = 0; i < N; ++i)
        sim->step(dt);

    REQUIRE_THAT(sim->time(), WithinAbs(N * dt, 1e-10));
}

TEST_CASE("Simulation throws if step called before initialize", "[simulation]")
{
    Matrix6x6 mass = diagonalMass(1000.0);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};

    auto sim = std::make_unique<Simulation>(
        std::make_unique<Body>(mass, added, q, qdot));

    REQUIRE_THROWS_AS(sim->step(0.1), std::logic_error);
}
