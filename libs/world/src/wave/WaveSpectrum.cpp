#include <ymir/world/wave/WaveSpectrum.h>

#include <ymir/common/PhysicalConstants.h>

#include <algorithm>
#include <cmath>
#include <random>

namespace ymir::naval
{

WaveSpectrum::WaveSpectrum(const Config& cfg)
{
    switch (cfg.spectrumType)
    {
    case WaveSpectrumType::JONSWAP:  buildJonswap(cfg); break;
    case WaveSpectrumType::PIERSON:  buildPierson(cfg); break;
    case WaveSpectrumType::REGULAR:  buildRegular(cfg); break;
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static double jonswapSpectral(double omega, double omega_p, double gamma)
{
    if (omega < 1e-8) return 0.0;
    double sigma = (omega <= omega_p) ? 0.07 : 0.09;
    double r     = std::exp(-0.5 * std::pow((omega - omega_p) / (sigma * omega_p), 2.0));
    double S_pm  = std::exp(-5.0 / 4.0 * std::pow(omega_p / omega, 4.0)) / std::pow(omega, 5.0);
    return S_pm * std::pow(gamma, r);
}

static std::vector<double> buildFrequencies(const WaveSpectrum::Config& cfg)
{
    double omega_p = 2.0 * M_PI / cfg.peakPeriod;
    double f_min   = (cfg.f_min > 0.0) ? cfg.f_min : omega_p * 0.5;
    double f_max   = (cfg.f_max > 0.0) ? cfg.f_max : omega_p * 4.0;

    int n = std::max(cfg.n_frequencies, 1);
    std::vector<double> freqs(n);
    double dw = (f_max - f_min) / (n - 1 > 0 ? n - 1 : 1);
    for (int i = 0; i < n; ++i)
        freqs[i] = f_min + i * dw;
    return freqs;
}

static std::vector<double> buildDirections(const WaveSpectrum::Config& cfg)
{
    int n = std::max(cfg.n_directions, 1);
    std::vector<double> dirs(n);
    double theta0 = cfg.mainDirection_deg * M_PI / 180.0;
    double d_theta = (n > 1) ? (2.0 * M_PI / n) : 0.0;
    for (int j = 0; j < n; ++j)
        dirs[j] = theta0 + (j - n / 2) * d_theta;
    return dirs;
}

static std::vector<double> buildSpreadingWeights(const std::vector<double>& dirs,
                                                  double                     mainDir,
                                                  double                     s)
{
    int n = static_cast<int>(dirs.size());
    std::vector<double> w(n);
    double dtheta = (n > 1) ? (2.0 * M_PI / n) : 1.0;

    // cos^(2s)((theta - theta0)/2) spreading
    for (int j = 0; j < n; ++j)
    {
        double diff = dirs[j] - mainDir;
        double c    = std::cos(diff / 2.0);
        w[j] = std::max(std::pow(c * c, s), 0.0) * dtheta;
    }

    // Normalize so sum = 1
    double total = 0.0;
    for (double v : w) total += v;
    if (total > 1e-12)
        for (double& v : w) v /= total;
    else
        for (double& v : w) v = 1.0 / n;

    return w;
}

// ---------------------------------------------------------------------------
// JONSWAP
// ---------------------------------------------------------------------------

void WaveSpectrum::buildJonswap(const Config& cfg)
{
    double omega_p = 2.0 * M_PI / cfg.peakPeriod;
    auto   freqs   = buildFrequencies(cfg);
    auto   dirs    = buildDirections(cfg);

    double mainDir = cfg.mainDirection_deg * M_PI / 180.0;
    auto   spread  = buildSpreadingWeights(dirs, mainDir, cfg.spreadingS);

    int Nf = static_cast<int>(freqs.size());
    int Nd = static_cast<int>(dirs.size());

    // Unscaled spectrum
    std::vector<double> S(Nf);
    for (int i = 0; i < Nf; ++i)
        S[i] = jonswapSpectral(freqs[i], omega_p, cfg.gamma);

    // Integrate for normalization: m0_unscaled = Σ S[i] * dω
    double m0_unscaled = 0.0;
    for (int i = 0; i < Nf; ++i)
    {
        double dw = (i + 1 < Nf) ? (freqs[i + 1] - freqs[i]) : (freqs[i] - freqs[i - 1]);
        m0_unscaled += S[i] * dw;
    }

    // Scale so m0 = (Hs/4)²
    double target_m0 = (cfg.significantWaveHeight / 4.0) * (cfg.significantWaveHeight / 4.0);
    double scale     = (m0_unscaled > 1e-20) ? (target_m0 / m0_unscaled) : 1.0;

    std::mt19937                          rng(cfg.seed);
    std::uniform_real_distribution<double> uniform(0.0, 2.0 * M_PI);

    components_.reserve(Nf * Nd);
    for (int i = 0; i < Nf; ++i)
    {
        double dw = (i + 1 < Nf) ? (freqs[i + 1] - freqs[i]) : (freqs[i] - freqs[i - 1]);
        double Si = S[i] * scale;
        double k  = freqs[i] * freqs[i] / g;  // deep water dispersion

        for (int j = 0; j < Nd; ++j)
        {
            double amp = std::sqrt(2.0 * Si * dw * spread[j]);
            if (amp < 1e-12) continue;
            WaveComponent comp;
            comp.amplitude  = amp;
            comp.frequency  = freqs[i];
            comp.direction  = dirs[j];
            comp.wavenumber = k;
            comp.phase      = uniform(rng);
            components_.push_back(comp);
        }
    }
}

// ---------------------------------------------------------------------------
// PIERSON-MOSKOWITZ (γ = 1)
// ---------------------------------------------------------------------------

void WaveSpectrum::buildPierson(const Config& cfg)
{
    Config pm     = cfg;
    pm.gamma      = 1.0;
    pm.spectrumType = WaveSpectrumType::JONSWAP;
    buildJonswap(pm);
}

// ---------------------------------------------------------------------------
// REGULAR
// ---------------------------------------------------------------------------

void WaveSpectrum::buildRegular(const Config& cfg)
{
    WaveComponent comp;
    comp.amplitude  = cfg.significantWaveHeight / 2.0;
    comp.frequency  = 2.0 * M_PI / cfg.peakPeriod;
    comp.direction  = cfg.mainDirection_deg * M_PI / 180.0;
    comp.wavenumber = comp.frequency * comp.frequency / g;
    comp.phase      = 0.0;
    components_.push_back(comp);
}

} // namespace ymir::naval
