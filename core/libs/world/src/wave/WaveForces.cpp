#include <ymir/world/wave/WaveForces.h>

#include <ymir/common/math/Interpolation.h>
#include <ymir/common/math/LinearAlgebra.h>
#include <ymir/common/PhysicalConstants.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace ymir::naval
{

namespace
{
// Excitation DOF mask, mirroring VesselFastTime.m:531 ([0 0 1 1 1 0]'):
// surge/sway/yaw 1st-order excitation is dropped, heave/roll/pitch is kept.
// (The dropped DOF are driven instead by the 2nd-order drift terms below.)
constexpr std::array<double, 6> kExcitationMask{0.0, 0.0, 1.0, 1.0, 1.0, 0.0};

// DOF that carry 2nd-order drift loads (surge, sway, yaw — MATLAB [1 2 6]).
constexpr std::array<int, 3> kDriftDof{0, 1, 5};

// Look up a [dof][freq][dir] transfer-function table at (omega, dir).
// Returns 0 when the table is absent/empty for that DOF.
double tableLookup(const std::vector<std::vector<std::vector<double>>>& table,
                   const std::vector<double>&                           freqs,
                   const std::vector<double>&                           dirs,
                   int                                                  dof,
                   double                                               omega,
                   double                                               dir)
{
    if (dof < 0 || dof >= static_cast<int>(table.size())) return 0.0;
    const auto& z = table[dof];
    if (z.empty() || z[0].empty()) return 0.0;
    return ymir::math::bilinear(freqs, dirs, z, omega, dir);
}
} // namespace

WaveForces::WaveForces(const Config& cfg, const WaveSpectrum& spectrum)
    : cfg_(cfg)
    , components_(spectrum.components())
{
}

void WaveForces::setSpectrum(const WaveSpectrum& spectrum)
{
    components_ = spectrum.components();
}

Forces WaveForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    if (components_.empty()) return Forces::zero();

    const double t   = state.time();
    const double yaw = state.yaw();
    const double xo  = cfg_.wavesOriginPosition[0];
    const double yo  = cfg_.wavesOriginPosition[1];

    const bool hasRao   = !cfg_.wamitFrequencies.empty() && !cfg_.raoMagnitude.empty();
    const bool hasDrift = !cfg_.wamitFrequencies.empty() && !cfg_.meanDriftCoeff.empty();

    Forces fw;

    // -----------------------------------------------------------------------
    // 1st-order excitation + 2nd-order mean drift (per wave component)
    //   excitation : VesselFastTime.m ~792 — sum_i a_i · Fex(ω,β) · cos(z+φ)
    //   mean drift : VesselFastTime.m ~778 — sum_i a_i² · D(ω,β)
    // -----------------------------------------------------------------------
    for (const auto& comp : components_)
    {
        // Wave elevation phase at the wave origin (z = -ω·t + φ at origin).
        const double phase = comp.frequency * t
                           - comp.wavenumber * (xo * std::cos(comp.direction)
                                              + yo * std::sin(comp.direction))
                           + comp.phase;

        const double amp2 = comp.amplitude * comp.amplitude;

        for (int dof = 0; dof < 6; ++dof)
        {
            if (hasRao && kExcitationMask[dof] != 0.0)
            {
                const double mag = tableLookup(cfg_.raoMagnitude, cfg_.wamitFrequencies,
                                               cfg_.wamitDirections, dof,
                                               comp.frequency, comp.direction);
                const double ph  = tableLookup(cfg_.raoPhase, cfg_.wamitFrequencies,
                                               cfg_.wamitDirections, dof,
                                               comp.frequency, comp.direction);
                fw.f[dof] += kExcitationMask[dof] * comp.amplitude * mag
                           * std::cos(phase + ph);
            }
        }

        if (hasDrift)
        {
            for (int dof : kDriftDof)
            {
                const double d = tableLookup(cfg_.meanDriftCoeff, cfg_.wamitFrequencies,
                                             cfg_.wamitDirections, dof,
                                             comp.frequency, comp.direction);
                fw.f[dof] += amp2 * d;
            }
        }
    }

    // -----------------------------------------------------------------------
    // 2nd-order slow drift (difference-frequency, Newman approximation)
    //   VesselFastTime.m ~782 — vanishes for a single regular component.
    // For each pair of components sharing a direction, the bound long wave at
    // the difference frequency drives a slowly-varying surge/sway/yaw force
    //   F_slow(dof) = sum_{i<j} 2·a_i·a_j·D̄(dof)·cos(Δk·r − Δω·t + Δφ)
    // with D̄ the mean of the two component mean-drift coefficients. The slow
    // spatial phase uses the EMA-smoothed position (ctx.q_avg), matching the
    // MATLAB auxPosition used for the slow-drift term.
    // -----------------------------------------------------------------------
    if (hasDrift && components_.size() > 1)
    {
        const double xs = ctx.q_avg[0];
        const double ys = ctx.q_avg[1];

        for (std::size_t i = 0; i + 1 < components_.size(); ++i)
        {
            for (std::size_t j = i + 1; j < components_.size(); ++j)
            {
                const auto& a = components_[i];
                const auto& b = components_[j];

                // Only components travelling in the same direction form a
                // long-crested difference wave.
                if (std::abs(a.direction - b.direction) > 1e-9) continue;

                const double dOmega = b.frequency - a.frequency;
                if (std::abs(dOmega) < 1e-9) continue;

                const double dk    = b.wavenumber - a.wavenumber;
                const double dPhase = b.phase - a.phase;
                const double slowPh = dk * (xs * std::cos(a.direction)
                                          + ys * std::sin(a.direction))
                                    - dOmega * t + dPhase;
                const double c       = std::cos(slowPh);
                const double weight  = 2.0 * a.amplitude * b.amplitude * c;

                for (int dof : kDriftDof)
                {
                    const double di = tableLookup(cfg_.meanDriftCoeff, cfg_.wamitFrequencies,
                                                  cfg_.wamitDirections, dof,
                                                  a.frequency, a.direction);
                    const double dj = tableLookup(cfg_.meanDriftCoeff, cfg_.wamitFrequencies,
                                                  cfg_.wamitDirections, dof,
                                                  b.frequency, b.direction);
                    fw.f[dof] += weight * 0.5 * (di + dj);
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // Wave-drift damping (VesselFastTime.m ~817) — applied once for the sea
    // state at its amplitude²-weighted mean incidence angle, using the body's
    // velocity relative to the current. bw is the in-line term, br the cross.
    //   F(dof) = u·(c·Bw + s·Br) + v·(s·Bw − c·Br)
    // with c=cos(incidence), s=sin(incidence), u/v surge/sway speed-to-water.
    // -----------------------------------------------------------------------
    {
        const bool hasDamping = std::any_of(
            kDriftDof.begin(), kDriftDof.end(),
            [&](int dof) { return !cfg_.driftDampingBw[dof].empty()
                               || !cfg_.driftDampingBr[dof].empty(); });

        if (hasDamping)
        {
            // amplitude²-weighted mean wave direction.
            double sumW = 0.0, sumWdir = 0.0;
            for (const auto& comp : components_)
            {
                const double w = comp.amplitude * comp.amplitude;
                sumW    += w;
                sumWdir += w * comp.direction;
            }
            const double meanDir   = (sumW > 0.0) ? (sumWdir / sumW) : components_[0].direction;
            const double incidence = meanDir - yaw;  // wave heading relative to ship
            const double c         = std::cos(incidence);
            const double s         = std::sin(incidence);

            const double u = ctx.speedToWater[0];
            const double v = ctx.speedToWater[1];

            for (int dof : kDriftDof)
            {
                const auto& bwTab = cfg_.driftDampingBw[dof];
                const auto& brTab = cfg_.driftDampingBr[dof];
                if (bwTab.empty() && brTab.empty()) continue;

                const double bw = bwTab.empty() ? 0.0
                                : ymir::math::linear(cfg_.wamitDirections, bwTab, incidence);
                const double br = brTab.empty() ? 0.0
                                : ymir::math::linear(cfg_.wamitDirections, brTab, incidence);

                fw.f[dof] += u * (c * bw + s * br) + v * (s * bw - c * br);
            }
        }
    }

    return fw;
}

} // namespace ymir::naval
