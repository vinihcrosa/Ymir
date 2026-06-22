#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/naval/wave/WaveForces.h>
#include <ymir/naval/wave/WaveSpectrum.h>
#include <ymir/naval/NavalContext.h>
#include <ymir/core/BodyState.h>

using Catch::Approx;
using namespace ymir::naval;

static NavalContext makeCtx()
{
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state = bs;
    return ctx;
}

static WaveSpectrum makeSpectrum()
{
    WaveSpectrum::Config cfg{};
    cfg.spectrumType          = WaveSpectrumType::REGULAR;
    cfg.significantWaveHeight = 1.0;
    cfg.peakPeriod            = 8.0;
    cfg.mainDirection_deg     = 0.0;
    cfg.seed                  = 0;
    cfg.n_frequencies         = 1;
    cfg.n_directions          = 1;
    return WaveSpectrum(cfg);
}

TEST_CASE("WaveForces constructs with WAMIT tables")
{
    WaveForces::Config cfg{};
    cfg.wavesOriginPosition = {0.0, 0.0, 0.0};
    // Minimal 1x1 RAO table (1 frequency, 1 direction)
    cfg.wamitFrequencies = {0.785};  // rad/s (8s period)
    cfg.wamitDirections  = {0.0};
    // RAO magnitude and phase for 6 DOF x 1 freq x 1 dir
    cfg.raoMagnitude.assign(6, std::vector<std::vector<double>>(1, std::vector<double>(1, 0.0)));
    cfg.raoPhase.assign(6,    std::vector<std::vector<double>>(1, std::vector<double>(1, 0.0)));
    // Set heave RAO = 1.0 (unit response)
    cfg.raoMagnitude[2][0][0] = 1.0;

    WaveForces model(cfg, makeSpectrum());
    REQUIRE(true);  // construction succeeds
}

TEST_CASE("WaveForces compute does not throw")
{
    WaveForces::Config cfg{};
    cfg.wavesOriginPosition = {0.0, 0.0, 0.0};
    cfg.wamitFrequencies = {0.785};
    cfg.wamitDirections  = {0.0};
    cfg.raoMagnitude.assign(6, std::vector<std::vector<double>>(1, std::vector<double>(1, 0.0)));
    cfg.raoPhase.assign(6,    std::vector<std::vector<double>>(1, std::vector<double>(1, 0.0)));

    WaveForces model(cfg, makeSpectrum());
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    REQUIRE_NOTHROW(model.compute(ctx.state));
}
