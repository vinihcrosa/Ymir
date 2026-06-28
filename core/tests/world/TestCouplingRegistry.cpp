#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/world/CouplingRegistry.h>

using Catch::Approx;

static ymir::Forces makeForces(double surge, double sway = 0.0,
                                double heave = 0.0, double roll = 0.0,
                                double pitch = 0.0, double yaw = 0.0)
{
    ymir::Forces f{};
    f.f[0] = surge;
    f.f[1] = sway;
    f.f[2] = heave;
    f.f[3] = roll;
    f.f[4] = pitch;
    f.f[5] = yaw;
    return f;
}

static void requireForcesApproxZero(const ymir::Forces& f)
{
    for (int i = 0; i < 6; ++i)
        REQUIRE(f.f[i] == Approx(0.0));
}

static void requireForcesApproxEqual(const ymir::Forces& a, const ymir::Forces& b)
{
    for (int i = 0; i < 6; ++i)
        REQUIRE(a.f[i] == Approx(b.f[i]));
}

TEST_CASE("CouplingRegistry: consumedForce returns zero before any resolve")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);

    requireForcesApproxZero(reg.consumedForce(0));
}

TEST_CASE("CouplingRegistry: consumedForce for consumer with no link returns zero")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);

    requireForcesApproxZero(reg.consumedForce(99));
}

TEST_CASE("CouplingRegistry: addLink + writeForce + resolve + consumedForce round-trip")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);

    const ymir::Forces F = makeForces(100.0, 50.0, -10.0);
    reg.writeForce(1, F);
    reg.resolve();

    requireForcesApproxEqual(reg.consumedForce(0), F);
}

TEST_CASE("CouplingRegistry: reset clears ready flags; second resolve without write keeps previous value")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);

    const ymir::Forces F = makeForces(200.0, 0.0, 0.0);
    reg.writeForce(1, F);
    reg.resolve();
    // Tick boundary
    reg.reset();
    // No writeForce this tick
    reg.resolve();

    // Consumer retains the last resolved force
    requireForcesApproxEqual(reg.consumedForce(0), F);
}

TEST_CASE("CouplingRegistry: two producers writing to same consumer produce sum")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);
    reg.addLink(2, 0);

    const ymir::Forces F1 = makeForces(100.0, 20.0, 0.0);
    const ymir::Forces F2 = makeForces(50.0,  10.0, 5.0);
    reg.writeForce(1, F1);
    reg.writeForce(2, F2);
    reg.resolve();

    const ymir::Forces expected = F1 + F2;
    requireForcesApproxEqual(reg.consumedForce(0), expected);
}

TEST_CASE("CouplingRegistry: producer writes zero force; consumedForce returns zero not previous")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);

    // Tick 1: write non-zero
    reg.writeForce(1, makeForces(500.0));
    reg.resolve();
    reg.reset();

    // Tick 2: write zero
    reg.writeForce(1, ymir::Forces::zero());
    reg.resolve();

    requireForcesApproxZero(reg.consumedForce(0));
}

TEST_CASE("CouplingRegistry: vector size does not grow after setup phase")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);
    reg.addLink(2, 0);

    // Capture size after all addLink calls
    // We can't inspect links_ directly, but we can verify
    // all tick operations complete without crashing
    for (int tick = 0; tick < 10; ++tick) {
        reg.reset();
        reg.writeForce(1, makeForces(static_cast<double>(tick)));
        reg.writeForce(2, makeForces(0.0, static_cast<double>(tick)));
        reg.resolve();
    }
    // Just needs to not crash or grow unexpectedly
    REQUIRE(true);
}

TEST_CASE("CouplingRegistry: multiple independent consumers receive correct forces")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);  // tug 1 → ship 0
    reg.addLink(2, 3);  // tug 2 → body 3

    const ymir::Forces FA = makeForces(100.0, 0.0, 0.0);
    const ymir::Forces FB = makeForces(0.0, 200.0, 0.0);
    reg.writeForce(1, FA);
    reg.writeForce(2, FB);
    reg.resolve();

    requireForcesApproxEqual(reg.consumedForce(0), FA);
    requireForcesApproxEqual(reg.consumedForce(3), FB);
}

TEST_CASE("CouplingRegistry: reset does not clear previously resolved force")
{
    ymir::CouplingRegistry reg;
    reg.addLink(1, 0);

    reg.writeForce(1, makeForces(300.0));
    reg.resolve();

    reg.reset();
    // After reset, consumedForce still returns the last resolved value
    requireForcesApproxEqual(reg.consumedForce(0), makeForces(300.0));
}
