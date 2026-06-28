#pragma once

#include <ymir/world/wave/WaveComponent.h>

#include <cstdint>
#include <vector>

namespace ymir::naval
{

enum class WaveSpectrumType
{
    JONSWAP,
    PIERSON,
    REGULAR
};

class WaveSpectrum
{
public:
    struct Config
    {
        WaveSpectrumType spectrumType          = WaveSpectrumType::JONSWAP;
        double           significantWaveHeight = 2.0;   // Hs, m
        double           peakPeriod            = 8.0;   // Tp, s
        double           gamma                 = 3.3;   // JONSWAP peak enhancement
        double           spreadingS            = 2.0;   // Mitsuyasu s exponent
        double           mainDirection_deg     = 0.0;   // principal direction (math, deg)
        int              n_frequencies         = 32;
        int              n_directions          = 18;
        uint32_t         seed                  = 42;

        // Frequency range (0 = auto)
        double f_min = 0.0;
        double f_max = 0.0;
    };

    explicit WaveSpectrum(const Config& cfg);

    std::size_t                       numComponents() const noexcept { return components_.size(); }
    const std::vector<WaveComponent>& components()    const noexcept { return components_; }

private:
    void buildJonswap(const Config& cfg);
    void buildPierson(const Config& cfg);
    void buildRegular(const Config& cfg);

    std::vector<WaveComponent> components_;
};

} // namespace ymir::naval
