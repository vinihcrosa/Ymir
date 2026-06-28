#include <ymir/simulation/NavalDomain.h>

#include <ymir/common/math/AngleUtils.h>

#include <cassert>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace ymir {

namespace
{

static std::array<double, 2> nautToBodyFrame(double speedMag, double nautDeg, double yawRad)
{
    double towardNaut = nautDeg + 180.0;
    double mathRad    = ymir::math::deg2rad(90.0 - towardNaut);
    double vEast      = speedMag * std::cos(mathRad);
    double vNorth     = speedMag * std::sin(mathRad);
    double vx         =  std::cos(yawRad) * vEast + std::sin(yawRad) * vNorth;
    double vy         = -std::sin(yawRad) * vEast + std::cos(yawRad) * vNorth;
    return {vx, vy};
}

} // namespace

NavalDomain::NavalDomain(std::string name)
    : name_(std::move(name))
{}

void NavalDomain::addBody(int id, std::unique_ptr<ymir::RigidBody6DOF> body)
{
    BodyEntry& entry = entries_[id];
    entry.body = body.get();
    sim_.addBody(id, std::move(body));
}

void NavalDomain::addNavalForceModel(int bodyId, std::unique_ptr<naval::NavalForceModel> model)
{
    BodyEntry& entry = entries_.at(bodyId);
    naval::NavalForceModel* ptr = model.get();
    ptr->bindContext(&entry.ctx);
    entry.models.push_back(ptr);
    entry.body->addForceModel(std::move(model));
}

void NavalDomain::registerVessel(int bodyId, naval::DynamicVessel& vessel,
                                 naval::ThrustForces* tf, naval::RudderForces* rf)
{
    BodyEntry& entry = entries_.at(bodyId);
    entry.vessel = &vessel;
    entry.thrust = tf;
    entry.rudder = rf;
}

void NavalDomain::initialize()
{
    for (auto& [id, entry] : entries_)
    {
        entry.q_avg = sim_.state(id).q();
        if (env_ != nullptr)
            entry.ctx = buildContext(id, entry, 0.0);
        for (naval::NavalForceModel* m : entry.models)
            m->bindContext(&entry.ctx);
    }
}

void NavalDomain::onAddedToWorld(Environment& env, CouplingRegistry& coupling)
{
    assert(!addedToWorld_ && "NavalDomain::onAddedToWorld called more than once");
    env_          = &env;
    coupling_     = &coupling;
    addedToWorld_ = true;
}

void NavalDomain::step(double dt)
{
    assert(env_ != nullptr && "NavalDomain::step called without onAddedToWorld");

    const double t = sim_.time();

    for (auto& [id, entry] : entries_)
        entry.ctx = buildContext(id, entry, dt);

    for (auto& [id, entry] : entries_)
    {
        if (entry.vessel)
        {
            entry.vessel->updateControl(t, dt, entry.ctx);
            entry.vessel->updateStates(dt);
            entry.vessel->syncToForceModels(entry.thrust, entry.rudder);
        }
    }

    sim_.step(dt);

    constexpr double tau = 16.5;
    for (auto& [id, entry] : entries_)
    {
        const double alpha = 1.0 - std::exp(-dt / tau);
        const ymir::Vector6 q = sim_.state(id).q();
        for (int i = 0; i < 6; ++i)
            entry.q_avg[i] += alpha * (q[i] - entry.q_avg[i]);
    }
}

std::vector<BodyPosition> NavalDomain::allBodyPositions() const
{
    std::vector<BodyPosition> result;
    result.reserve(entries_.size());
    for (const auto& [id, entry] : entries_)
    {
        const BodyState bs = sim_.state(id);
        const auto& q = bs.q();
        result.push_back(BodyPosition{id, q[0], q[1], q[2]});
    }
    return result;
}

BodyState NavalDomain::bodyState(int id) const
{
    return state(id);
}

std::string NavalDomain::name() const
{
    return name_;
}

double NavalDomain::distanceBetween(int idA, int idB) const
{
    assert(entries_.count(idA) && "NavalDomain::distanceBetween: unknown idA");
    assert(entries_.count(idB) && "NavalDomain::distanceBetween: unknown idB");

    const BodyState bsA = sim_.state(idA);
    const BodyState bsB = sim_.state(idB);
    const auto& qA = bsA.q();
    const auto& qB = bsB.q();
    const double dx = qA[0] - qB[0];
    const double dy = qA[1] - qB[1];
    const double dz = qA[2] - qB[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void NavalDomain::reset()
{
    for (auto& [id, entry] : entries_)
    {
        entry.q_avg = sim_.state(id).q();
        for (naval::NavalForceModel* m : entry.models)
            m->resetState();
        entry.body->reset();
        if (env_ != nullptr)
            entry.ctx = buildContext(id, entry, 0.0);
    }
}

BodyState NavalDomain::state(int bodyId) const
{
    if (entries_.find(bodyId) == entries_.end())
        throw std::out_of_range("NavalDomain::state: unknown bodyId");
    return sim_.state(bodyId);
}

double NavalDomain::time() const noexcept
{
    return sim_.time();
}

naval::NavalContext NavalDomain::buildContext(int id, const BodyEntry& entry, double dt) const
{
    ymir::BodyState bs  = sim_.state(id);
    double          yaw = bs.yaw();

    auto [cu, cv] = nautToBodyFrame(env_->currentSpeed(), env_->currentDirectionNaut(), yaw);
    auto [wu, wv] = nautToBodyFrame(env_->windSpeed(),    env_->windDirectionNaut(),    yaw);

    assert(env_->waterDepth() > 0.0 && "NavalDomain: waterDepth must be > 0");

    ymir::Vector6 speedToWater{};
    speedToWater[0] = bs.u() - cu;
    speedToWater[1] = bs.v() - cv;
    for (int i = 2; i < 6; ++i)
        speedToWater[i] = bs.qdot()[i];

    ymir::Vector6 speedToWind{};
    speedToWind[0] = wu;
    speedToWind[1] = wv;

    return naval::NavalContext{bs, speedToWater, speedToWind, entry.q_avg,
                               env_->waterDepth(), env_->tide()};
}

std::string NavalDomain::serializeStateJson() const
{
    std::ostringstream os;
    os << "{\"t\":" << sim_.time() << ",\"vessels\":[";

    bool first = true;
    for (const auto& [id, entry] : entries_)
    {
        const BodyState s = sim_.state(id);
        const Vector6&  q = s.q();
        const Vector6& qd = s.qdot();

        if (!first) os << ',';
        first = false;

        os << "{\"id\":"   << id
           << ",\"x\":"    << q[0]
           << ",\"y\":"    << q[1]
           << ",\"z\":"    << q[2]
           << ",\"phi\":"  << q[3]
           << ",\"theta\":" << q[4]
           << ",\"psi\":"  << q[5]
           << ",\"u\":"    << qd[0]
           << ",\"v\":"    << qd[1]
           << ",\"r\":"    << qd[5]
           << '}';
    }

    os << "]}";
    return os.str();
}

} // namespace ymir
