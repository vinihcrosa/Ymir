#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <ymir/physics/ForceModel.h>

using Catch::Matchers::WithinAbs;
using namespace ymir;

TEST_CASE("ZeroForceModel always returns zero forces", "[forcemodel]")
{
    ZeroForceModel model;

    BodyState state{Vector6{}, Vector6{}, 0.0, 0.1};
    Forces    f = model.compute(state);

    for (int i = 0; i < 6; ++i)
        REQUIRE_THAT(f.f[i], WithinAbs(0.0, 1e-15));
}

TEST_CASE("ZeroForceModel name is 'zero'", "[forcemodel]")
{
    ZeroForceModel model;
    REQUIRE(model.name() == "zero");
}

TEST_CASE("ForceModel default name is 'unnamed'", "[forcemodel]")
{
    // Minimal concrete implementation
    struct TestForce : ForceModel
    {
        Forces compute(const BodyState&) override { return Forces::zero(); }
    };

    TestForce f;
    REQUIRE(f.name() == "unnamed");
}
