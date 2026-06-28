#include <ymir/simulation/NavalSimulation.h>

#include <ymir/common/math/AngleUtils.h>

#include <cmath>
#include <stdexcept>

namespace ymir::naval
{

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

void NavalSimulation::addBody(int id, std::unique_ptr<ymir::RigidBody6DOF> body)
{
    BodyEntry& entry = entries_[id];
    entry.body = body.get();
    sim_.addBody(id, std::move(body));
}

void NavalSimulation::addNavalForceModel(int bodyId, std::unique_ptr<NavalForceModel> model)
{
    BodyEntry& entry = entries_.at(bodyId);
    NavalForceModel* ptr = model.get();
    ptr->bindContext(&entry.ctx);
    entry.models.push_back(ptr);
    entry.body->addForceModel(std::move(model));
}

void NavalSimulation::registerVessel(int bodyId, DynamicVessel& vessel,
                                     ThrustForces* tf, RudderForces* rf)
{
    BodyEntry& entry = entries_.at(bodyId);
    entry.vessel = &vessel;
    entry.thrust = tf;
    entry.rudder = rf;
}

void NavalSimulation::initialize()
{
    for (auto& [id, entry] : entries_)
    {
        entry.q_avg = sim_.state(id).q();
        entry.ctx   = buildContext(id, entry, 0.0);
        for (NavalForceModel* m : entry.models)
            m->bindContext(&entry.ctx);
    }
}

void NavalSimulation::step(double dt)
{
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

void NavalSimulation::reset()
{
    for (auto& [id, entry] : entries_)
    {
        entry.q_avg = sim_.state(id).q();
        for (NavalForceModel* m : entry.models)
            m->resetState();
        entry.body->reset();
        entry.ctx = buildContext(id, entry, 0.0);
    }
}

void NavalSimulation::setEnvironment(const ymir::Environment& env)
{
    env_ = env;
}

ymir::BodyState NavalSimulation::state(int bodyId) const
{
    if (entries_.find(bodyId) == entries_.end())
        throw std::out_of_range("NavalSimulation::state: unknown bodyId");
    return sim_.state(bodyId);
}

double NavalSimulation::time() const noexcept
{
    return sim_.time();
}

NavalContext NavalSimulation::buildContext(int id, const BodyEntry& entry, double dt) const
{
    ymir::BodyState bs  = sim_.state(id);
    double          yaw = bs.yaw();

    auto [cu, cv] = nautToBodyFrame(env_.currentSpeed(), env_.currentDirectionNaut(), yaw);
    auto [wu, wv] = nautToBodyFrame(env_.windSpeed(),    env_.windDirectionNaut(),    yaw);

    ymir::Vector6 speedToWater{};
    speedToWater[0] = bs.u() - cu;
    speedToWater[1] = bs.v() - cv;
    for (int i = 2; i < 6; ++i)
        speedToWater[i] = bs.qdot()[i];

    ymir::Vector6 speedToWind{};
    speedToWind[0] = wu;
    speedToWind[1] = wv;

    return NavalContext{bs, speedToWater, speedToWind, entry.q_avg,
                        env_.waterDepth(), env_.tide()};
}

} // namespace ymir::naval
