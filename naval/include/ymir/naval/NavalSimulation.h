#pragma once

#include <ymir/core/BodyState.h>
#include <ymir/core/RigidBody6DOF.h>
#include <ymir/core/Simulation.h>
#include <ymir/naval/NavalContext.h>
#include <ymir/naval/NavalEnvironment.h>
#include <ymir/naval/NavalForceModel.h>

#include <memory>
#include <vector>

namespace ymir::naval
{

/**
 * Orchestrator for the naval domain.
 *
 * Owns a Simulation (multi-body container) with a single RigidBody6DOF
 * registered at ID 0. Maintains NavalContext and EMA state per step.
 */
class NavalSimulation
{
public:
    explicit NavalSimulation(std::unique_ptr<ymir::RigidBody6DOF> body);

    NavalSimulation(const NavalSimulation&)            = delete;
    NavalSimulation& operator=(const NavalSimulation&) = delete;

    /** Register a naval force model. Force model is added to the body. */
    void addNavalForceModel(std::unique_ptr<NavalForceModel> model);

    /** Bind contexts and prepare EMA state. First step() will init CVODE. */
    void initialize();

    void step(double dt);

    /** Reset CVODE time, EMA, and force model states. Does not reset position. */
    void reset();

    void setEnvironment(const NavalEnvironment& env);

    ymir::BodyState state() const;
    double          time()  const noexcept;

private:
    NavalContext buildContext(double dt) const;

    ymir::Simulation        sim_;
    ymir::RigidBody6DOF*    body_ptr_ = nullptr;  // non-owning; sim_ holds the unique_ptr
    NavalEnvironment        env_{};
    NavalContext            ctx_{};
    ymir::Vector6           q_avg_{};

    std::vector<NavalForceModel*> navalModels_;
};

} // namespace ymir::naval
