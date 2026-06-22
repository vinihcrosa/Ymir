#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <ymir/core/RigidBody6DOF.h>
#include <ymir/core/math/LinearAlgebra.h>

using Catch::Matchers::WithinAbs;
using namespace ymir;

static Matrix6x6 diagonalMass(double m, double Ixx, double Iyy, double Izz)
{
    Matrix6x6 M{};
    M[0][0] = m;
    M[1][1] = m;
    M[2][2] = m;
    M[3][3] = Ixx;
    M[4][4] = Iyy;
    M[5][5] = Izz;
    return M;
}

TEST_CASE("RigidBody6DOF: invTotalMass * totalMass ≈ I", "[body]")
{
    Matrix6x6 mass  = diagonalMass(1000.0, 5e5, 4e5, 3e5);
    Matrix6x6 added = diagonalMass(200.0,  1e5, 0.8e5, 0.6e5);
    Vector6   q{}, qdot{};

    RigidBody6DOF body{0, mass, added, q, qdot};

    auto result = math::matMul(body.totalMass(), body.invTotalMass());

    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            REQUIRE_THAT(result[i][j], WithinAbs(i == j ? 1.0 : 0.0, 1e-10));
}

TEST_CASE("RigidBody6DOF: computeAcceleration F/m for diagonal mass", "[body]")
{
    Matrix6x6 mass = diagonalMass(1000.0, 1e5, 1e5, 1e5);
    Matrix6x6 added{};
    Vector6   q{}, qdot{};

    RigidBody6DOF body{0, mass, added, q, qdot};

    Forces F;
    F.f[0] = 500.0;  // 500 N in surge

    Vector6 acc = body.computeAcceleration(F);

    // a = F/m = 500/1000 = 0.5 m/s²
    REQUIRE_THAT(acc[0], WithinAbs(0.5, 1e-12));
    for (int i = 1; i < 6; ++i)
        REQUIRE_THAT(acc[i], WithinAbs(0.0, 1e-12));
}

TEST_CASE("RigidBody6DOF: state() returns correct snapshot", "[body]")
{
    Matrix6x6 mass = diagonalMass(1000.0, 1e5, 1e5, 1e5);
    Matrix6x6 added{};
    Vector6   q{1.0, 2.0, 3.0, 0.1, 0.2, 0.3};
    Vector6   qdot{4.0, 5.0, 6.0, 0.4, 0.5, 0.6};

    RigidBody6DOF body{0, mass, added, q, qdot};
    BodyState     s = body.state();

    REQUIRE_THAT(s.x(),    WithinAbs(1.0, 1e-15));
    REQUIRE_THAT(s.y(),    WithinAbs(2.0, 1e-15));
    REQUIRE_THAT(s.yaw(),  WithinAbs(0.3, 1e-15));
    REQUIRE_THAT(s.u(),    WithinAbs(4.0, 1e-15));
    REQUIRE_THAT(s.r(),    WithinAbs(0.6, 1e-15));
    REQUIRE_THAT(s.time(), WithinAbs(0.0, 1e-15));  // time starts at 0
}
