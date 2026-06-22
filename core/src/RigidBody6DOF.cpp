#include <ymir/core/RigidBody6DOF.h>
#include <ymir/core/integrator/CvodeIntegrator.h>
#include <ymir/core/math/LinearAlgebra.h>

namespace ymir
{

RigidBody6DOF::RigidBody6DOF(int                id,
                               const Matrix6x6&   massMatrix,
                               const Matrix6x6&   addedMass,
                               const Vector6&     initialQ,
                               const Vector6&     initialQdot,
                               const CvodeConfig& config)
    : id_(id)
    , massMatrix_(massMatrix)
    , addedMass_(addedMass)
    , totalMass_(math::matAdd(massMatrix, addedMass))
    , invTotalMass_(math::invert6x6(totalMass_))
    , q_(initialQ)
    , qdot_(initialQdot)
    , qddot_{}
    , integrator_(std::make_unique<CvodeIntegrator>(config))
{
}

RigidBody6DOF::~RigidBody6DOF() = default;

void RigidBody6DOF::addForceModel(std::unique_ptr<ForceModel> model)
{
    modelPtrs_.push_back(model.get());
    ownedModels_.push_back(std::move(model));
}

void RigidBody6DOF::step(double dt)
{
    if (!initialized_)
    {
        integrator_->initialize(*this, modelPtrs_);
        initialized_ = true;
    }
    integrator_->step(*this, dt);
    t_ += dt;
}

void RigidBody6DOF::reset()
{
    if (initialized_)
        integrator_->reset(*this);
    t_ = 0.0;
}

BodyState RigidBody6DOF::state() const noexcept
{
    return BodyState{q_, qdot_, t_, 0.0};
}

Vector6 RigidBody6DOF::computeAcceleration(const Forces& totalForce) const noexcept
{
    return math::matVecProduct(invTotalMass_, totalForce.f);
}

void RigidBody6DOF::setState(const Vector6& q, const Vector6& qdot) noexcept
{
    q_    = q;
    qdot_ = qdot;
}

void RigidBody6DOF::setAcceleration(const Vector6& qddot) noexcept
{
    qddot_ = qddot;
}

} // namespace ymir
