#include <ymir/core/Body.h>
#include <ymir/core/math/LinearAlgebra.h>

namespace ymir
{

Body::Body(const Matrix6x6& massMatrix,
           const Matrix6x6& addedMass,
           const Vector6&   initialQ,
           const Vector6&   initialQdot)
    : massMatrix_(massMatrix)
    , addedMass_(addedMass)
    , totalMass_(math::matAdd(massMatrix, addedMass))
    , invTotalMass_(math::invert6x6(totalMass_))
    , q_(initialQ)
    , qdot_(initialQdot)
    , qddot_{}
{
}

BodyState Body::state(double time, double dt) const noexcept
{
    return BodyState{q_, qdot_, time, dt};
}

Vector6 Body::computeAcceleration(const Forces& totalForce) const noexcept
{
    return math::matVecProduct(invTotalMass_, totalForce.f);
}

void Body::setState(const Vector6& q, const Vector6& qdot) noexcept
{
    q_    = q;
    qdot_ = qdot;
}

void Body::setAcceleration(const Vector6& qddot) noexcept
{
    qddot_ = qddot;
}

} // namespace ymir
