#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/physics/forces/TugForces.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>

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

TEST_CASE("TugForces PUSH zero bollard produces no force")
{
    TugForces::Config cfg{};
    TugForces::TugConfig t{};
    t.mode         = TugMode::PUSH;
    t.position     = {50.0, 0.0, 0.0};
    t.angle_deg    = 0.0;
    t.bollardPull  = 0.0;
    cfg.tugs.push_back(t);

    TugForces model(cfg);
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    for (int i = 0; i < 6; ++i)
        REQUIRE(forces.f[i] == Approx(0.0).margin(1e-10));
}

TEST_CASE("TugForces PUSH positive bollard produces force along heading")
{
    TugForces::Config cfg{};
    TugForces::TugConfig t{};
    t.mode        = TugMode::PUSH;
    t.position    = {50.0, 0.0, 0.0};
    t.angle_deg   = 0.0;     // pushing along +x (aft → fwd)
    t.bollardPull = 100000.0; // 100 kN
    cfg.tugs.push_back(t);

    TugForces model(cfg);
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    REQUIRE(forces.f[0] != Approx(0.0).margin(1.0));
}
