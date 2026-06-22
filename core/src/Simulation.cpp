#include <ymir/core/Simulation.h>

#include <stdexcept>
#include <string>

namespace ymir
{

void Simulation::addBody(int id, std::unique_ptr<AbstractBody> body)
{
    if (bodies_.count(id))
        throw std::invalid_argument(
            "Simulation::addBody: duplicate id " + std::to_string(id));

    order_.push_back(body.get());
    bodies_.emplace(id, std::move(body));
}

void Simulation::step(double dt)
{
    for (AbstractBody* b : order_)
        b->step(dt);
    t_ += dt;
}

AbstractBody& Simulation::body(int id)
{
    auto it = bodies_.find(id);
    if (it == bodies_.end())
        throw std::out_of_range(
            "Simulation::body: id not found " + std::to_string(id));
    return *it->second;
}

const AbstractBody& Simulation::body(int id) const
{
    auto it = bodies_.find(id);
    if (it == bodies_.end())
        throw std::out_of_range(
            "Simulation::body: id not found " + std::to_string(id));
    return *it->second;
}

BodyState Simulation::state(int id) const
{
    return body(id).state();
}

bool Simulation::hasBody(int id) const noexcept
{
    return bodies_.count(id) > 0;
}

} // namespace ymir
