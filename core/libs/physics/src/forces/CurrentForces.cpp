#include <ymir/physics/forces/CurrentForces.h>

#include <ymir/common/math/AngleUtils.h>
#include <ymir/common/math/Interpolation.h>
#include <ymir/common/PhysicalConstants.h>

#include <algorithm>
#include <cmath>

namespace ymir::naval
{

CurrentForces::CurrentForces(const Config& cfg) : cfg_(cfg) {}

void CurrentForces::resetState() noexcept
{
    vdp_qz_    = 0.0;
    vdp_dqzdt_ = 0.0;
}

Forces CurrentForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    if (cfg_.model == CurrentModel::OBOKATA)
        return computeObokata(state, ctx);
    return computeRegular(state, ctx);
}

// ---------------------------------------------------------------------------
// OBOKATA — section integration over 50 hull sections
// ---------------------------------------------------------------------------
Forces CurrentForces::computeObokata(const BodyState& state, const NavalContext& ctx)
{
    const double vc_x = ctx.speedToWater[0];
    const double vc_y = ctx.speedToWater[1];
    const double r    = state.r();

    double Fx = 0.0, Fy = 0.0, Mz = 0.0;

    const int n = cfg_.n_sections;
    if (n < 2) return Forces::zero();

    const double dx = cfg_.length_BP / static_cast<double>(n - 1);

    for (int i = 0; i < n; i++)
    {
        // Section x-position in body frame
        double xi = (cfg_.sectionLocalPositions.size() == static_cast<size_t>(n))
                        ? cfg_.sectionLocalPositions[i]
                        : -cfg_.length_BP / 2.0 + i * dx;

        // Water velocity relative to hull section at xi.
        // vessel velocity at section = (vc_x, vc_y + r*xi)  [body-frame rotation]
        // water_vel_relative_to_section = fluid_vel - vessel_vel_at_section
        //   = (cu - vc_x, cv - vc_y) - (0, r*xi)
        //   = (-vc_x_residual, -vc_y_residual - r*xi)   where speedToWater = vesselVel - fluidVel
        // This matches the MATLAB relativeVelocity convention: atan2(-v - r*xi, -u).
        double water_x = -vc_x;
        double water_y = -vc_y - r * xi;

        double v2 = water_x * water_x + water_y * water_y;
        if (v2 < 1e-12) continue;

        // Incidence angle 0..360
        double angle_rad = std::atan2(water_y, water_x);
        double angle_deg = ymir::math::wrapTo360(ymir::math::rad2deg(angle_rad));

        double cdx = ymir::math::linear(cfg_.angles, cfg_.cdx, angle_deg);
        double cdy = ymir::math::linear(cfg_.angles, cfg_.cdy, angle_deg);

        // Local sectional area proportional to section width (uniform assumed)
        double ds_x = cfg_.frontalHeight * dx;
        double ds_y = cfg_.lateralHeight * dx;

        double fx = 0.5 * rho_water * cdx * ds_x * v2;
        double fy = 0.5 * rho_water * cdy * ds_y * v2;

        Fx += fx;
        Fy += fy;
        Mz += fy * xi;
    }

    // Yaw moment centred at wavesOriginPosition
    const double xo = cfg_.wavesOriginPosition[0];
    Mz -= Fy * xo;

    Forces fc;
    fc.f[0] = Fx;
    fc.f[1] = Fy;
    fc.f[5] = Mz;
    return fc;
}

// ---------------------------------------------------------------------------
// REGULAR — area-based with Van der Pol VIM for lateral lift
// ---------------------------------------------------------------------------
Forces CurrentForces::computeRegular(const BodyState& state, const NavalContext& ctx)
{
    const double vc_x = ctx.speedToWater[0];
    const double vc_y = ctx.speedToWater[1];
    const double r    = state.r();

    const double dt_raw = ctx.state.dt();
    const double dt     = (dt_raw > 0.0) ? dt_raw : 0.1;

    double v2 = vc_x * vc_x + vc_y * vc_y;
    if (v2 < 1e-12) return Forces::zero();

    // Same sign convention as OBOKATA: negate to match MATLAB relativeVelocity
    double angle_rad = std::atan2(-vc_y, -vc_x);
    double angle_deg = ymir::math::wrapTo360(ymir::math::rad2deg(angle_rad));

    double cdx = ymir::math::linear(cfg_.angles, cfg_.cdx, angle_deg);
    double cdy = ymir::math::linear(cfg_.angles, cfg_.cdy, angle_deg);

    // Drag forces
    double Fx = 0.5 * rho_water * cdx * cfg_.frontalArea * v2;
    double Fy = 0.5 * rho_water * cdy * cfg_.lateralArea * v2;

    // Van der Pol VIM oscillator for lateral lift (REGULAR mode)
    double v = std::sqrt(v2);

    // Dimensionless acc driving term (lock-in acceleration parameter)
    double acc0 = 2.0 * cfg_.az * v * v / (cfg_.length_BP * cfg_.length_BP);

    // Step VDP oscillator (6 sub-steps)
    stepVdp(dt / 6.0, acc0);
    stepVdp(dt / 6.0, acc0);
    stepVdp(dt / 6.0, acc0);
    stepVdp(dt / 6.0, acc0);
    stepVdp(dt / 6.0, acc0);
    stepVdp(dt / 6.0, acc0);

    // Lift force from VDP state
    double cl_vdp = cfg_.clO * vdp_qz_;
    double Fy_vim = 0.5 * rho_water * cl_vdp * cfg_.lateralArea * v2;

    // Yaw
    const double xo = cfg_.wavesOriginPosition[0];
    double Mz = (Fy + Fy_vim) * (-xo);

    Forces fc;
    fc.f[0] = Fx;
    fc.f[1] = Fy + Fy_vim;
    fc.f[5] = Mz;

    (void)r;  // yaw rate reserved for future section integration in REGULAR
    return fc;
}

// ---------------------------------------------------------------------------
// Van der Pol RK4 (internal)
// ---------------------------------------------------------------------------
void CurrentForces::stepVdp(double h, double acc0)
{
    // State: [qz, dqzdt]
    // dqz/dt       = dqzdt
    // d²qz/dt²     = ez*(1 - qz²)*dqzdt - qz + acc0
    const double ez = cfg_.ez;

    auto f1 = [&](double q, double dq) -> double { return dq; };
    auto f2 = [&](double q, double dq) -> double
    {
        return ez * (1.0 - q * q) * dq - q + acc0;
    };

    double q  = vdp_qz_;
    double dq = vdp_dqzdt_;

    double k1q  = h * f1(q, dq);
    double k1dq = h * f2(q, dq);

    double k2q  = h * f1(q + 0.5 * k1q, dq + 0.5 * k1dq);
    double k2dq = h * f2(q + 0.5 * k1q, dq + 0.5 * k1dq);

    double k3q  = h * f1(q + 0.5 * k2q, dq + 0.5 * k2dq);
    double k3dq = h * f2(q + 0.5 * k2q, dq + 0.5 * k2dq);

    double k4q  = h * f1(q + k3q, dq + k3dq);
    double k4dq = h * f2(q + k3q, dq + k3dq);

    vdp_qz_    = q  + (k1q  + 2.0 * k2q  + 2.0 * k3q  + k4q)  / 6.0;
    vdp_dqzdt_ = dq + (k1dq + 2.0 * k2dq + 2.0 * k3dq + k4dq) / 6.0;
}

} // namespace ymir::naval
