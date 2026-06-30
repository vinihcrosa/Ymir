#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/physics/forces/CurrentForces.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>

using Catch::Approx;
using namespace ymir::naval;

static CurrentForces::Config makeObokataCfg()
{
    CurrentForces::Config cfg{};
    cfg.model        = CurrentModel::OBOKATA;
    cfg.length_BP    = 100.0;
    cfg.beam         = 20.0;
    cfg.frontalHeight = 5.0;
    cfg.lateralHeight = 5.0;
    cfg.n_sections   = 10;
    cfg.angles = {0.0, 90.0, 180.0, 270.0, 360.0};
    cfg.cdx    = {1.0, 0.0, -1.0, 0.0, 1.0};
    cfg.cdy    = {0.0, 1.0,  0.0, -1.0, 0.0};
    cfg.cdz    = {0.0, 0.0,  0.0, 0.0, 0.0};
    return cfg;
}

static CurrentForces::Config makeRegularCfg()
{
    CurrentForces::Config cfg{};
    cfg.model       = CurrentModel::REGULAR;
    cfg.length_BP   = 100.0;
    cfg.frontalArea = 200.0;
    cfg.lateralArea = 1000.0;
    cfg.st          = 0.2;
    cfg.ez          = 0.3;
    cfg.az          = 0.1;
    cfg.clO         = 1.0;
    cfg.angles = {0.0, 90.0, 180.0, 270.0, 360.0};
    cfg.cdx    = {1.0, 0.0, -1.0, 0.0, 1.0};
    cfg.cdy    = {0.0, 1.0,  0.0, -1.0, 0.0};
    cfg.cdz    = {0.0, 0.0,  0.0, 0.0, 0.0};
    return cfg;
}

static NavalContext makeCtx(double vc_u, double vc_v)
{
    ymir::Vector6 q{};
    ymir::Vector6 qdot{};
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state = bs;
    ctx.speedToWater[0] = vc_u;
    ctx.speedToWater[1] = vc_v;
    return ctx;
}

TEST_CASE("CurrentForces OBOKATA zero at rest")
{
    CurrentForces model(makeObokataCfg());
    NavalContext ctx = makeCtx(0.0, 0.0);
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    for (int i = 0; i < 6; ++i)
        REQUIRE(forces.f[i] == Approx(0.0).margin(1e-10));
}

TEST_CASE("CurrentForces OBOKATA current from ahead produces surge force")
{
    CurrentForces model(makeObokataCfg());
    NavalContext ctx = makeCtx(2.0, 0.0);
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[0] != Approx(0.0).margin(1e-4));
}

// Roll/pitch moments from the current force acting at a vertical arm
// (MATLAB currentObokata 652-658). With originZ=0, arm = frontal/lateralHeight.
TEST_CASE("CurrentForces OBOKATA current produces roll/pitch arm moments")
{
    auto cfg = makeObokataCfg();
    cfg.frontalHeight = 5.0;
    cfg.lateralHeight = 7.0;

    // Current from ahead → surge force Fx → pitch moment f[4] = Fx * lateralHeight.
    {
        CurrentForces model(cfg);
        NavalContext ctx = makeCtx(2.0, 0.0);
        model.bindContext(&ctx);
        auto f = model.compute(ctx.state);
        REQUIRE(f.f[4] == Approx(f.f[0] * cfg.lateralHeight).epsilon(1e-9));
        REQUIRE(f.f[3] == Approx(-f.f[1] * cfg.frontalHeight).epsilon(1e-9));
    }
    // Current from the side → sway force Fy → roll moment f[3] = -Fy * frontalHeight.
    {
        CurrentForces model(cfg);
        NavalContext ctx = makeCtx(0.0, 2.0);
        model.bindContext(&ctx);
        auto f = model.compute(ctx.state);
        REQUIRE(f.f[1] != Approx(0.0).margin(1e-4));
        REQUIRE(f.f[3] == Approx(-f.f[1] * cfg.frontalHeight).epsilon(1e-9));
    }
}

TEST_CASE("CurrentForces REGULAR zero at rest")
{
    CurrentForces model(makeRegularCfg());
    NavalContext ctx = makeCtx(0.0, 0.0);
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    for (int i = 0; i < 6; ++i)
        REQUIRE(forces.f[i] == Approx(0.0).margin(1e-10));
}

TEST_CASE("CurrentForces VDP resetState zeros oscillator")
{
    CurrentForces model(makeRegularCfg());
    NavalContext ctx = makeCtx(2.0, 0.0);
    model.bindContext(&ctx);

    // Run a few steps to excite VDP
    for (int i = 0; i < 10; ++i)
        model.compute(ctx.state);

    // Reset should zero the VDP state
    model.resetState();

    // After reset, VDP output starts from zero again
    NavalContext ctx2 = makeCtx(0.0, 0.0);
    model.bindContext(&ctx2);
    auto forces = model.compute(ctx2.state);
    for (int i = 0; i < 6; ++i)
        REQUIRE(forces.f[i] == Approx(0.0).margin(1e-10));
}
