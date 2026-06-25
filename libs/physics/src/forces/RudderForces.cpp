#include <ymir/physics/forces/RudderForces.h>

#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/common/PhysicalConstants.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace ymir::naval
{

RudderForces::RudderForces(const Config& cfg, const ThrustForces* thrust)
    : cfg_(cfg)
    , thrust_(thrust)
    , currentAngle_(cfg.rudders.size(), 0.0)
{
    for (std::size_t i = 0; i < cfg_.rudders.size(); ++i)
        currentAngle_[i] = cfg_.rudders[i].angle_deg;
}

Forces RudderForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    Forces fr;
    const double dt = state.dt();

    for (std::size_t i = 0; i < cfg_.rudders.size(); ++i)
    {
        const auto& r = cfg_.rudders[i];

        // Rate-limited rudder dynamics
        if (dt > 0.0 && r.rateLimit > 0.0)
        {
            double error  = r.angle_deg - currentAngle_[i];
            double max_dg = r.rateLimit * dt;
            double delta  = std::max(-max_dg, std::min(max_dg, error));
            currentAngle_[i] += delta;
        }
        else
        {
            currentAngle_[i] = r.angle_deg;
        }

        double alpha_rad = currentAngle_[i] * M_PI / 180.0;
        if (std::abs(alpha_rad) < 1e-9) continue;

        // Inflow speed at rudder (body frame u + optional slipstream)
        double Va = ctx.speedToWater[0];

        if (thrust_ != nullptr && r.thrusterIdx != std::numeric_limits<std::size_t>::max())
        {
            // Actuator disk wake: V_wake = sqrt(Va² + 2·T/(rho·A_disk))
            double T_prop = thrust_->getThrust(r.thrusterIdx);
            double A_disk = M_PI / 4.0 * r.area;  // approximate
            double Va2    = Va * Va + 2.0 * std::abs(T_prop) / (cfg_.rho * A_disk);
            Va = (T_prop >= 0.0) ? std::sqrt(Va2) : -std::sqrt(Va2);
        }

        double v2 = Va * Va;
        if (v2 < 1e-12) continue;

        // Lift slope with aspect ratio correction (Prandtl)
        double AR = r.aspectRatio;
        double CL_alpha = 2.0 * M_PI * AR / (AR + 2.0);
        double CL = CL_alpha * alpha_rad;
        double CD = CL * CL / (M_PI * AR);  // induced drag

        double q_dyn = 0.5 * cfg_.rho * v2;
        double L = q_dyn * r.area * CL;
        double D = q_dyn * r.area * CD;

        // Lift acts perpendicular to inflow (sway), drag acts along inflow (surge)
        double sign_Va = (Va >= 0.0) ? 1.0 : -1.0;
        double fy = -L * sign_Va;  // lift opposes turning
        double fx = -D * sign_Va;  // drag opposes motion

        fr.f[0] += fx;
        fr.f[1] += fy;
        fr.f[3] += r.position[1] * 0.0 - r.position[2] * fy;
        fr.f[4] += r.position[2] * fx  - r.position[0] * 0.0;
        fr.f[5] += r.position[0] * fy  - r.position[1] * fx;
    }

    return fr;
}

} // namespace ymir::naval
