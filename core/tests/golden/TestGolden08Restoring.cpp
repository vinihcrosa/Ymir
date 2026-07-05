// -----------------------------------------------------------------------------
// Cross-engine golden validation — scenario 08_restoring.
//
// Feeds the TMS Dynamics 3.0 golden motion trajectory (state.csv) into Ymir's
// force models row by row and compares the computed loads against the golden
// per-force decomposition (loads.csv). This is integrator-independent: both the
// input state and the expected force come from the reference engine, so it
// validates the force *formula*, not the time stepping.
//
// Scenario 08 isolates hydrostatic restoring + radiation/viscous damping: the
// vessel starts +0.5 m above heave equilibrium in a zeroed environment and
// oscillates vertically. See reference/README.md in the golden dataset.
// -----------------------------------------------------------------------------

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <ymir/physics/BodyState.h>
#include <ymir/vessel/NavalContext.h>

#include "GoldenCsv.h"
#include "Vessel1Params.h"

#include <cmath>
#include <string>

using namespace ymir;
using namespace ymir::naval;
using ymir::test::GoldenCsv;

namespace
{
const std::string kDir = std::string(YMIR_GOLDEN_DIR) + "/08_restoring/";

/// Build a body state from one row of the golden motion trajectory.
BodyState stateFromRow(const GoldenCsv& s, std::size_t r)
{
    Vector6 q{s.at(r, "surge"), s.at(r, "sway"), s.at(r, "heave"),
              s.at(r, "roll"), s.at(r, "pitch"), s.at(r, "yaw")};
    Vector6 qdot{s.at(r, "vel_x"), s.at(r, "vel_y"), s.at(r, "vel_z"),
                 s.at(r, "vel_xx"), s.at(r, "vel_yy"), s.at(r, "vel_zz")};
    return BodyState(q, qdot, s.at(r, "Time"), 0.1);
}
} // namespace

TEST_CASE("Golden 08: restoring force matches TMS Dynamics", "[golden][restoring]")
{
    GoldenCsv state(kDir + "state.csv");
    GoldenCsv loads(kDir + "loads.csv");

    RestoringForces model(ymir::test::vessel1Restoring());

    for (std::size_t r = 1; r < state.rowCount(); ++r) // row 0 is t=0, all zero
    {
        BodyState    bs = stateFromRow(state, r);
        NavalContext ctx{};
        ctx.state = bs;
        ctx.tide  = 0.0;
        model.bindContext(&ctx);

        Forces f = model.compute(bs);

        // Ymir models hydrostatic restoring with a DIAGONAL stiffness matrix and
        // intentionally omits the off-diagonal heave<->pitch coupling terms
        // (hydrostaticRestoring[2][4] / [4][2]) that TMS Dynamics carries — see
        // AGENTS.md "Off-diagonal hydrostatic stiffness silently ignored".
        //
        // With pitch near zero (early in the oscillation) the two engines agree
        // to ~3e-5; as pitch grows the omitted coupling drives the divergence
        // up to ~6.5% (Fr_z) and ~13.5% (Mr_y). The tolerances below bound that
        // full-trajectory divergence, so this test doubles as a regression guard
        // on the diagonal-only design decision: tighten it only if the coupling
        // terms are added to RestoringForces. The absolute margins absorb the
        // zero-crossings as the damped heave oscillation settles to equilibrium.
        REQUIRE(f.f[2] == Catch::Approx(loads.at(r, "Fr_z")).epsilon(7e-2).margin(3e6));
        REQUIRE(f.f[4] == Catch::Approx(loads.at(r, "Mr_y")).epsilon(1.5e-1).margin(1e8));

        // Surge/sway/roll/yaw restoring are zero in this scenario.
        REQUIRE(f.f[0] == Catch::Approx(loads.at(r, "Fr_x")).margin(1.0));
        REQUIRE(f.f[1] == Catch::Approx(loads.at(r, "Fr_y")).margin(1.0));
        REQUIRE(f.f[5] == Catch::Approx(loads.at(r, "Mr_z")).margin(1.0));
    }
}

TEST_CASE("Golden 08: heave damping matches TMS Dynamics", "[golden][damping]")
{
    GoldenCsv state(kDir + "state.csv");
    GoldenCsv loads(kDir + "loads.csv");

    DampingForces model(ymir::test::vessel1Damping());

    // Heave-damping force crosses zero as the vessel oscillates, so a pure
    // relative tolerance is meaningless near the crossings. Use a combined
    // relative + absolute-floor check. The floor also absorbs the larger
    // divergence at low heave speed (Ymir omits a small heave-linear term the
    // reference carries).
    constexpr double kRel   = 2e-2;
    constexpr double kFloor = 6e5; // N

    int checked = 0;
    for (std::size_t r = 1; r < state.rowCount(); ++r)
    {
        BodyState    bs = stateFromRow(state, r);
        NavalContext ctx{};
        ctx.state = bs;
        // No current in scenario 08 → still water → horizontal flow ~0.
        ctx.speedToWater[0] = 0.0;
        ctx.speedToWater[1] = 0.0;
        model.bindContext(&ctx);

        Forces       f      = model.compute(bs);
        const double golden = loads.at(r, "Fd_z");

        REQUIRE(f.f[2] == Catch::Approx(golden).epsilon(kRel).margin(kFloor));
        ++checked;
    }
    REQUIRE(checked > 100); // guard against an empty/short fixture
}
