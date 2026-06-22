#include <ymir/naval/forces/WindForces.h>

#include <ymir/core/math/AngleUtils.h>
#include <ymir/core/math/Interpolation.h>
#include <ymir/naval/PhysicalConstants.h>

#include <cmath>

namespace ymir::naval
{

WindForces::WindForces(const Config& cfg) : cfg_(cfg) {}

Forces WindForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    const double r   = state.r();  // yaw rate
    const auto& orig = cfg_.wavesOriginPosition;

    // Apparent wind in body frame, corrected for yaw-rate rotation
    double vwd_x = ctx.speedToWind[0] - r * orig[1];
    double vwd_y = ctx.speedToWind[1] + r * orig[0];

    double v2 = vwd_x * vwd_x + vwd_y * vwd_y;
    if (v2 < 1e-12) return Forces::zero();

    // Incidence angle 0..360 degrees
    double angle_rad = std::atan2(vwd_y, vwd_x);
    double angle_deg = ymir::math::wrapTo360(ymir::math::rad2deg(angle_rad));

    double cdx = ymir::math::linear(cfg_.angles, cfg_.cdx, angle_deg);
    double cdy = ymir::math::linear(cfg_.angles, cfg_.cdy, angle_deg);
    double cdz = ymir::math::linear(cfg_.angles, cfg_.cdz, angle_deg);

    // Effective areas
    double f_area = cfg_.frontalArea;
    double l_area = cfg_.lateralArea;
    if (cfg_.model == WindModel::ACSINKAGE)
    {
        double dz = state.z() - (orig[2] - cfg_.draft);
        f_area += cfg_.beam   * dz;
        l_area += cfg_.length_BP * dz;
    }

    Forces fwd;
    fwd.f[0] = 0.5 * rho_air * cdx * f_area * v2;
    fwd.f[1] = 0.5 * rho_air * cdy * l_area * v2;

    const double roll  = state.roll();
    const double pitch = state.pitch();
    const double cc    = std::cos(roll) * std::cos(pitch);

    fwd.f[5] = 0.5 * rho_air * cdz * l_area * cfg_.length_BP * v2
             + fwd.f[1] * (cfg_.midshipDistance - orig[0]);
    fwd.f[3] = -fwd.f[1] * (cfg_.frontalHeight - orig[2]) * cc;
    fwd.f[4] =  fwd.f[0] * (cfg_.lateralHeight - orig[2]) * cc;

    return fwd;
}

} // namespace ymir::naval
