#pragma once

#include <ymir/core/Body.h>
#include <ymir/core/BodyState.h>
#include <ymir/core/Simulation.h>
#include <ymir/core/Types.h>
#include <ymir/core/integrator/CvodeConfig.h>
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
 * Wraps core::Simulation, maintains NavalContext, and updates the EMA state
 * of q_avg each step. Binds all NavalForceModel context pointers before step.
 *
 * Declaration order is significant: sim_ must appear before ctx_ so that
 * ctx_ can be initialized with sim_.state() in the constructor initializer list.
 */
class NavalSimulation
{
public:
    explicit NavalSimulation(std::unique_ptr<ymir::Body> body,
                             const ymir::CvodeConfig& cfg = {});

    NavalSimulation(const NavalSimulation&)            = delete;
    NavalSimulation& operator=(const NavalSimulation&) = delete;

    /** Register a naval force model. sim_ takes ownership; raw ptr stored for rebinding. */
    void addNavalForceModel(std::unique_ptr<NavalForceModel> model);

    void initialize();
    void step(double dt);
    void reset();

    void setEnvironment(const NavalEnvironment& env);

    ymir::BodyState state() const;
    double          time()  const noexcept;

private:
    NavalContext buildContext(double dt) const;

    // sim_ MUST be declared before ctx_ — initialization order follows declaration order.
    ymir::Simulation              sim_;
    NavalEnvironment              env_{};
    NavalContext                  ctx_;   // initialized in constructor body after sim_
    ymir::Vector6                 q_avg_{};

    std::vector<NavalForceModel*> navalModels_;  // non-owning; sim_ holds unique_ptrs
};

} // namespace ymir::naval
