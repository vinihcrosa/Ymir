#include <ymir/physics/forces/TugForces.h>

#include <ymir/common/math/LinearAlgebra.h>
#include <ymir/common/math/Interpolation.h>
#include <ymir/common/PhysicalConstants.h>

#include <algorithm>
#include <cmath>

namespace ymir::naval
{

TugForces::TugForces(const Config& cfg) : cfg_(cfg) {}

Forces TugForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    Forces ft;

    for (const auto& t : cfg_.tugs)
    {
        if (t.bollardPull < 1e-9) continue;

        double forceMag = t.bollardPull;

        if (t.mode == TugMode::PUSH)
        {
            // Speed-dependent reduction: tanh curve
            double v = std::sqrt(ctx.speedToWater[0] * ctx.speedToWater[0]
                               + ctx.speedToWater[1] * ctx.speedToWater[1]);
            double factor = 1.0 - std::tanh(t.speedPushK * v);
            forceMag *= factor;
        }
        else if (t.mode == TugMode::PULL)
        {
            if (!t.pullSpeeds.empty() && !t.pullAngles.empty() && !t.pullTable.empty())
            {
                double v = std::sqrt(ctx.speedToWater[0] * ctx.speedToWater[0]
                                   + ctx.speedToWater[1] * ctx.speedToWater[1]);
                double a_deg = t.angle_deg;
                forceMag = ymir::math::bilinear(t.pullSpeeds, t.pullAngles, t.pullTable, v, a_deg);
            }
        }
        // ESCORTING: use bollardPull directly (simplified)

        double az_rad = t.angle_deg * M_PI / 180.0;
        double fx = forceMag * std::cos(az_rad);
        double fy = forceMag * std::sin(az_rad);
        double fz = 0.0;

        ft.f[0] += fx;
        ft.f[1] += fy;
        ft.f[2] += fz;

        ft.f[3] += t.position[1] * fz - t.position[2] * fy;
        ft.f[4] += t.position[2] * fx - t.position[0] * fz;
        ft.f[5] += t.position[0] * fy - t.position[1] * fx;
    }

    return ft;
}

} // namespace ymir::naval
