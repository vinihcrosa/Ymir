#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/world/wave/WaveForces.h>
#include <ymir/world/wave/WaveSpectrum.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>

#include <cmath>

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

// Regular-wave frequency for Tp = 8 s (matches makeSpectrum()).
static constexpr double kRegularOmega = 2.0 * M_PI / 8.0;

TEST_CASE("WaveForces excitation mask zeroes surge/sway/yaw")
{
    WaveForces::Config cfg{};
    cfg.wavesOriginPosition = {0.0, 0.0, 0.0};
    cfg.wamitFrequencies = {kRegularOmega};
    cfg.wamitDirections  = {0.0};

    // Non-zero unit excitation magnitude on ALL six DOF, zero phase.
    cfg.raoMagnitude.assign(6, std::vector<std::vector<double>>(1, std::vector<double>(1, 1.0)));
    cfg.raoPhase.assign(6,    std::vector<std::vector<double>>(1, std::vector<double>(1, 0.0)));
    // No drift configured → surge/sway/yaw must come out exactly zero.

    WaveForces model(cfg, makeSpectrum());
    NavalContext ctx = makeCtx();          // t = 0, phase = 0 → cos = 1
    model.bindContext(&ctx);

    ymir::Forces fw = model.compute(ctx.state);

    // Masked DOF: surge(0), sway(1), yaw(5) zeroed.
    REQUIRE(fw.f[0] == Approx(0.0));
    REQUIRE(fw.f[1] == Approx(0.0));
    REQUIRE(fw.f[5] == Approx(0.0));

    // Kept DOF: heave(2), roll(3), pitch(4) carry the excitation (a · mag).
    const double amp = 0.5;  // Hs/2 with Hs = 1.0
    REQUIRE(fw.f[2] == Approx(amp));
    REQUIRE(fw.f[3] == Approx(amp));
    REQUIRE(fw.f[4] == Approx(amp));
}

TEST_CASE("WaveForces mean drift is a non-zero steady force in a regular wave")
{
    WaveForces::Config cfg{};
    cfg.wavesOriginPosition = {0.0, 0.0, 0.0};
    cfg.wamitFrequencies = {kRegularOmega};
    cfg.wamitDirections  = {0.0};
    // No excitation — isolate the mean-drift term.
    cfg.raoMagnitude.assign(6, std::vector<std::vector<double>>(1, std::vector<double>(1, 0.0)));
    cfg.raoPhase.assign(6,    std::vector<std::vector<double>>(1, std::vector<double>(1, 0.0)));

    // Surge mean-drift coefficient (force per amplitude²).
    cfg.meanDriftCoeff.assign(6, std::vector<std::vector<double>>(1, std::vector<double>(1, 0.0)));
    const double driftCoeff = 1000.0;
    cfg.meanDriftCoeff[0][0][0] = driftCoeff;

    WaveForces model(cfg, makeSpectrum());

    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    const double amp      = 0.5;                 // Hs/2
    const double expected = amp * amp * driftCoeff;  // a² · D

    // Steady: same value regardless of time (no phase dependence).
    ymir::BodyState s0(ymir::Vector6{}, ymir::Vector6{}, 0.0, 0.1);
    ymir::BodyState s1(ymir::Vector6{}, ymir::Vector6{}, 5.0, 0.1);

    ymir::Forces f0 = model.compute(s0);
    ymir::Forces f1 = model.compute(s1);

    REQUIRE(expected > 0.0);
    REQUIRE(f0.f[0] == Approx(expected));
    REQUIRE(f1.f[0] == Approx(expected));   // time-invariant ⇒ steady drift
    // Single regular component ⇒ no slow-drift contamination on sway/yaw.
    REQUIRE(f0.f[1] == Approx(0.0));
    REQUIRE(f0.f[5] == Approx(0.0));
}
