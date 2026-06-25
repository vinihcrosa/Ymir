#pragma once

#include <ymir/physics/AbstractBody.h>
#include <ymir/physics/Forces.h>
#include <ymir/common/Types.h>
#include <ymir/physics/integrator/CvodeConfig.h>

#include <memory>
#include <vector>

namespace ymir
{

class CvodeIntegrator;

/**
 * Concrete 6-DOF rigid body with CVODE/BDF integration.
 *
 * Owns its mass matrices, kinematic state, force models, and integrator.
 * CVODE is initialized lazily on the first step() call.
 *
 * Non-copyable and non-movable: CvodeIntegrator::user_data holds an
 * address-stable pointer to its internal RhsContext (see D-10).
 */
class RigidBody6DOF final : public AbstractBody
{
public:
    RigidBody6DOF(int                id,
                  const Matrix6x6&   massMatrix,
                  const Matrix6x6&   addedMass,
                  const Vector6&     initialQ,
                  const Vector6&     initialQdot,
                  const CvodeConfig& config = {});

    ~RigidBody6DOF();

    RigidBody6DOF(const RigidBody6DOF&)            = delete;
    RigidBody6DOF& operator=(const RigidBody6DOF&) = delete;
    RigidBody6DOF(RigidBody6DOF&&)                 = delete;
    RigidBody6DOF& operator=(RigidBody6DOF&&)      = delete;

    // AbstractBody
    void addForceModel(std::unique_ptr<ForceModel> model) override;
    void step(double dt) override;  // lazy CVODE init on first call

    BodyState state() const noexcept override;
    int       id()    const noexcept override { return id_; }

    /** Reset CVODE time to 0, reinitialise from current body state. */
    void reset();

    // Read-only access (used by CvodeIntegrator and tests)
    const Matrix6x6& massMatrix()   const noexcept { return massMatrix_; }
    const Matrix6x6& addedMass()    const noexcept { return addedMass_; }
    const Matrix6x6& totalMass()    const noexcept { return totalMass_; }
    const Matrix6x6& invTotalMass() const noexcept { return invTotalMass_; }

    const Vector6& q()     const noexcept { return q_; }
    const Vector6& qdot()  const noexcept { return qdot_; }
    const Vector6& qddot() const noexcept { return qddot_; }

    double time() const noexcept { return t_; }

    Vector6 computeAcceleration(const Forces& totalForce) const noexcept;

private:
    friend class CvodeIntegrator;

    void setState(const Vector6& q, const Vector6& qdot) noexcept;
    void setAcceleration(const Vector6& qddot) noexcept;

    int       id_;

    Matrix6x6 massMatrix_;
    Matrix6x6 addedMass_;
    Matrix6x6 totalMass_;
    Matrix6x6 invTotalMass_;

    Vector6 q_;
    Vector6 qdot_;
    Vector6 qddot_;

    std::vector<std::unique_ptr<ForceModel>> ownedModels_;
    std::vector<ForceModel*>                 modelPtrs_;

    std::unique_ptr<CvodeIntegrator> integrator_;
    double t_           = 0.0;
    bool   initialized_ = false;
};

} // namespace ymir
