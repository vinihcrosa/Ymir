#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <ymir/core/math/LinearAlgebra.h>
#include <ymir/core/math/AngleUtils.h>
#include <ymir/core/math/Interpolation.h>

using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;
using namespace ymir;
using namespace ymir::math;

// ---------------------------------------------------------------------------
// LinearAlgebra
// ---------------------------------------------------------------------------

TEST_CASE("invert6x6 of identity returns identity", "[math][linalg]")
{
    Matrix6x6 I{};
    for (int i = 0; i < 6; ++i)
        I[i][i] = 1.0;

    Matrix6x6 inv = invert6x6(I);

    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            REQUIRE_THAT(inv[i][j], WithinAbs(i == j ? 1.0 : 0.0, 1e-15));
}

TEST_CASE("invert6x6: M * M^-1 ≈ I for diagonal matrix", "[math][linalg]")
{
    Matrix6x6 M{};
    M[0][0] = 1000.0;
    M[1][1] = 1200.0;
    M[2][2] = 1500.0;
    M[3][3] = 5e6;
    M[4][4] = 4e6;
    M[5][5] = 3e6;

    Matrix6x6 inv    = invert6x6(M);
    Matrix6x6 result = matMul(M, inv);

    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            REQUIRE_THAT(result[i][j], WithinAbs(i == j ? 1.0 : 0.0, 1e-10));
}

TEST_CASE("invert6x6 throws on singular matrix", "[math][linalg]")
{
    Matrix6x6 singular{};  // all zeros — singular
    REQUIRE_THROWS_AS(invert6x6(singular), std::runtime_error);
}

TEST_CASE("matVecProduct known result", "[math][linalg]")
{
    Matrix6x6 A{};
    A[0][0] = 2.0;
    A[1][1] = 3.0;
    A[2][2] = 4.0;

    Vector6 x{1.0, 2.0, 3.0, 0.0, 0.0, 0.0};
    Vector6 result = matVecProduct(A, x);

    REQUIRE_THAT(result[0], WithinAbs(2.0, 1e-15));
    REQUIRE_THAT(result[1], WithinAbs(6.0, 1e-15));
    REQUIRE_THAT(result[2], WithinAbs(12.0, 1e-15));
    REQUIRE_THAT(result[3], WithinAbs(0.0, 1e-15));
}

TEST_CASE("matVecProduct with scale", "[math][linalg]")
{
    Matrix6x6 A{};
    A[0][0] = 1.0;

    Vector6 x{5.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    Vector6 result = matVecProduct(A, x, 3.0);

    REQUIRE_THAT(result[0], WithinAbs(15.0, 1e-15));
}

// ---------------------------------------------------------------------------
// AngleUtils
// ---------------------------------------------------------------------------

TEST_CASE("wrapTo2Pi maps to 0..2pi range", "[math][angles]")
{
    constexpr double TWO_PI = 2.0 * math::PI;

    REQUIRE_THAT(wrapTo2Pi(0.0),          WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(wrapTo2Pi(TWO_PI),       WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(wrapTo2Pi(-math::PI),    WithinAbs(math::PI, 1e-12));
    REQUIRE_THAT(wrapTo2Pi(3.0 * TWO_PI), WithinAbs(0.0, 1e-12));
}

TEST_CASE("wrapToPi maps to -pi..pi range", "[math][angles]")
{
    REQUIRE_THAT(wrapToPi(0.0),          WithinAbs(0.0, 1e-12));
    REQUIRE_THAT(wrapToPi(math::PI),     WithinAbs(-math::PI, 1e-12));
    REQUIRE_THAT(wrapToPi(math::PI * 3), WithinAbs(-math::PI, 1e-12));
}

TEST_CASE("deg2rad / rad2deg round-trip", "[math][angles]")
{
    double angles[] = {0.0, 45.0, 90.0, 180.0, 270.0, 360.0, -90.0};
    for (double deg : angles)
        REQUIRE_THAT(rad2deg(deg2rad(deg)), WithinAbs(deg, 1e-12));
}

// ---------------------------------------------------------------------------
// Interpolation
// ---------------------------------------------------------------------------

TEST_CASE("linear interpolation interior point", "[math][interp]")
{
    std::vector<double> x = {0.0, 1.0, 2.0, 3.0};
    std::vector<double> y = {0.0, 1.0, 4.0, 9.0};

    REQUIRE_THAT(linear(x, y, 0.5),  WithinAbs(0.5,  1e-12));
    REQUIRE_THAT(linear(x, y, 1.5),  WithinAbs(2.5,  1e-12));
    REQUIRE_THAT(linear(x, y, 2.5),  WithinAbs(6.5,  1e-12));
}

TEST_CASE("linear interpolation clamp at extremes", "[math][interp]")
{
    std::vector<double> x = {1.0, 2.0, 3.0};
    std::vector<double> y = {10.0, 20.0, 30.0};

    REQUIRE_THAT(linear(x, y, 0.0),  WithinAbs(10.0, 1e-12));  // clamp lo
    REQUIRE_THAT(linear(x, y, 5.0),  WithinAbs(30.0, 1e-12));  // clamp hi
    REQUIRE_THAT(linear(x, y, 1.0),  WithinAbs(10.0, 1e-12));  // exact lo
    REQUIRE_THAT(linear(x, y, 3.0),  WithinAbs(30.0, 1e-12));  // exact hi
}
