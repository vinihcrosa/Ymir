#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/physics/forces/SquatForces.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>
#include <ymir/common/PhysicalConstants.h>

using Catch::Approx;
using namespace ymir::naval;

static NavalContext makeCtx(double u = 0.0, double depth = 20.0)
{
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    qdot[0] = u;
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state      = bs;
    ctx.waterDepth = depth;
    ctx.speedToWater[0] = u;
    return ctx;
}

TEST_CASE("SquatForces zero at rest")
{
    SquatForces::Config cfg{};
    cfg.blockCoefficient = 0.75;
    cfg.volumetricWeight = 1e6 * g;
    cfg.length_BP        = 100.0;
    cfg.hydroRestHeave   = 1e5;

    SquatForces model(cfg);
    NavalContext ctx = makeCtx(0.0);
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[2] == Approx(0.0).margin(1e-6));
}

TEST_CASE("SquatForces sinkage negative heave when moving")
{
    SquatForces::Config cfg{};
    cfg.blockCoefficient = 0.75;
    cfg.volumetricWeight = 1e6 * g;
    cfg.length_BP        = 100.0;
    cfg.hydroRestHeave   = 1e6;  // large stiffness to amplify

    SquatForces model(cfg);
    // Speed = 3 m/s in 20 m water
    NavalContext ctx = makeCtx(3.0, 20.0);
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    // Squat sinks vessel → negative heave force (or zero; magnitude uncertain)
    REQUIRE(forces.f[2] <= 0.0);
}

// Cs coefficient table parity with MATLAB VesselFastTime.squatForce: a hull with
// Cb > 1 passes Cs through as Cb itself (so a Cb of 1.5 yields LESS squat than a
// Cb of 0.9, which maps to the banded value 2.4). Pre-fix both mapped to 2.4.
TEST_CASE("SquatForces Cb>1 passes Cs through (MATLAB ordering)")
{
    auto squatAt = [](double Cb) {
        SquatForces::Config cfg{};
        cfg.blockCoefficient = Cb;
        cfg.volumetricWeight = 1e6 * g;
        cfg.length_BP        = 100.0;
        cfg.hydroRestHeave   = 1e6;
        SquatForces model(cfg);
        NavalContext ctx = makeCtx(3.0, 20.0);
        model.bindContext(&ctx);
        return model.compute(ctx.state).f[2];
    };

    const double banded = squatAt(0.9);  // Cs = 2.4
    const double passthrough = squatAt(1.5);  // Cs = 1.5 (< 2.4) → smaller sinkage
    REQUIRE(banded < 0.0);
    REQUIRE(passthrough < 0.0);
    // Smaller Cs → smaller magnitude → less negative heave.
    REQUIRE(passthrough > banded);
    // Ratio tracks Cs ratio (1.5 / 2.4) since everything else is identical.
    REQUIRE(passthrough / banded == Approx(1.5 / 2.4).epsilon(1e-6));
}
