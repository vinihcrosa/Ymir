#include <ymir/physics/forces/ThrustForces.h>

#include <ymir/common/PhysicalConstants.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace ymir::naval
{

namespace
{

constexpr double kDeg2Rad = M_PI / 180.0;

// Clamped piecewise-linear interpolation on an ascending-J open-water table,
// mirroring dynamics/Interpolador.m + the J clamp in thruster.m::interpkqkt.
// curve entries are {J, Kq, Kt}; col == 1 picks Kq, col == 2 picks Kt.
double interpColumn(const std::vector<std::array<double, 3>>& curve, double J, std::size_t col)
{
    const std::size_t n = curve.size();
    if (n == 0) return 0.0;
    if (n == 1) return curve[0][col];

    // Clamp J into table range (flat extrapolation, as in MATLAB).
    if (J <= curve.front()[0]) return curve.front()[col];
    if (J >= curve.back()[0])  return curve.back()[col];

    for (std::size_t k = 0; k + 1 < n; ++k)
    {
        const double J0 = curve[k][0];
        const double J1 = curve[k + 1][0];
        if (J >= J0 && J <= J1)
        {
            const double span = J1 - J0;
            const double t    = (span > 0.0) ? (J - J0) / span : 0.0;
            return curve[k][col] * (1.0 - t) + curve[k + 1][col] * t;
        }
    }
    return curve.back()[col];
}

} // namespace

ThrustForces::ThrustForces(const Config& cfg)
    : cfg_(cfg)
    , commands_(cfg.thrusters.size())
    , lastThrust_(cfg.thrusters.size(), 0.0)
    , lastTorque_(cfg.thrusters.size(), 0.0)
    , lastPower_(cfg.thrusters.size(), 0.0)
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

double ThrustForces::getTorque(std::size_t id) const noexcept
{
    if (id >= lastTorque_.size()) return 0.0;
    return lastTorque_[id];
}

double ThrustForces::getPower(std::size_t id) const noexcept
{
    if (id >= lastPower_.size()) return 0.0;
    return lastPower_[id];
}

Forces ThrustForces::computeNaval(const BodyState& /*state*/, const NavalContext& ctx)
{
    Forces ft;

    for (std::size_t i = 0; i < cfg_.thrusters.size(); ++i)
    {
        const auto& t   = cfg_.thrusters[i];
        const auto& cmd = commands_[i];

        const double D  = t.diameter;
        const double n  = cmd.currentRPM / 60.0;  // rev/s (already RPM-lagged by Thruster entity)

        lastThrust_[i] = 0.0;
        lastTorque_[i] = 0.0;
        lastPower_[i]  = 0.0;

        const double az_rad = cmd.currentAzimuth_deg * kDeg2Rad;

        // -------------------------------------------------------------------
        // Legacy linear-Kt fallback (no open-water table supplied).
        // -------------------------------------------------------------------
        if (t.openWaterCurve.empty())
        {
            if (std::abs(n) < 1e-9)
                continue;

            double Kt = 0.4 * cmd.currentPitch - 0.1;  // valid ~0.5 < P/D < 1.2
            Kt = std::max(0.05, std::min(Kt, 0.5));

            const double sign_n = (n >= 0.0) ? 1.0 : -1.0;
            const double T = sign_n * cfg_.rho * n * n * D * D * D * D * Kt;
            lastThrust_[i] = T;

            const double fx = T * std::cos(az_rad);
            const double fy = T * std::sin(az_rad);
            ft.f[0] += fx;
            ft.f[1] += fy;
            ft.f[3] += -t.position[2] * fy;
            ft.f[4] +=  t.position[2] * fx;
            ft.f[5] +=  t.position[0] * fy - t.position[1] * fx;
            continue;
        }

        const double pitch = cmd.currentPitch;  // pitch term in J denominator + thrust sign
        if (std::abs(n) < 1e-9 || std::abs(pitch) < 1e-9)
            continue;

        // -------------------------------------------------------------------
        // Advance / transverse inflow at the thruster, in thruster-local axes.
        // Body-frame velocity relative to current (ctx.speedToWater) plus the
        // yaw-rate lever arm, rotated by the thruster azimuth.
        //   thruster.m advanceStep ~207-222 + speedThroughWaterUpdate.
        // -------------------------------------------------------------------
        const auto&  sw = ctx.speedToWater;
        const double r  = sw[5];                      // yaw rate (rad/s)
        const double px = t.position[0];
        const double py = t.position[1];
        const double vpx = sw[0] - r * py;
        const double vpy = sw[1] + r * px;
        const double cosA = std::cos(az_rad);
        const double sinA = std::sin(az_rad);
        const double vaAxial = cosA * vpx - sinA * vpy;   // speed(1) — advance velocity
        const double vtTrans = sinA * vpx + cosA * vpy;   // speed(2) — transverse inflow

        // Advance ratio J = Va / (pitch · n · D); clamping handled in interpColumn.
        auto advanceRatio = [&](double rev) {
            const double denom = pitch * rev * D;
            return (std::abs(denom) < 1e-12) ? 0.0 : vaAxial / denom;
        };

        const double maxPower = t.maximumPowerW;
        const double mechEff  = (t.mechanicalEfficiency > 1e-9) ? t.mechanicalEfficiency : 1.0;

        // -------------------------------------------------------------------
        // Power-saturation loop (thruster.m ~243-262): shed 10% rotation per
        // iteration until shaft power fits under the available power ceiling.
        // -------------------------------------------------------------------
        double nEff = n;
        auto shaftPower = [&](double rev) {
            const double Kq  = interpColumn(t.openWaterCurve, advanceRatio(rev), 1);
            const double tq  = cfg_.rho * std::abs(rev) * rev * std::pow(D, 5) * Kq;
            return 2.0 * M_PI * rev * tq / mechEff;
        };

        for (int counter = 0; counter < 60 && shaftPower(nEff) > maxPower; ++counter)
            nEff -= nEff * 0.10;

        if (maxPower == 0.0)
            nEff = 0.0;

        // -------------------------------------------------------------------
        // Forces from the (possibly saturated) effective rotation.
        // -------------------------------------------------------------------
        const double J  = advanceRatio(nEff);
        const double Kq = interpColumn(t.openWaterCurve, J, 1);
        const double Kt = interpColumn(t.openWaterCurve, J, 2);

        const double torque = cfg_.rho * std::abs(nEff) * nEff * std::pow(D, 5) * Kq;
        const double power  = 2.0 * M_PI * nEff * torque / mechEff;

        // Transverse-speed reduction `reduc` (thruster.m ~270-273).
        const double vtMax = t.transversalSpeedLimit;
        double reduc = 0.0;
        if (vtMax > 0.0 && std::abs(vtTrans) <= vtMax)
        {
            const double ratio = std::abs(vtTrans) / vtMax;
            reduc = (1.0 - ratio) *
                    (1.0 - 2.0 * (1.0 - 2.0 * t.transversalReductionCoeff) * ratio);
        }

        double thrust = (pitch >= 0.0 ? 1.0 : -1.0) * reduc *
                        (cfg_.rho * std::abs(nEff) * nEff * std::pow(D, 4) * Kt);

        // Astern efficiency: applied when exactly one of {rotation, pitch} is
        // negative, i.e. the propeller is operating in reverse (thruster.m ~281-283).
        if ((nEff < 0.0) != (pitch < 0.0))
            thrust *= t.asternEfficiency;

        // Paddle / side-wash force, only while thrust is astern (thruster.m ~284-297).
        double paddleForce = 0.0;
        if (thrust < 0.0)
        {
            const double c0 = t.paddleCoeffs[0];
            double c1 = t.paddleCoeffs[1];
            double c2 = t.paddleCoeffs[2];
            if (c1 == 0.0) c1 = 10.0;
            if (c2 == 0.0) c2 = 1.0;

            const double vMag    = std::sqrt(sw[0] * sw[0] + sw[1] * sw[1]);
            const double nMaxRev = t.rotationSpeedMax / 60.0;
            const double ratio   = (std::abs(nMaxRev) > 1e-12)
                                       ? std::abs(pitch * nEff / nMaxRev)
                                       : 0.0;
            paddleForce = c0 * (1.0 + vMag / (2.0 * c1)) * std::pow(ratio, c2);
        }

        lastThrust_[i] = thrust;
        lastTorque_[i] = torque;
        lastPower_[i]  = power;

        // Resolve into body frame; paddle acts 90° off the thruster axis.
        const double fx = thrust * cosA + paddleForce * std::cos(az_rad + M_PI / 2.0);
        const double fy = thrust * sinA + paddleForce * std::sin(az_rad + M_PI / 2.0);

        ft.f[0] += fx;
        ft.f[1] += fy;
        ft.f[3] += -t.position[2] * fy;
        ft.f[4] +=  t.position[2] * fx;
        ft.f[5] +=  t.position[0] * fy - t.position[1] * fx;
    }

    return ft;
}

} // namespace ymir::naval
