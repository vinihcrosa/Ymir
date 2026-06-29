#include <ymir/world/World.h>

#include <cassert>
#include <stdexcept>
#include <utility>

namespace ymir {

void World::step(double dt)
{
    time_ += dt;
    timeline_.advanceStep(time_, env_);
    coupling_.reset();
    for (auto& d : domains_) d->step(dt);
    coupling_.resolve();
}

void World::addDomain(std::unique_ptr<IDomain> domain)
{
    assert(domain != nullptr);
    const std::string newName = domain->name();
    for ([[maybe_unused]] const auto& d : domains_)
        assert(d->name() != newName && "World::addDomain: duplicate domain name");

    domain->onAddedToWorld(env_, coupling_);
    domains_.push_back(std::move(domain));
}

IDomain& World::domain(const std::string& name)
{
    for (const auto& d : domains_)
        if (d->name() == name)
            return *d;
    throw std::out_of_range("World::domain: unknown domain name");
}

const IDomain& World::domain(const std::string& name) const
{
    for (const auto& d : domains_)
        if (d->name() == name)
            return *d;
    throw std::out_of_range("World::domain: unknown domain name");
}

WorldSnapshot World::snapshot() const
{
    WorldSnapshot s;
    s.simTime = time_;
    s.environment = {env_.windSpeed(), env_.windDirectionNaut(),
                     env_.currentSpeed(), env_.currentDirectionNaut(),
                     env_.waterDepth(), env_.tide()};
    for (const auto& d : domains_) {
        DomainSnapshot ds;
        ds.name = d->name();
        for (const auto& bp : d->allBodyPositions())
            ds.bodies.push_back({bp.id, d->bodyState(bp.id)});
        s.domains.push_back(std::move(ds));
    }
    return s;
}

} // namespace ymir
