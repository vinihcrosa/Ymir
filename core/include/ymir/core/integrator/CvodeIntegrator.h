#pragma once

#include <ymir/core/integrator/CvodeConfig.h>

#include <sundials/sundials_context.h>
#include <nvector/nvector_serial.h>
#include <sunmatrix/sunmatrix_dense.h>
#include <sunlinsol/sunlinsol_dense.h>

#include <vector>

namespace ymir
{

class RigidBody6DOF;
class ForceModel;

/**
 * CVODE/BDF integrator for 6-DOF rigid body dynamics.
 *
 * State vector (12 doubles):
 *   y[0..5]  = q     (position in inertial frame)
 *   y[6..11] = qdot  (velocity in body frame)
 *
 * Non-copyable and non-movable: CVODE's user_data pointer references
 * internal context by address (see D-10 in STATE.md).
 */
class CvodeIntegrator
{
public:
    explicit CvodeIntegrator(const CvodeConfig& config = {});
    ~CvodeIntegrator();

    CvodeIntegrator(const CvodeIntegrator&)            = delete;
    CvodeIntegrator& operator=(const CvodeIntegrator&) = delete;
    CvodeIntegrator(CvodeIntegrator&&)                 = delete;
    CvodeIntegrator& operator=(CvodeIntegrator&&)      = delete;

    /** Allocate CVODE memory and set initial conditions from body. */
    void initialize(RigidBody6DOF& body, std::vector<ForceModel*>& models);

    /** Advance body state by dt seconds. Throws on CVODE error. */
    void step(RigidBody6DOF& body, double dt);

    /** Reinitialize CVODE with the body's current state (no reallocation). */
    void reset(RigidBody6DOF& body);

    double time() const noexcept { return t_; }

    // Public so the static rhs() callback can access it — not user API.
    struct RhsContext
    {
        RigidBody6DOF*            body   = nullptr;
        std::vector<ForceModel*>* models = nullptr;
    };

private:
    CvodeConfig config_;
    double      t_ = 0.0;

    SUNContext      sunCtx_ = nullptr;
    void*           mem_    = nullptr;
    N_Vector        y_      = nullptr;
    SUNMatrix       A_      = nullptr;
    SUNLinearSolver LS_     = nullptr;

    RhsContext ctx_{};

    static int rhsCallback(sunrealtype t, N_Vector y, N_Vector ydot, void* user_data);
};

} // namespace ymir
