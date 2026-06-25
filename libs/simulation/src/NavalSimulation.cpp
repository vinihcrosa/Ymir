#include <ymir/simulation/NavalSimulation.h>

#include <ymir/common/math/AngleUtils.h>

#include <cmath>

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

NavalSimulation::NavalSimulation(std::unique_ptr<ymir::RigidBody6DOF> body)
{
    body_ptr_ = body.get();
    sim_.addBody(0, std::move(body));
}

void NavalSimulation::addNavalForceModel(std::unique_ptr<NavalForceModel> model)
{
    NavalForceModel* ptr = model.get();
    ptr->bindContext(&ctx_);
    navalModels_.push_back(ptr);
    body_ptr_->addForceModel(std::move(model));
}

void NavalSimulation::initialize()
{
    q_avg_ = sim_.state(0).q();
    ctx_   = buildContext(0.0);
    for (NavalForceModel* m : navalModels_)
        m->bindContext(&ctx_);
}

void NavalSimulation::step(double dt)
{
    ctx_ = buildContext(dt);
    sim_.step(dt);

    constexpr double tau  = 16.5;
    const double     alpha = 1.0 - std::exp(-dt / tau);
    const ymir::Vector6 q = sim_.state(0).q();
    for (int i = 0; i < 6; ++i)
        q_avg_[i] += alpha * (q[i] - q_avg_[i]);
}

void NavalSimulation::reset()
{
    q_avg_ = sim_.state(0).q();
    for (NavalForceModel* m : navalModels_)
        m->resetState();
    body_ptr_->reset();
    ctx_ = buildContext(0.0);
}

void NavalSimulation::setEnvironment(const NavalEnvironment& env)
{
    env_ = env;
}

ymir::BodyState NavalSimulation::state() const
{
    return sim_.state(0);
}

double NavalSimulation::time() const noexcept
{
    return sim_.time();
}

NavalContext NavalSimulation::buildContext(double dt) const
{
    ymir::BodyState bs  = sim_.state(0);
    double          yaw = bs.yaw();

    auto [cu, cv] = nautToBodyFrame(env_.currentSpeed, env_.currentDirectionNaut, yaw);
    auto [wu, wv] = nautToBodyFrame(env_.windSpeed,    env_.windDirectionNaut,    yaw);

    ymir::Vector6 speedToWater{};
    speedToWater[0] = bs.u() - cu;
    speedToWater[1] = bs.v() - cv;
    for (int i = 2; i < 6; ++i)
        speedToWater[i] = bs.qdot()[i];

    ymir::Vector6 speedToWind{};
    speedToWind[0] = wu;
    speedToWind[1] = wv;

    return NavalContext{bs, speedToWater, speedToWind, q_avg_, env_.waterDepth, env_.tide};
}

} // namespace ymir::naval
