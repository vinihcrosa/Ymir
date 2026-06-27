#include <ymir/physics/forces/ThrustForces.h>

#include <ymir/common/PhysicalConstants.h>

#include <algorithm>
#include <cmath>

namespace ymir::naval
{

ThrustForces::ThrustForces(const Config& cfg)
    : cfg_(cfg)
    , commands_(cfg.thrusters.size())
    , lastThrust_(cfg.thrusters.size(), 0.0)
{
    for (std::size_t i = 0; i < cfg_.thrusters.size(); ++i)
    {
        commands_[i].currentRPM         = cfg_.thrusters[i].initialRPM;
        commands_[i].currentAzimuth_deg = cfg_.thrusters[i].azimuth_deg;
        commands_[i].currentPitch       = cfg_.thrusters[i].pitchRatio;
    }
}

void ThrustForces::setActuatorState(std::size_t id, const ThrusterCommand& cmd) noexcept
{
    if (id < commands_.size())
        commands_[id] = cmd;
}

double ThrustForces::getThrust(std::size_t id) const noexcept
{
    if (id >= lastThrust_.size()) return 0.0;
    return lastThrust_[id];
}

Forces ThrustForces::computeNaval(const BodyState& /*state*/, const NavalContext& /*ctx*/)
{
    Forces ft;

    for (std::size_t i = 0; i < cfg_.thrusters.size(); ++i)
    {
        const auto& t   = cfg_.thrusters[i];
        const auto& cmd = commands_[i];

        double n = cmd.currentRPM / 60.0;  // rev/s
        if (std::abs(n) < 1e-9)
        {
            lastThrust_[i] = 0.0;
            continue;
        }

        // Open-water thrust: T = rho * n^2 * D^4 * Kt
        double D  = t.diameter;
        double PD = cmd.currentPitch;
        double Kt = 0.4 * PD - 0.1;  // linear approximation, valid ~0.5 < P/D < 1.2
        Kt = std::max(0.05, std::min(Kt, 0.5));

        double sign_n = (n >= 0.0) ? 1.0 : -1.0;
        double T = sign_n * cfg_.rho * n * n * D * D * D * D * Kt;
        lastThrust_[i] = T;

        // Resolve force into body frame using current azimuth
        double az_rad = cmd.currentAzimuth_deg * M_PI / 180.0;
        double fx = T * std::cos(az_rad);
        double fy = T * std::sin(az_rad);
        double fz = 0.0;

        ft.f[0] += fx;
        ft.f[1] += fy;
        ft.f[2] += fz;

        // Moments about body origin
        ft.f[3] += t.position[1] * fz - t.position[2] * fy;
        ft.f[4] += t.position[2] * fx - t.position[0] * fz;
        ft.f[5] += t.position[0] * fy - t.position[1] * fx;
    }

    return ft;
}

} // namespace ymir::naval
