#pragma once

#include <ymir/physics/integrator/IIntegrator.h>
#include <ymir/common/Types.h>

#include <vector>

namespace ymir
{

struct RK45Config
{
    double reltol   = 1e-6;
    double abstol   = 1e-8;
    double maxStep  = 0.0;   // 0 = unlimited
    int    maxSteps = 10000;
};

/**
 * Dormand-Prince RK45 adaptive integrator for 6-DOF rigid body dynamics.
 *
 * Fourth-order accurate with fifth-order error estimate.
 * Step size adapts to keep local error within (atol + rtol*|y|).
 * No heap allocation during integration — stage buffers are pre-allocated.
 *
 * State vector (12 doubles):
 *   y[0..5]  = q     (position in inertial frame)
 *   y[6..11] = qdot  (velocity in body frame)
 */
class RK45Integrator : public IIntegrator
{
public:
    explicit RK45Integrator(const RK45Config& config = {});
    ~RK45Integrator() override = default;

    RK45Integrator(const RK45Integrator&)            = delete;
    RK45Integrator& operator=(const RK45Integrator&) = delete;
    RK45Integrator(RK45Integrator&&)                 = delete;
    RK45Integrator& operator=(RK45Integrator&&)      = delete;

    void   initialize(RigidBody6DOF& body, std::vector<ForceModel*>& models) override;
    void   step(RigidBody6DOF& body, double dt) override;
    void   reset(RigidBody6DOF& body) override;
    double time() const noexcept override { return t_; }

private:
    /** Evaluate RHS: given state y (12 doubles), return dy/dt (12 doubles). */
    void rhs(RigidBody6DOF& body, const double* y, double* dydt, double t_eval);

    /** Pack body (q, qdot) → y[12]. */
    static void pack(const RigidBody6DOF& body, double* y);

    /** Unpack y[12] → body (q, qdot) and update accelerations. */
    void unpack(RigidBody6DOF& body, const double* y, double t_eval);

    RK45Config             config_;
    std::vector<ForceModel*>* models_ = nullptr;
    double                 t_       = 0.0;

    // Pre-allocated stage buffers (12 doubles each) — no heap in hot path
    double y_[12]{};
    double k1_[12]{}, k2_[12]{}, k3_[12]{}, k4_[12]{};
    double k5_[12]{}, k6_[12]{}, k7_[12]{};
    double ytmp_[12]{};
    double y5_[12]{};   // 5th-order solution (error estimate vs implicit 4th)
};

} // namespace ymir
