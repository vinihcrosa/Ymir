#include <ymir/naval/wave/WaveForces.h>

#include <ymir/core/math/Interpolation.h>
#include <ymir/core/math/LinearAlgebra.h>
#include <ymir/naval/PhysicalConstants.h>

#include <algorithm>
#include <cmath>

namespace ymir::naval
{

WaveForces::WaveForces(const Config& cfg, const WaveSpectrum& spectrum)
    : cfg_(cfg)
    , components_(spectrum.components())
{
    q_rao_prev_.fill(0.0);
}

void WaveForces::resetState() noexcept
{
    q_rao_prev_.fill(0.0);
}

Forces WaveForces::computeNaval(const BodyState& state, const NavalContext& ctx)
{
    if (components_.empty()) return Forces::zero();

    const double t  = state.time();
    const double xo = cfg_.wavesOriginPosition[0];
    const double yo = cfg_.wavesOriginPosition[1];

    Forces fw;

    bool hasRao = !cfg_.wamitFrequencies.empty() && !cfg_.raoMagnitude.empty();

    for (const auto& comp : components_)
    {
        // Wave elevation phase at origin
        double phase = comp.frequency * t
                     - comp.wavenumber * (xo * std::cos(comp.direction)
                                        + yo * std::sin(comp.direction))
                     + comp.phase;

        // DOF 2,3,4: simple heave/roll/pitch — use cosine excitation if no RAO
        // DOF 0,1,5: surge/sway/yaw — use RAO if available

        for (int dof = 0; dof < 6; ++dof)
        {
            double mag   = 0.0;
            double ph    = 0.0;

            if (hasRao && dof < static_cast<int>(cfg_.raoMagnitude.size()))
            {
                const auto& raoM = cfg_.raoMagnitude[dof];
                const auto& raoP = cfg_.raoPhase[dof];

                if (!raoM.empty() && !raoM[0].empty())
                {
                    mag = ymir::math::bilinear(cfg_.wamitFrequencies,
                                               cfg_.wamitDirections,
                                               raoM,
                                               comp.frequency,
                                               comp.direction);
                    ph  = ymir::math::bilinear(cfg_.wamitFrequencies,
                                               cfg_.wamitDirections,
                                               raoP,
                                               comp.frequency,
                                               comp.direction);
                }
            }

            fw.f[dof] += comp.amplitude * mag * std::cos(phase + ph);
        }
    }

    return fw;
}

} // namespace ymir::naval
