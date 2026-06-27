#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/simulation/CouplingForceModel.h>

using Catch::Approx;

static ymir::naval::NavalContext makeDummyContext()
{
    return ymir::naval::NavalContext{};
}

static ymir::BodyState makeDummyState()
{
    return ymir::BodyState{};
}

static ymir::Forces makeForces(double surge, double sway = 0.0,
                                double heave = 0.0, double roll = 0.0,
                                double pitch = 0.0, double yaw = 0.0)
{
    ymir::Forces f{};
    f.f[0] = surge; f.f[1] = sway;  f.f[2] = heave;
    f.f[3] = roll;  f.f[4] = pitch; f.f[5] = yaw;
    return f;
}

static void requireForcesApproxEqual(const ymir::Forces& a, const ymir::Forces& b)
{
    for (int i = 0; i < 6; ++i)
        REQUIRE(a.f[i] == Approx(b.f[i]));
}

static void requireForcesApproxZero(const ymir::Forces& f)
{
    requireForcesApproxEqual(f, ymir::Forces::zero());
}

TEST_CASE("CouplingForceModel: compute returns Forces::zero when no force written to registry")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);

    ymir::CouplingForceModel model(0, reg);
    auto ctx = makeDummyContext();
    model.bindContext(&ctx);

    requireForcesApproxZero(model.compute(makeDummyState()));
}

TEST_CASE("CouplingForceModel: compute returns resolved force after writeForce + resolve")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);

    const ymir::Forces F = makeForces(100.0, 50.0, -10.0);
    reg.writeForce(1, F);
    reg.resolve();

    ymir::CouplingForceModel model(0, reg);
    auto ctx = makeDummyContext();
    model.bindContext(&ctx);

    requireForcesApproxEqual(model.compute(makeDummyState()), F);
}

TEST_CASE("CouplingForceModel: Jacobi invariant — reset without new writeForce retains previous force")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);

    const ymir::Forces F = makeForces(200.0);
    reg.writeForce(1, F);
    reg.resolve();
    reg.reset();
    reg.resolve();  // no writeForce this tick

    ymir::CouplingForceModel model(0, reg);
    auto ctx = makeDummyContext();
    model.bindContext(&ctx);

    requireForcesApproxEqual(model.compute(makeDummyState()), F);
}

TEST_CASE("CouplingForceModel: name() returns CouplingForceModel")
{
    ymir::CouplingRegistry reg;
    ymir::CouplingForceModel model(0, reg);

    REQUIRE(model.name() == "CouplingForceModel");
}

TEST_CASE("CouplingForceModel: multiple instances on same registry return independent forces")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);
    reg.addLink(2, 3);

    const ymir::Forces FA = makeForces(100.0);
    const ymir::Forces FB = makeForces(0.0, 200.0);
    reg.writeForce(1, FA);
    reg.writeForce(2, FB);
    reg.resolve();

    ymir::CouplingForceModel model0(0, reg);
    ymir::CouplingForceModel model3(3, reg);
    auto ctx = makeDummyContext();
    model0.bindContext(&ctx);
    model3.bindContext(&ctx);

    requireForcesApproxEqual(model0.compute(makeDummyState()), FA);
    requireForcesApproxEqual(model3.compute(makeDummyState()), FB);
}
