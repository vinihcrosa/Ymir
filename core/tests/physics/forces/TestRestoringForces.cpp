#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/physics/forces/RestoringForces.h>
#include <ymir/vessel/NavalContext.h>
#include <ymir/physics/BodyState.h>
#include <ymir/common/Types.h>
#include <ymir/common/PhysicalConstants.h>

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

TEST_CASE("RestoringForces zero displacement at neutral")
{
    RestoringConfig cfg{};
    cfg.volumetricWeight   = 1000.0 * g;  // 1 tonne vessel
    cfg.mass               = 1000.0;
    cfg.hydro_rest[2][2]   = 1000.0;
    cfg.hydro_rest[3][3]   = 5000.0;
    cfg.hydro_rest[4][4]   = 5000.0;

    RestoringForces model(cfg);
    NavalContext ctx = makeCtx();
    model.bindContext(&ctx);

    auto forces = model.compute(ctx.state);
    // net buoyancy = mass*g - volumetricWeight = 0 → heave = 0
    REQUIRE(forces.f[2] == Approx(0.0).margin(1e-3));
    REQUIRE(forces.f[3] == Approx(0.0).margin(1e-3));
    REQUIRE(forces.f[4] == Approx(0.0).margin(1e-3));
}

TEST_CASE("RestoringForces roll spring opposes positive roll")
{
    RestoringConfig cfg{};
    cfg.volumetricWeight = 0.0;
    cfg.mass             = 0.0;
    cfg.hydro_rest[3][3] = 10000.0;  // K_roll = 10 kN/rad

    RestoringForces model(cfg);

    ymir::Vector6 q{};
    q[3] = 0.1;  // roll = 0.1 rad
    ymir::Vector6 qdot{};
    ymir::BodyState bs(q, qdot, 0.0, 0.1);
    NavalContext ctx{};
    ctx.state = bs;
    model.bindContext(&ctx);

    auto forces = model.compute(bs);
    REQUIRE(forces.f[3] < 0.0);  // restoring moment opposes roll
}
