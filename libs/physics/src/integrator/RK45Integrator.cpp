#include <ymir/physics/integrator/RK45Integrator.h>
#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/physics/ForceModel.h>
#include <ymir/physics/Forces.h>

#include <cmath>
#include <stdexcept>

namespace ymir
{

// ---------------------------------------------------------------------------
// Dormand-Prince coefficients (DOPRI5)
// ---------------------------------------------------------------------------

// c — time nodes
static constexpr double c2 = 1.0/5.0,  c3 = 3.0/10.0, c4 = 4.0/5.0;
static constexpr double c5 = 8.0/9.0,  c6 = 1.0,       c7 = 1.0;

// a — Runge-Kutta matrix (lower triangular)
static constexpr double a21 =  1.0/5.0;
static constexpr double a31 =  3.0/40.0,      a32 =  9.0/40.0;
static constexpr double a41 =  44.0/45.0,     a42 = -56.0/15.0,     a43 =  32.0/9.0;
static constexpr double a51 =  19372.0/6561.0, a52 = -25360.0/2187.0,
                        a53 =  64448.0/6561.0, a54 = -212.0/729.0;
static constexpr double a61 =  9017.0/3168.0,  a62 = -355.0/33.0,
                        a63 =  46732.0/5247.0, a64 =  49.0/176.0,    a65 = -5103.0/18656.0;

// b — 5th-order weights
static constexpr double b1 =  35.0/384.0,  b3 = 500.0/1113.0,
                        b4 =  125.0/192.0, b5 = -2187.0/6784.0,  b6 = 11.0/84.0;

// e — error coefficients (b5 - b4, Dormand-Prince)
static constexpr double e1 =  71.0/57600.0,   e3 = -71.0/16695.0,
                        e4 =  71.0/1920.0,    e5 = -17253.0/339200.0,
                        e6 =  22.0/525.0,     e7 = -1.0/40.0;

// ---------------------------------------------------------------------------

RK45Integrator::RK45Integrator(const RK45Config& config)
    : config_(config)
{}

void RK45Integrator::initialize(RigidBody6DOF& body, std::vector<ForceModel*>& models)
{
    models_ = &models;
    t_      = 0.0;
    pack(body, y_);
}

void RK45Integrator::reset(RigidBody6DOF& body)
{
    t_ = 0.0;
    pack(body, y_);
}

// ---------------------------------------------------------------------------
// pack / unpack
// ---------------------------------------------------------------------------

void RK45Integrator::pack(const RigidBody6DOF& body, double* y)
{
    const Vector6& q    = body.q();
    const Vector6& qdot = body.qdot();
    for (int i = 0; i < 6; ++i)
    {
        y[i]     = q[i];
        y[i + 6] = qdot[i];
    }
}

void RK45Integrator::unpack(RigidBody6DOF& body, const double* y, double t_eval)
{
    Vector6 q{}, qdot{};
    for (int i = 0; i < 6; ++i)
    {
        q[i]    = y[i];
        qdot[i] = y[i + 6];
    }
    body.setState(q, qdot);

    // Recompute acceleration at this state (for BodyState consumers)
    BodyState s{q, qdot, t_eval, 0.0};
    Forces    Ft = Forces::zero();
    for (ForceModel* m : *models_)
        Ft += m->compute(s);
    body.setAcceleration(body.computeAcceleration(Ft));
}

// ---------------------------------------------------------------------------
// RHS  dy/dt = f(t, y)
// ---------------------------------------------------------------------------

void RK45Integrator::rhs(RigidBody6DOF& body, const double* y, double* dydt, double t_eval)
{
    Vector6 q{}, qdot{};
    for (int i = 0; i < 6; ++i)
    {
        q[i]    = y[i];
        qdot[i] = y[i + 6];
    }
    body.setState(q, qdot);

    BodyState s{q, qdot, t_eval, 0.0};
    Forces    Ft = Forces::zero();
    for (ForceModel* m : *models_)
        Ft += m->compute(s);

    Vector6 qddot = body.computeAcceleration(Ft);
    body.setAcceleration(qddot);

    // Kinematic transformation: inertial position rates
    const double psi = q[5];
    dydt[0] = std::cos(psi) * qdot[0] - std::sin(psi) * qdot[1];  // ẋ
    dydt[1] = std::sin(psi) * qdot[0] + std::cos(psi) * qdot[1];  // ẏ
    dydt[2] = qdot[2];  // ż
    dydt[3] = qdot[3];  // φ̇ (small-angle)
    dydt[4] = qdot[4];  // θ̇
    dydt[5] = qdot[5];  // ψ̇ = r

    for (int i = 0; i < 6; ++i)
        dydt[6 + i] = qddot[i];
}

// ---------------------------------------------------------------------------
// Adaptive step: advance body by exactly dt using DOPRI5
// ---------------------------------------------------------------------------

void RK45Integrator::step(RigidBody6DOF& body, double dt)
{
    double h     = dt;
    double t_end = t_ + dt;
    int    nstep = 0;

    while (t_ < t_end)
    {
        if (nstep++ > config_.maxSteps)
            throw std::runtime_error("RK45Integrator: max steps exceeded");

        // Clamp to target
        if (t_ + h > t_end)
            h = t_end - t_;

        if (config_.maxStep > 0.0 && h > config_.maxStep)
            h = config_.maxStep;

        // Stage 1
        rhs(body, y_, k1_, t_);

        // Stage 2
        for (int i = 0; i < 12; ++i)
            ytmp_[i] = y_[i] + h * a21 * k1_[i];
        rhs(body, ytmp_, k2_, t_ + c2 * h);

        // Stage 3
        for (int i = 0; i < 12; ++i)
            ytmp_[i] = y_[i] + h * (a31 * k1_[i] + a32 * k2_[i]);
        rhs(body, ytmp_, k3_, t_ + c3 * h);

        // Stage 4
        for (int i = 0; i < 12; ++i)
            ytmp_[i] = y_[i] + h * (a41 * k1_[i] + a42 * k2_[i] + a43 * k3_[i]);
        rhs(body, ytmp_, k4_, t_ + c4 * h);

        // Stage 5
        for (int i = 0; i < 12; ++i)
            ytmp_[i] = y_[i] + h * (a51 * k1_[i] + a52 * k2_[i]
                                   + a53 * k3_[i] + a54 * k4_[i]);
        rhs(body, ytmp_, k5_, t_ + c5 * h);

        // Stage 6
        for (int i = 0; i < 12; ++i)
            ytmp_[i] = y_[i] + h * (a61 * k1_[i] + a62 * k2_[i] + a63 * k3_[i]
                                   + a64 * k4_[i] + a65 * k5_[i]);
        rhs(body, ytmp_, k6_, t_ + c6 * h);

        // 5th-order solution (also stage 7 = FSAL reuse next step)
        for (int i = 0; i < 12; ++i)
            y5_[i] = y_[i] + h * (b1 * k1_[i] + b3 * k3_[i] + b4 * k4_[i]
                                  + b5 * k5_[i] + b6 * k6_[i]);

        rhs(body, y5_, k7_, t_ + c7 * h);

        // Error estimate (difference between 4th and 5th order)
        double err = 0.0;
        for (int i = 0; i < 12; ++i)
        {
            double sc = config_.abstol + config_.reltol * std::max(std::abs(y_[i]),
                                                                    std::abs(y5_[i]));
            double ei = h * (e1 * k1_[i] + e3 * k3_[i] + e4 * k4_[i]
                            + e5 * k5_[i] + e6 * k6_[i] + e7 * k7_[i]);
            err += (ei / sc) * (ei / sc);
        }
        err = std::sqrt(err / 12.0);

        if (err <= 1.0)
        {
            // Accept step: advance state and time
            for (int i = 0; i < 12; ++i)
                y_[i] = y5_[i];
            t_ += h;

            // FSAL: k7 becomes k1 of next step (already evaluated)
            for (int i = 0; i < 12; ++i)
                k1_[i] = k7_[i];
        }

        // Adjust step size (PI controller, capped at 10x increase)
        if (err == 0.0) err = 1e-10;
        double factor = 0.9 * std::pow(1.0 / err, 0.2);
        factor = std::min(factor, 10.0);
        factor = std::max(factor, 0.1);
        h *= factor;
    }

    // Sync body state from final y_
    Vector6 q{}, qdot{};
    for (int i = 0; i < 6; ++i)
    {
        q[i]    = y_[i];
        qdot[i] = y_[i + 6];
    }
    body.setState(q, qdot);
}

} // namespace ymir
