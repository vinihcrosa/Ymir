#pragma once

#include <vector>

namespace ymir
{

class RigidBody6DOF;
class ForceModel;

/**
 * Abstract integrator interface for 6-DOF rigid body dynamics.
 *
 * Concrete implementations (CvodeIntegrator, RK45Integrator) are
 * owned exclusively by RigidBody6DOF via unique_ptr<IIntegrator>.
 */
class IIntegrator
{
public:
    virtual ~IIntegrator() = default;

    /** Allocate internal state and set initial conditions from body. */
    virtual void initialize(RigidBody6DOF& body, std::vector<ForceModel*>& models) = 0;

    /** Advance body state by dt seconds. */
    virtual void step(RigidBody6DOF& body, double dt) = 0;

    /** Reinitialise from body's current state without reallocation. */
    virtual void reset(RigidBody6DOF& body) = 0;

    virtual double time() const noexcept = 0;
};

} // namespace ymir
