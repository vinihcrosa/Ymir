#pragma once

#include <ymir/core/BodyState.h>
#include <ymir/core/integrator/CvodeConfig.h>

#include <memory>
#include <vector>

namespace ymir
{

class Body;
class ForceModel;
class CvodeIntegrator;

/**
 * Top-level composition: one Body + N ForceModels + one CvodeIntegrator.
 *
 * Usage:
 *   auto sim = Simulation(std::move(body), config);
 *   sim.addForceModel(std::make_unique<MyForce>(...));
 *   sim.initialize();
 *   for (...) sim.step(dt);
 */
class Simulation
{
public:
    Simulation(std::unique_ptr<Body> body, const CvodeConfig& config = {});
    ~Simulation();

    Simulation(const Simulation&)            = delete;
    Simulation& operator=(const Simulation&) = delete;

    /** Transfer ownership of a force model into this simulation. */
    void addForceModel(std::unique_ptr<ForceModel> model);

    /** Must be called before step(). */
    void initialize();

    /** Advance simulation by dt seconds. */
    void step(double dt);

    BodyState state() const;
    double    time()  const noexcept { return t_; }

private:
    std::unique_ptr<Body>                    body_;
    std::unique_ptr<CvodeIntegrator>         integrator_;
    std::vector<std::unique_ptr<ForceModel>> ownedModels_;
    std::vector<ForceModel*>                 modelPtrs_;

    double t_           = 0.0;
    bool   initialized_ = false;
};

} // namespace ymir
