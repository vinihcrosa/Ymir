#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <ymir/core/Forces.h>

using Catch::Matchers::WithinAbs;
using namespace ymir;

TEST_CASE("Forces default constructed to zero", "[forces]")
{
    Forces f;
    for (int i = 0; i < 6; ++i)
        REQUIRE_THAT(f.f[i], WithinAbs(0.0, 1e-15));
}

TEST_CASE("Forces::zero() returns zeroed forces", "[forces]")
{
    Forces f = Forces::zero();
    for (int i = 0; i < 6; ++i)
        REQUIRE_THAT(f.f[i], WithinAbs(0.0, 1e-15));
}

TEST_CASE("Forces operator+", "[forces]")
{
    Forces a, b;
    a.f[0] = 100.0;
    a.f[5] = 50.0;
    b.f[0] = 200.0;
    b.f[3] = -10.0;

    Forces c = a + b;
    REQUIRE_THAT(c.f[0], WithinAbs(300.0,  1e-12));
    REQUIRE_THAT(c.f[3], WithinAbs(-10.0,  1e-12));
    REQUIRE_THAT(c.f[5], WithinAbs(50.0,   1e-12));
}

TEST_CASE("Forces operator+=", "[forces]")
{
    Forces a, b;
    a.f[1] = 10.0;
    b.f[1] = 5.0;
    a += b;
    REQUIRE_THAT(a.f[1], WithinAbs(15.0, 1e-12));
}

TEST_CASE("Forces operator-", "[forces]")
{
    Forces a, b;
    a.f[2] = 30.0;
    b.f[2] = 10.0;
    Forces c = a - b;
    REQUIRE_THAT(c.f[2], WithinAbs(20.0, 1e-12));
}

TEST_CASE("Forces operator* scalar", "[forces]")
{
    Forces a;
    a.f[0] = 100.0;
    Forces b = a * 3.0;
    REQUIRE_THAT(b.f[0], WithinAbs(300.0, 1e-12));
    REQUIRE_THAT(a.f[0], WithinAbs(100.0, 1e-12));  // original unchanged
}
