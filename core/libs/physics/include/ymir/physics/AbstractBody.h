#pragma once

#include <ymir/physics/BodyState.h>
#include <ymir/physics/ForceModel.h>

#include <memory>

namespace ymir
{

/**
 * Interface for any physical body in the simulation.
 *
 * A body has no lifecycle concept: it is always ready to receive the next
 * step(). Initialization and initial state are implementation details of
 * concrete classes. Time is owned by Simulation, not by the body.
 */
class AbstractBody
{
public:
    virtual ~AbstractBody() = default;

    virtual void addForceModel(std::unique_ptr<ForceModel> model) = 0;

    /** Advance this body by dt seconds. */
    virtual void step(double dt) = 0;

    /** Immutable snapshot of current kinematics. */
    virtual BodyState state() const noexcept = 0;

    virtual int id() const noexcept = 0;

    AbstractBody(const AbstractBody&)            = delete;
    AbstractBody& operator=(const AbstractBody&) = delete;

protected:
    AbstractBody() = default;
};

} // namespace ymir
