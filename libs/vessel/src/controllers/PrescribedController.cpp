#include <ymir/vessel/controllers/PrescribedController.h>

#include <algorithm>
#include <cassert>

namespace ymir::naval
{

PrescribedController::PrescribedController(Config cfg)
    : cfg_(std::move(cfg))
{}

void PrescribedController::update(double t, double /*dt*/, const NavalContext& /*ctx*/,
                                  std::vector<Thruster>& thrusters,
                                  std::vector<Rudder>&   rudders)
{
    const std::size_t nThrust = std::min(cfg_.thrusterRPM.size(), thrusters.size());
    for (std::size_t i = 0; i < nThrust; ++i)
    {
        const double rpm = interpolate(cfg_.thrusterRPM[i], t);
        thrusters[i].setDemand(rpm, thrusters[i].state().demandedPitch,
                               thrusters[i].state().demandedAzimuth_deg);
    }

    const std::size_t nRudder = std::min(cfg_.rudderAngle_rad.size(), rudders.size());
    for (std::size_t i = 0; i < nRudder; ++i)
    {
        rudders[i].setDemand(interpolate(cfg_.rudderAngle_rad[i], t));
    }
}

double PrescribedController::interpolate(const TimeSeries& ts, double t) noexcept
{
    assert(!ts.times.empty() && ts.times.size() == ts.values.size());

    if (ts.times.size() == 1 || t <= ts.times.front())
        return ts.values.front();

    if (t >= ts.times.back())
        return ts.values.back();

    // lb points to first element >= t
    auto lb = std::lower_bound(ts.times.begin(), ts.times.end(), t);

    // lb != begin (t > front) and lb != end (t < back) — safe to go back one
    const std::size_t hi = static_cast<std::size_t>(lb - ts.times.begin());
    const std::size_t lo = hi - 1;

    const double t0 = ts.times[lo];
    const double t1 = ts.times[hi];
    const double v0 = ts.values[lo];
    const double v1 = ts.values[hi];

    const double alpha = (t - t0) / (t1 - t0);
    return v0 + alpha * (v1 - v0);
}

} // namespace ymir::naval
