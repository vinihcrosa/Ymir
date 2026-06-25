#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/physics/ForceModel.h>
#include <ymir/simulation/Simulation.h>
#include <ymir/physics/integrator/CvodeConfig.h>

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

struct ConstantForce : ForceModel
{
    explicit ConstantForce(Forces f) : f_(f) {}
    Forces compute(const BodyState&) override { return f_; }
    std::string name() const override { return "constant"; }
private:
    Forces f_;
};

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
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("Integrator: zero force preserves constant velocity", "[integrator]")
{
    const double m  = 1000.0;
    const double u0 = 2.0;
    const double dt = 0.1;
    const int    N  = 100;

    Matrix6x6 mass = diagonalMass(m);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};
    qdot[0] = u0;

    CvodeConfig cfg;
    cfg.reltol = 1e-9;
    cfg.abstol = 1e-11;

    auto body = std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot, cfg);
    body->addForceModel(std::make_unique<ZeroForceModel>());

    Simulation sim;
    sim.addBody(0, std::move(body));

    for (int i = 0; i < N; ++i)
        sim.step(dt);

    BodyState s = sim.state(0);
    REQUIRE_THAT(s.x(), WithinRel(u0 * N * dt, 1e-6));
    REQUIRE_THAT(s.u(), WithinRel(u0, 1e-6));
}

TEST_CASE("Integrator: constant force gives x = 0.5*a*t^2", "[integrator]")
{
    const double m  = 1000.0;
    const double F  = 100.0;
    const double a  = F / m;
    const double T  = 10.0;
    const double dt = 0.1;

    Matrix6x6 mass = diagonalMass(m);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};

    Forces force;
    force.f[0] = F;

    auto body = std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot);
    body->addForceModel(std::make_unique<ConstantForce>(force));

    Simulation sim;
    sim.addBody(0, std::move(body));

    int steps = static_cast<int>(T / dt);
    for (int i = 0; i < steps; ++i)
        sim.step(dt);

    BodyState s = sim.state(0);
    REQUIRE_THAT(s.x(), WithinRel(0.5 * a * T * T, 1e-3));
    REQUIRE_THAT(s.u(), WithinRel(a * T, 1e-3));
}

TEST_CASE("Integrator: SHO period matches 2pi*sqrt(m/k)", "[integrator]")
{
    const double m  = 10.0;
    const double k  = 10.0;
    const double x0 = 1.0;

    const double T_expected = 2.0 * std::acos(-1.0) * std::sqrt(m / k);
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

    auto body = std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot, cfg);
    body->addForceModel(std::make_unique<SpringForce>(k, 0));

    Simulation sim;
    sim.addBody(0, std::move(body));

    double prev_x = x0, t = 0.0;
    int    crossings = 0;
    double t_first_cross = 0.0, t_last_cross = 0.0;

    for (int i = 0; i < steps; ++i)
    {
        sim.step(dt);
        t += dt;
        double curr_x = sim.state(0).x();

        if (prev_x > 0.0 && curr_x <= 0.0)
        {
            double t_cross = t - dt + dt * prev_x / (prev_x - curr_x);
            crossings++;
            if (crossings == 1) t_first_cross = t_cross;
            t_last_cross = t_cross;
        }
        prev_x = curr_x;
    }

    REQUIRE(crossings >= 2);
    double measured_period = (t_last_cross - t_first_cross) / (crossings - 1);
    REQUIRE_THAT(measured_period, WithinRel(T_expected, 1e-3));
}

TEST_CASE("Simulation time accumulates correctly", "[simulation]")
{
    const double dt = 0.1;
    const int    N  = 50;

    Matrix6x6 mass = diagonalMass(1000.0);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};

    auto body = std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot);
    body->addForceModel(std::make_unique<ZeroForceModel>());

    Simulation sim;
    sim.addBody(0, std::move(body));

    for (int i = 0; i < N; ++i)
        sim.step(dt);

    REQUIRE_THAT(sim.time(), WithinAbs(N * dt, 1e-10));
}

TEST_CASE("Simulation: addBody throws on duplicate id", "[simulation]")
{
    Matrix6x6 mass = diagonalMass(1000.0);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};

    Simulation sim;
    sim.addBody(0, std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot));

    REQUIRE_THROWS_AS(
        sim.addBody(0, std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot)),
        std::invalid_argument);
}

TEST_CASE("Simulation: multi-body both advance", "[simulation]")
{
    const double dt = 0.1;
    const int    N  = 10;

    Matrix6x6 mass = diagonalMass(1000.0);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};

    auto b0 = std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot);
    auto b1 = std::make_unique<RigidBody6DOF>(1, mass, added, q, qdot);
    b0->addForceModel(std::make_unique<ZeroForceModel>());
    b1->addForceModel(std::make_unique<ZeroForceModel>());

    Simulation sim;
    sim.addBody(0, std::move(b0));
    sim.addBody(1, std::move(b1));

    REQUIRE(sim.bodyCount() == 2);

    for (int i = 0; i < N; ++i)
        sim.step(dt);

    REQUIRE_THAT(sim.time(),      WithinAbs(N * dt, 1e-10));
    REQUIRE_THAT(sim.state(0).time(), WithinAbs(N * dt, 1e-10));
    REQUIRE_THAT(sim.state(1).time(), WithinAbs(N * dt, 1e-10));
}
