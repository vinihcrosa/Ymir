#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/physics/ForceModel.h>
#include <ymir/physics/integrator/RK45Integrator.h>
#include <ymir/physics/integrator/IIntegrator.h>

#include <cmath>
#include <memory>
#include <vector>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using namespace ymir;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static Matrix6x6 diagonalMass(double m)
{
    Matrix6x6 M{};
    M[0][0] = m; M[1][1] = m; M[2][2] = m;
    M[3][3] = m * 100.0; M[4][4] = m * 100.0; M[5][5] = m * 100.0;
    return M;
}

/** Drive body using RK45 directly (bypasses RigidBody6DOF::step which uses CVODE). */
static BodyState runRK45(RigidBody6DOF& body,
                          std::vector<ForceModel*> models,
                          double dt, int steps,
                          const RK45Config& cfg = {})
{
    RK45Integrator rk{cfg};
    rk.initialize(body, models);
    for (int i = 0; i < steps; ++i)
        rk.step(body, dt);
    return body.state();
}

struct SpringForceRK : ForceModel
{
    SpringForceRK(double k, int dof) : k_(k), dof_(dof) {}
    Forces compute(const BodyState& s) override
    {
        Forces f; f.f[dof_] = -k_ * s.q()[dof_]; return f;
    }
    std::string name() const override { return "spring"; }
private: double k_; int dof_;
};

struct ConstantForceRK : ForceModel
{
    explicit ConstantForceRK(Forces f) : f_(f) {}
    Forces compute(const BodyState&) override { return f_; }
    std::string name() const override { return "constant"; }
private: Forces f_;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("RK45: zero force preserves constant velocity", "[rk45]")
{
    const double m  = 1000.0;
    const double u0 = 2.0;
    const double dt = 0.1;
    const int    N  = 100;

    Matrix6x6 mass = diagonalMass(m);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};
    qdot[0] = u0;

    auto body    = std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot);
    auto zero    = std::make_unique<ZeroForceModel>();
    std::vector<ForceModel*> models{zero.get()};

    BodyState s = runRK45(*body, models, dt, N);
    REQUIRE_THAT(s.x(), WithinRel(u0 * N * dt, 1e-5));
    REQUIRE_THAT(s.u(), WithinRel(u0, 1e-5));
}

TEST_CASE("RK45: constant force gives x = 0.5*a*t^2", "[rk45]")
{
    const double m  = 1000.0;
    const double F  = 100.0;
    const double a  = F / m;
    const double T  = 10.0;
    const double dt = 0.1;

    Matrix6x6 mass = diagonalMass(m);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};

    Forces force; force.f[0] = F;
    auto body = std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot);
    auto fm   = std::make_unique<ConstantForceRK>(force);
    std::vector<ForceModel*> models{fm.get()};

    const int steps = static_cast<int>(T / dt);
    BodyState s = runRK45(*body, models, dt, steps);

    REQUIRE_THAT(s.x(), WithinRel(0.5 * a * T * T, 1e-3));
    REQUIRE_THAT(s.u(), WithinRel(a * T, 1e-3));
}

TEST_CASE("RK45: SHO period matches 2pi*sqrt(m/k)", "[rk45]")
{
    const double m  = 10.0;
    const double k  = 10.0;
    const double x0 = 1.0;
    const double T_expected = 2.0 * std::acos(-1.0) * std::sqrt(m / k);
    const double dt = 0.05;
    const int    N_periods  = 3;
    const int    steps = static_cast<int>(N_periods * T_expected / dt) + 1;

    Matrix6x6 mass = diagonalMass(m);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};
    q[0] = x0;

    auto body = std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot);
    auto spr  = std::make_unique<SpringForceRK>(k, 0);
    std::vector<ForceModel*> models{spr.get()};

    RK45Integrator rk{RK45Config{1e-8, 1e-10}};
    rk.initialize(*body, models);

    double prev_x = x0, t = 0.0;
    int    crossings = 0;
    double t_first   = 0.0, t_last = 0.0;

    for (int i = 0; i < steps; ++i)
    {
        rk.step(*body, dt);
        t += dt;
        double curr_x = body->state().x();

        if (prev_x > 0.0 && curr_x <= 0.0)
        {
            double tc = t - dt + dt * prev_x / (prev_x - curr_x);
            crossings++;
            if (crossings == 1) t_first = tc;
            t_last = tc;
        }
        prev_x = curr_x;
    }

    REQUIRE(crossings >= 2);
    double measured = (t_last - t_first) / (crossings - 1);
    REQUIRE_THAT(measured, WithinRel(T_expected, 1e-3));
}

TEST_CASE("RK45: analytical SHO error < 1e-4 over 10s", "[rk45]")
{
    // x(t) = cos(omega*t), omega = sqrt(k/m)
    const double m     = 1.0;
    const double k     = 1.0;
    const double omega = std::sqrt(k / m);
    const double x0    = 1.0;
    const double T_sim = 10.0;
    const double dt    = 0.5;

    Matrix6x6 mass = diagonalMass(m);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};
    q[0] = x0;

    auto body = std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot);
    auto spr  = std::make_unique<SpringForceRK>(k, 0);
    std::vector<ForceModel*> models{spr.get()};

    const int steps = static_cast<int>(T_sim / dt);
    BodyState s = runRK45(*body, models, dt, steps, {1e-6, 1e-8});

    const double t_final = steps * dt;
    const double x_exact = x0 * std::cos(omega * t_final);
    REQUIRE_THAT(s.x(), WithinAbs(x_exact, 1e-4));
}

TEST_CASE("RK45: IIntegrator interface is honoured", "[rk45]")
{
    std::unique_ptr<IIntegrator> integrator = std::make_unique<RK45Integrator>();
    REQUIRE(integrator != nullptr);
    REQUIRE(integrator->time() == 0.0);
}

TEST_CASE("RK45: step adaptation — large dt converges without maxSteps error", "[rk45]")
{
    // SHO with large outer dt — internal step size adapts automatically
    const double m  = 10.0;
    const double k  = 10.0;
    const double x0 = 1.0;

    Matrix6x6 mass = diagonalMass(m);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};
    q[0] = x0;

    auto body = std::make_unique<RigidBody6DOF>(0, mass, added, q, qdot);
    auto spr  = std::make_unique<SpringForceRK>(k, 0);
    std::vector<ForceModel*> models{spr.get()};

    // dt = 1.0s is much larger than the natural period would allow fixed-step
    // RK45 adapts internally — must not throw
    REQUIRE_NOTHROW(runRK45(*body, models, /*dt=*/1.0, /*steps=*/10, {1e-6, 1e-8}));
}
