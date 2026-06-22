#include <ymir/core/integrator/CvodeIntegrator.h>

#include <ymir/core/RigidBody6DOF.h>
#include <ymir/core/ForceModel.h>
#include <ymir/core/Forces.h>

#include <sundials/sundials_context.h>
#include <cvode/cvode.h>
#include <nvector/nvector_serial.h>
#include <sunmatrix/sunmatrix_dense.h>
#include <sunlinsol/sunlinsol_dense.h>

#include <cmath>
#include <stdexcept>
#include <string>

namespace ymir
{

// ---------------------------------------------------------------------------
// RHS — CVODE callback (static member → has friend access to RigidBody6DOF)
// ---------------------------------------------------------------------------

int CvodeIntegrator::rhsCallback(sunrealtype t_rhs, N_Vector y, N_Vector ydot, void* user_data)
{
    auto*          ctx  = static_cast<CvodeIntegrator::RhsContext*>(user_data);
    RigidBody6DOF& body = *ctx->body;

    // 1. Extract q and qdot from CVODE state vector
    Vector6 q{}, qdot{};
    for (int i = 0; i < 6; ++i)
    {
        q[i]    = NV_Ith_S(y, i);
        qdot[i] = NV_Ith_S(y, i + 6);
    }

    // 2. Update body state so ForceModels read current kinematics
    body.setState(q, qdot);

    // 3. Sum all forces (pass t_rhs as the evaluation time, not body.time())
    BodyState state{q, qdot, static_cast<double>(t_rhs), 0.0};
    Forces    Ft = Forces::zero();
    for (ForceModel* model : *ctx->models)
        Ft += model->compute(state);

    // 4. Compute acceleration: qddot = invM * Ft
    Vector6 qddot = body.computeAcceleration(Ft);
    body.setAcceleration(qddot);

    // 5. Kinematic transformation: ydot[0..5] (body → inertial)
    const double psi = q[5];  // yaw
    NV_Ith_S(ydot, 0) = std::cos(psi) * qdot[0] - std::sin(psi) * qdot[1];  // ẋ
    NV_Ith_S(ydot, 1) = std::sin(psi) * qdot[0] + std::cos(psi) * qdot[1];  // ẏ
    NV_Ith_S(ydot, 2) = qdot[2];  // ż
    // ASSUMPTION: small-angle — Euler rates ≈ body angular rates
    NV_Ith_S(ydot, 3) = qdot[3];  // φ̇ ≈ p
    NV_Ith_S(ydot, 4) = qdot[4];  // θ̇ ≈ q
    NV_Ith_S(ydot, 5) = qdot[5];  // ψ̇ = r

    // 6. ydot[6..11] = qddot
    for (int i = 0; i < 6; ++i)
        NV_Ith_S(ydot, 6 + i) = qddot[i];

    return 0;
}

// ---------------------------------------------------------------------------
// CvodeIntegrator
// ---------------------------------------------------------------------------

CvodeIntegrator::CvodeIntegrator(const CvodeConfig& config)
    : config_(config)
{
    if (SUNContext_Create(SUN_COMM_NULL, &sunCtx_) != 0)
        throw std::runtime_error("CvodeIntegrator: SUNContext_Create failed");

    mem_ = CVodeCreate(CV_BDF, sunCtx_);
    if (!mem_)
        throw std::runtime_error("CvodeIntegrator: CVodeCreate failed");
}

CvodeIntegrator::~CvodeIntegrator()
{
    if (LS_)     SUNLinSolFree(LS_);
    if (A_)      SUNMatDestroy(A_);
    if (y_)      N_VDestroy(y_);
    if (mem_)    CVodeFree(&mem_);
    if (sunCtx_) SUNContext_Free(&sunCtx_);
}

void CvodeIntegrator::initialize(RigidBody6DOF& body, std::vector<ForceModel*>& models)
{
    ctx_ = RhsContext{&body, &models};
    t_   = 0.0;

    y_ = N_VNew_Serial(12, sunCtx_);
    if (!y_)
        throw std::runtime_error("CvodeIntegrator: N_VNew_Serial failed");

    const Vector6& q    = body.q();
    const Vector6& qdot = body.qdot();
    for (int i = 0; i < 6; ++i)
    {
        NV_Ith_S(y_, i)     = q[i];
        NV_Ith_S(y_, i + 6) = qdot[i];
    }

    if (CVodeInit(mem_, CvodeIntegrator::rhsCallback, t_, y_) != CV_SUCCESS)
        throw std::runtime_error("CvodeIntegrator: CVodeInit failed");

    if (CVodeSStolerances(mem_, config_.reltol, config_.abstol) != CV_SUCCESS)
        throw std::runtime_error("CvodeIntegrator: CVodeSStolerances failed");

    if (CVodeSetMaxNumSteps(mem_, config_.maxSteps) != CV_SUCCESS)
        throw std::runtime_error("CvodeIntegrator: CVodeSetMaxNumSteps failed");

    if (config_.maxStep > 0.0)
        CVodeSetMaxStep(mem_, config_.maxStep);

    CVodeSetUserData(mem_, &ctx_);

    A_ = SUNDenseMatrix(12, 12, sunCtx_);
    if (!A_)
        throw std::runtime_error("CvodeIntegrator: SUNDenseMatrix failed");

    LS_ = SUNLinSol_Dense(y_, A_, sunCtx_);
    if (!LS_)
        throw std::runtime_error("CvodeIntegrator: SUNLinSol_Dense failed");

    if (CVodeSetLinearSolver(mem_, LS_, A_) != CVLS_SUCCESS)
        throw std::runtime_error("CvodeIntegrator: CVodeSetLinearSolver failed");
}

void CvodeIntegrator::step(RigidBody6DOF& body, double dt)
{
    double t_target = t_ + dt;

    int flag = CVode(mem_, t_target, y_, &t_, CV_NORMAL);
    if (flag < 0)
    {
        throw std::runtime_error(
            "CvodeIntegrator: CVode failed with flag " + std::to_string(flag));
    }

    Vector6 q{}, qdot{};
    for (int i = 0; i < 6; ++i)
    {
        q[i]    = NV_Ith_S(y_, i);
        qdot[i] = NV_Ith_S(y_, i + 6);
    }
    body.setState(q, qdot);
}

void CvodeIntegrator::reset(RigidBody6DOF& body)
{
    const Vector6& q    = body.q();
    const Vector6& qdot = body.qdot();
    for (int i = 0; i < 6; ++i)
    {
        NV_Ith_S(y_, i)     = q[i];
        NV_Ith_S(y_, i + 6) = qdot[i];
    }
    t_ = 0.0;
    CVodeReInit(mem_, t_, y_);
}

} // namespace ymir
