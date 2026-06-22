#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/naval/wave/WaveSpectrum.h>

using Catch::Approx;
using namespace ymir::naval;

TEST_CASE("WaveSpectrum JONSWAP constructs without crash")
{
    WaveSpectrum::Config cfg{};
    cfg.spectrumType = WaveSpectrumType::JONSWAP;
    cfg.significantWaveHeight = 2.0;
    cfg.peakPeriod            = 8.0;
    cfg.gamma                 = 3.3;
    cfg.seed                  = 42;
    cfg.n_frequencies         = 20;
    cfg.n_directions          = 8;
    cfg.spreadingS            = 2.0;
    cfg.mainDirection_deg     = 0.0;

    WaveSpectrum spectrum(cfg);
    REQUIRE(spectrum.numComponents() > 0);
}

TEST_CASE("WaveSpectrum PIERSON constructs")
{
    WaveSpectrum::Config cfg{};
    cfg.spectrumType = WaveSpectrumType::PIERSON;
    cfg.significantWaveHeight = 1.5;
    cfg.peakPeriod            = 6.0;
    cfg.seed                  = 123;
    cfg.n_frequencies         = 10;
    cfg.n_directions          = 4;
    cfg.spreadingS            = 1.0;
    cfg.mainDirection_deg     = 45.0;

    WaveSpectrum spectrum(cfg);
    REQUIRE(spectrum.numComponents() > 0);
}

TEST_CASE("WaveSpectrum REGULAR has single component")
{
    WaveSpectrum::Config cfg{};
    cfg.spectrumType          = WaveSpectrumType::REGULAR;
    cfg.significantWaveHeight = 1.0;
    cfg.peakPeriod            = 8.0;
    cfg.mainDirection_deg     = 0.0;
    cfg.seed                  = 0;
    cfg.n_frequencies         = 1;
    cfg.n_directions          = 1;

    WaveSpectrum spectrum(cfg);
    REQUIRE(spectrum.numComponents() == 1);
}

TEST_CASE("WaveSpectrum total variance matches Hs estimate")
{
    WaveSpectrum::Config cfg{};
    cfg.spectrumType          = WaveSpectrumType::JONSWAP;
    cfg.significantWaveHeight = 3.0;
    cfg.peakPeriod            = 10.0;
    cfg.gamma                 = 3.3;
    cfg.seed                  = 7;
    cfg.n_frequencies         = 64;
    cfg.n_directions          = 18;
    cfg.spreadingS            = 2.0;
    cfg.mainDirection_deg     = 0.0;

    WaveSpectrum spectrum(cfg);
    // Total wave variance = sum of (a_i^2 / 2)
    double m0 = 0.0;
    for (const auto& comp : spectrum.components())
        m0 += comp.amplitude * comp.amplitude / 2.0;

    double Hs_computed = 4.0 * std::sqrt(m0);
    // Should be within 20% of target Hs
    REQUIRE(Hs_computed == Approx(cfg.significantWaveHeight).epsilon(0.20));
}
