#include <ymir/core/Simulation.h>

#include <ymir/core/Body.h>
#include <ymir/core/ForceModel.h>
#include <ymir/core/integrator/CvodeIntegrator.h>

#include <stdexcept>

namespace ymir
{

Simulation::Simulation(std::unique_ptr<Body> body, const CvodeConfig& config)
    : body_(std::move(body))
    , integrator_(std::make_unique<CvodeIntegrator>(config))
{
}

Simulation::~Simulation() = default;

void Simulation::addForceModel(std::unique_ptr<ForceModel> model)
{
    modelPtrs_.push_back(model.get());
    ownedModels_.push_back(std::move(model));
}

void Simulation::initialize()
{
    integrator_->initialize(*body_, modelPtrs_);
    t_           = 0.0;
    initialized_ = true;
}

void Simulation::step(double dt)
{
    if (!initialized_)
        throw std::logic_error("Simulation::step called before initialize()");

    integrator_->step(*body_, dt);
    t_ += dt;
}

BodyState Simulation::state() const
{
    return body_->state(t_, 0.0);
}

} // namespace ymir
