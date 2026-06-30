#include <ymir/physics/forces/RudderForces.h>

#include <ymir/physics/forces/ThrustForces.h>
#include <ymir/common/PhysicalConstants.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace ymir::naval
{

namespace
{

// Linear interpolation of column [col] of a {angle_deg, Cl, Cd} table against
// angle_deg. Mirrors MATLAB interp1q (ascending x) but clamps outside the
// tabulated range instead of returning NaN.
double interpCoefficient(const std::vector<std::array<double, 3>>& table,
                         double angle_deg, std::size_t col) noexcept
{
    const std::size_t n = table.size();
    if (n == 0) return 0.0;
    if (angle_deg <= table.front()[0]) return table.front()[col];
    if (angle_deg >= table.back()[0])  return table.back()[col];

    for (std::size_t i = 1; i < n; ++i)
    {
        if (angle_deg <= table[i][0])
        {
            const double a0 = table[i - 1][0], a1 = table[i][0];
            const double c0 = table[i - 1][col], c1 = table[i][col];
            const double t  = (a1 > a0) ? (angle_deg - a0) / (a1 - a0) : 0.0;
            return c0 + t * (c1 - c0);
        }
    }
    return table.back()[col];
}

} // namespace

RudderForces::RudderForces(const Config& cfg, const ThrustForces* thrust)
    : cfg_(cfg)
    , thrust_(thrust)
    , commands_(cfg.rudders.size())
{
}

void RudderForces::setActuatorState(std::size_t id, const RudderCommand& cmd) noexcept
{
    if (id < commands_.size())
        commands_[id] = cmd;
}

Forces RudderForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    Forces fr;

    for (std::size_t i = 0; i < cfg_.rudders.size(); ++i)
    {
        const auto& r   = cfg_.rudders[i];
        const auto& cmd = commands_[i];

        if (r.coefficients.empty()) continue;

        const double delta = cmd.currentAngle_rad;  // rudder angle (rad)

        // --- Longitudinal inflow at the rudder (rudder.m advanceStep) ---------
        // Free-stream surge relative to water, reduced by the propeller hull
        // wake fraction when flowing aft (Va > 0).
        const double Va = ctx.speedToWater[0];
        double Vi = (Va > 0.0) ? Va * r.hullEfficiency : Va;

        double relativeVx = Vi;
        if (thrust_ != nullptr && r.thrusterIdx != std::numeric_limits<std::size_t>::max())
        {
            // Propeller slipstream: p1 geometric factor × p2 actuator-disk term.
            // p2 = √(Vi² + 8|T|/(ρ·π·D²)) − sign(Vi)·Vi, which → 0 as T → 0,
            // so this degrades to Vi when the propeller is not loaded.
            const double T      = thrust_->getThrust(r.thrusterIdx);
            const double D      = r.thrusterDiameter;
            const double signVi = (Vi > 0.0) - (Vi < 0.0);
            const double p2     = std::sqrt(Vi * Vi
                                  + 8.0 * std::abs(T) / (cfg_.rho * M_PI * D * D))
                                  - signVi * Vi;
            relativeVx = Vi + r.p1 * p2;
        }

        // --- Transversal inflow at the rudder --------------------------------
        // Body-frame sway relative to water plus the yaw-rate lever r·x_rudder.
        const double relativeVy = ctx.speedToWater[1] + state.r() * r.position[0];

        const double V2 = relativeVx * relativeVx + relativeVy * relativeVy;
        if (V2 < 1e-12) continue;

        // --- Incidence angle and tabulated coefficients ----------------------
        const double alphaVr = std::atan2(relativeVy, relativeVx);

        double beta_deg = -(delta - alphaVr) * 180.0 / M_PI;
        beta_deg = std::fmod(beta_deg, 360.0);
        if (beta_deg < 0.0) beta_deg += 360.0;

        const double Cl =  interpCoefficient(r.coefficients, beta_deg, 1);
        const double Cd = -interpCoefficient(r.coefficients, beta_deg, 2);

        const double q    = 0.5 * cfg_.rho * r.area * V2;
        const double drag = q * Cd;  // along inflow
        const double lift = q * Cl;  // perpendicular to inflow

        // Resolve drag (along α) and lift (along α − π/2) into body axes.
        const double fx = drag * std::cos(alphaVr) + lift * std::cos(alphaVr - M_PI / 2.0);
        const double fy = drag * std::sin(alphaVr) + lift * std::sin(alphaVr - M_PI / 2.0);

        fr.f[0] += fx;
        fr.f[1] += fy;
        fr.f[3] += -r.position[2] * fy;
        fr.f[4] +=  r.position[2] * fx;
        fr.f[5] +=  r.position[0] * fy - r.position[1] * fx;
    }

    return fr;
}

} // namespace ymir::naval
