// -----------------------------------------------------------------------------
// Cross-engine golden validation — scenario 02_wind.
//
// Wind 10 m/s from 0° (nautical), vessel free from rest. Same state-in ->
// force-out method as the 08 restoring test: the golden trajectory heading is
// used to rotate the environmental wind into the body frame (GoldenFrame), then
// the reconstructed apparent wind is fed to Ymir's WindForces and compared to
// the golden wind loads.
// -----------------------------------------------------------------------------

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <ymir/physics/BodyState.h>
#include <ymir/vessel/NavalContext.h>

#include "GoldenCsv.h"
#include "GoldenFrame.h"
#include "Vessel1Params.h"

#include <string>

using namespace ymir;
using namespace ymir::naval;
using ymir::test::GoldenCsv;

namespace
{
const std::string kDir = std::string(YMIR_GOLDEN_DIR) + "/02_wind/";

// Environment from the scenario controlDict: wind 10 m/s @ 0° nautical.
constexpr double kWindSpeed = 10.0;
constexpr double kWindDir   = 0.0;
} // namespace

TEST_CASE("Golden 02: wind force matches TMS Dynamics", "[golden][wind]")
{
    GoldenCsv state(kDir + "state.csv");
    GoldenCsv loads(kDir + "loads.csv");

    WindForces model(ymir::test::vessel1Wind());

    int checked = 0;
    for (std::size_t r = 1; r < state.rowCount(); ++r) // row 0 is t=0, all zero
    {
        const double yaw = state.at(r, "yaw");
        Vector6      q{state.at(r, "surge"), state.at(r, "sway"), state.at(r, "heave"),
                       state.at(r, "roll"), state.at(r, "pitch"), yaw};
        Vector6      qdot{state.at(r, "vel_x"), state.at(r, "vel_y"), state.at(r, "vel_z"),
                          state.at(r, "vel_xx"), state.at(r, "vel_yy"), state.at(r, "vel_zz")};
        BodyState    bs(q, qdot, state.at(r, "Time"), 0.1);

        // Body-frame wind reconstructed from the environment + golden heading,
        // exactly as NavalDomain builds speedToWind at run time.
        auto         wind = ymir::test::nautToBodyFrame(kWindSpeed, kWindDir, yaw);
        NavalContext ctx{};
        ctx.state           = bs;
        ctx.speedToWind[0]  = wind[0];
        ctx.speedToWind[1]  = wind[1];
        model.bindContext(&ctx);

        Forces f = model.compute(bs);

        // Longitudinal wind drag. Ymir agrees with TMS in sign and shape to
        // ~4%; the residual is the air-density constant (Ymir rho_air = 1.225
        // vs the reference's ~1.27). A 5% relative tolerance bounds the full
        // trajectory; tighten it if rho_air is reconciled.
        REQUIRE(f.f[0] == Catch::Approx(loads.at(r, "Fwd_x")).epsilon(5e-2).margin(1e3));

        // Beam force and yaw moment are zero for this bow-on heading
        // (cdy = 0 at 180° incidence).
        REQUIRE(f.f[1] == Catch::Approx(loads.at(r, "Fwd_y")).margin(1.0));
        REQUIRE(f.f[5] == Catch::Approx(loads.at(r, "Mwd_z")).margin(1.0));
        ++checked;
    }
    REQUIRE(checked > 100);
}
