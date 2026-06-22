#include <ymir/naval/NavalSimulation.h>

#include <ymir/core/math/AngleUtils.h>

#include <cmath>

namespace ymir::naval
{

namespace
{

// Convert nautical speed+direction to body-frame (u, v) components.
// nautDeg: "from where", 0=North, 90=East, clockwise.
// Returns the velocity vector as seen by a vessel with the given yaw.
static std::array<double, 2> nautToBodyFrame(double speedMag, double nautDeg, double yawRad)
{
    // "from where" → "toward": add 180°
    double towardNaut = nautDeg + 180.0;
    // Nautical → math angle: math = 90 - naut (X=East, Y=North)
    double mathRad = ymir::math::deg2rad(90.0 - towardNaut);
    double vEast   = speedMag * std::cos(mathRad);
    double vNorth  = speedMag * std::sin(mathRad);
    // Rotate inertial → body: body_x = cos(ψ)·E + sin(ψ)·N
    double vx =  std::cos(yawRad) * vEast + std::sin(yawRad) * vNorth;
    double vy = -std::sin(yawRad) * vEast + std::cos(yawRad) * vNorth;
    return {vx, vy};
}

} // namespace

NavalSimulation::NavalSimulation(std::unique_ptr<ymir::Body> body, const ymir::CvodeConfig& cfg)
    : sim_(std::move(body), cfg)
    , ctx_{}  // default-init (BodyState now has default ctor); will be rebuilt on first step
{
}

void NavalSimulation::addNavalForceModel(std::unique_ptr<NavalForceModel> model)
{
    NavalForceModel* ptr = model.get();
    ptr->bindContext(&ctx_);
    navalModels_.push_back(ptr);
    sim_.addForceModel(std::move(model));  // sim_ takes ownership; ptr remains valid
}

void NavalSimulation::initialize()
{
    const ymir::Vector6& q = sim_.state().q();
    q_avg_ = q;
    sim_.initialize();
    // Build a valid context now that we have a known state
    ctx_ = buildContext(0.0);
    for (NavalForceModel* m : navalModels_)
        m->bindContext(&ctx_);
}

void NavalSimulation::step(double dt)
{
    // Rebuild context from current state + environment
    ctx_ = buildContext(dt);

    // Step the integrator (force models will use ctx_ via their bound pointer)
    sim_.step(dt);

    // Update EMA after the step (new q from integrator)
    constexpr double tau = 16.5;
    const double alpha   = 1.0 - std::exp(-dt / tau);
    const ymir::Vector6& q = sim_.state().q();
    for (int i = 0; i < 6; ++i)
        q_avg_[i] += alpha * (q[i] - q_avg_[i]);
}

void NavalSimulation::reset()
{
    const ymir::Vector6& q = sim_.state().q();
    q_avg_ = q;

    for (NavalForceModel* m : navalModels_)
        m->resetState();

    sim_.initialize();
    ctx_ = buildContext(0.0);
}

void NavalSimulation::setEnvironment(const NavalEnvironment& env)
{
    env_ = env;
}

ymir::BodyState NavalSimulation::state() const
{
    return sim_.state();
}

double NavalSimulation::time() const noexcept
{
    return sim_.time();
}

NavalContext NavalSimulation::buildContext(double dt) const
{
    ymir::BodyState bs = sim_.state();
    double yaw = bs.yaw();

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
