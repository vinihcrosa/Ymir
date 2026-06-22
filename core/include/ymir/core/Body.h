#pragma once

#include <ymir/core/BodyState.h>
#include <ymir/core/Forces.h>
#include <ymir/core/Types.h>

namespace ymir
{

class CvodeIntegrator;

/**
 * A rigid body with 6-DOF dynamics.
 *
 * Owns its mass matrices and current kinematic state.
 * The inverse total mass matrix (M + A)^-1 is computed once in the
 * constructor and reused every integration step.
 *
 * Mutation of q/qdot is restricted to CvodeIntegrator (friend).
 * External code reads state via state() → BodyState (immutable snapshot).
 */
class Body
{
public:
    Body(const Matrix6x6& massMatrix,
         const Matrix6x6& addedMass,
         const Vector6&   initialQ,
         const Vector6&   initialQdot);

    Body(const Body&)            = delete;
    Body& operator=(const Body&) = delete;
    Body(Body&&)                 = default;

    /** Immutable snapshot of current kinematics. */
    BodyState state(double time, double dt) const noexcept;

    /** qddot = invTotalMass * Ft */
    Vector6 computeAcceleration(const Forces& totalForce) const noexcept;

    // Read-only access to matrices
    const Matrix6x6& massMatrix()    const noexcept { return massMatrix_; }
    const Matrix6x6& addedMass()     const noexcept { return addedMass_; }
    const Matrix6x6& totalMass()     const noexcept { return totalMass_; }
    const Matrix6x6& invTotalMass()  const noexcept { return invTotalMass_; }

    const Vector6& q()     const noexcept { return q_; }
    const Vector6& qdot()  const noexcept { return qdot_; }
    const Vector6& qddot() const noexcept { return qddot_; }

private:
    friend class CvodeIntegrator;

    void setState(const Vector6& q, const Vector6& qdot) noexcept;
    void setAcceleration(const Vector6& qddot) noexcept;

    Matrix6x6 massMatrix_;
    Matrix6x6 addedMass_;
    Matrix6x6 totalMass_;
    Matrix6x6 invTotalMass_;

    Vector6 q_;
    Vector6 qdot_;
    Vector6 qddot_;
};

} // namespace ymir
