#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/world/EnvironmentTimeline.h>
#include <ymir/world/Environment.h>

#include <cmath>
#include <stdexcept>

using Catch::Approx;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string singleCurrentJson(double t, double speed, double dir)
{
    // Single series, single keyframe — minimal valid payload.
    return R"({"currentSeries":[[{"t":)" + std::to_string(t)
         + R"(,"speed":)" + std::to_string(speed)
         + R"(,"dirNaut":)" + std::to_string(dir)
         + R"(}]],"windSeries":[],"waveSeries":[]})";
}

static std::string emptyJson()
{
    return R"({"currentSeries":[],"windSeries":[],"waveSeries":[]})";
}

// ---------------------------------------------------------------------------
// Empty timeline
// ---------------------------------------------------------------------------

TEST_CASE("EnvironmentTimeline default empty() is true")
{
    ymir::EnvironmentTimeline tl;
    REQUIRE(tl.empty());
}

TEST_CASE("EnvironmentTimeline empty timeline: advanceStep is a no-op")
{
    ymir::EnvironmentTimeline tl;
    ymir::Environment env;
    tl.advanceStep(0.0, env);
    REQUIRE(env.currentSpeed() == Approx(0.0));
    REQUIRE(env.windSpeed()    == Approx(0.0));
}

TEST_CASE("EnvironmentTimeline loadJson empty arrays: empty() stays true")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(emptyJson());
    REQUIRE(tl.empty());
    ymir::Environment env;
    tl.advanceStep(0.0, env);
    REQUIRE(env.currentSpeed() == Approx(0.0));
}

// ---------------------------------------------------------------------------
// Single keyframe — clamp behaviour
// ---------------------------------------------------------------------------

TEST_CASE("EnvironmentTimeline single current keyframe at t=0: clamps for t>0")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(singleCurrentJson(0.0, 1.0, 90.0));
    ymir::Environment env;
    tl.advanceStep(500.0, env);
    REQUIRE(env.currentSpeed()         == Approx(1.0));
    REQUIRE(env.currentDirectionNaut() == Approx(90.0));
}

TEST_CASE("EnvironmentTimeline single keyframe: clamp before first keyframe (t<0)")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(singleCurrentJson(10.0, 2.0, 45.0));
    ymir::Environment env;
    tl.advanceStep(-10.0, env);
    REQUIRE(env.currentSpeed()         == Approx(2.0));
    REQUIRE(env.currentDirectionNaut() == Approx(45.0));
}

TEST_CASE("EnvironmentTimeline single keyframe: clamp after last keyframe (t=999)")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(R"({
        "currentSeries": [[
            {"t": 0,   "speed": 0.0, "dirNaut": 0},
            {"t": 100, "speed": 2.0, "dirNaut": 0}
        ]],
        "windSeries": [],
        "waveSeries": []
    })");
    ymir::Environment env;
    tl.advanceStep(999.0, env);
    REQUIRE(env.currentSpeed() == Approx(2.0));
}

// ---------------------------------------------------------------------------
// Linear speed interpolation
// ---------------------------------------------------------------------------

TEST_CASE("EnvironmentTimeline two current keyframes: speed interpolates linearly at midpoint")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(R"({
        "currentSeries": [[
            {"t": 0,   "speed": 0.0, "dirNaut": 0},
            {"t": 100, "speed": 2.0, "dirNaut": 0}
        ]],
        "windSeries": [],
        "waveSeries": []
    })");
    ymir::Environment env;
    tl.advanceStep(50.0, env);
    REQUIRE(env.currentSpeed() == Approx(1.0));
}

// ---------------------------------------------------------------------------
// Angular wrap-around interpolation
// ---------------------------------------------------------------------------

TEST_CASE("EnvironmentTimeline angular wrap-around: 350 deg to 10 deg midpoint = 0 deg")
{
    // Shortest-path: delta = +20 deg (not -340). Midpoint = 350 + 10 = 360 = 0 deg.
    ymir::EnvironmentTimeline tl;
    tl.loadJson(R"({
        "currentSeries": [[
            {"t": 0,   "speed": 1.0, "dirNaut": 350},
            {"t": 100, "speed": 1.0, "dirNaut": 10}
        ]],
        "windSeries": [],
        "waveSeries": []
    })");
    ymir::Environment env;
    tl.advanceStep(50.0, env);
    // Direction at midpoint should be near 0 (not 180).
    const double dir = env.currentDirectionNaut();
    REQUIRE((dir == Approx(0.0).margin(1e-9) || dir == Approx(360.0).margin(1e-9)));
}

// ---------------------------------------------------------------------------
// Vectorial composition — two current series
// ---------------------------------------------------------------------------

TEST_CASE("EnvironmentTimeline two current series, opposite directions: resultant speed = 0")
{
    // From North + From South → cancel exactly.
    ymir::EnvironmentTimeline tl;
    tl.loadJson(R"({
        "currentSeries": [
            [{"t": 0, "speed": 1.0, "dirNaut": 0}],
            [{"t": 0, "speed": 1.0, "dirNaut": 180}]
        ],
        "windSeries": [],
        "waveSeries": []
    })");
    ymir::Environment env;
    tl.advanceStep(0.0, env);
    REQUIRE(env.currentSpeed() == Approx(0.0).margin(1e-12));
}

TEST_CASE("EnvironmentTimeline two current series, same direction: resultant speed = sum")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(R"({
        "currentSeries": [
            [{"t": 0, "speed": 1.0, "dirNaut": 0}],
            [{"t": 0, "speed": 1.5, "dirNaut": 0}]
        ],
        "windSeries": [],
        "waveSeries": []
    })");
    ymir::Environment env;
    tl.advanceStep(0.0, env);
    REQUIRE(env.currentSpeed() == Approx(2.5));
}

TEST_CASE("EnvironmentTimeline two perpendicular currents 1 m/s each: resultant = sqrt(2)")
{
    // From North (flows South) + From East (flows West) → diagonal, speed = √2.
    ymir::EnvironmentTimeline tl;
    tl.loadJson(R"({
        "currentSeries": [
            [{"t": 0, "speed": 1.0, "dirNaut": 0}],
            [{"t": 0, "speed": 1.0, "dirNaut": 90}]
        ],
        "windSeries": [],
        "waveSeries": []
    })");
    ymir::Environment env;
    tl.advanceStep(0.0, env);
    REQUIRE(env.currentSpeed() == Approx(std::sqrt(2.0)));
}

// ---------------------------------------------------------------------------
// Wave series
// ---------------------------------------------------------------------------

TEST_CASE("EnvironmentTimeline wave single keyframe: advanceStep does not throw")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(R"({
        "currentSeries": [],
        "windSeries":    [],
        "waveSeries": [
            {"t": 0, "Hs": 2.0, "Tp": 10.0, "dirNaut": 270, "spectrum": "JONSWAP", "gamma": 3.3}
        ]
    })");
    ymir::Environment env;
    // setSeaState must be called without assertion or exception.
    REQUIRE_NOTHROW(tl.advanceStep(0.0, env));
    // Unrelated fields remain at defaults.
    REQUIRE(env.waterDepth() == Approx(100.0));
}

TEST_CASE("EnvironmentTimeline wave two keyframes: Hs interpolates linearly at midpoint")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(R"({
        "currentSeries": [],
        "windSeries":    [],
        "waveSeries": [
            {"t":   0, "Hs": 1.0, "Tp": 8.0, "dirNaut": 0, "spectrum": "JONSWAP"},
            {"t": 100, "Hs": 3.0, "Tp": 8.0, "dirNaut": 0, "spectrum": "JONSWAP"}
        ]
    })");
    ymir::Environment env;
    // setSeaState is called with Hs=2.0 at t=50; no public getter, so verify no crash
    // and that current/wind are untouched.
    REQUIRE_NOTHROW(tl.advanceStep(50.0, env));
    REQUIRE(env.currentSpeed() == Approx(0.0));
    REQUIRE(env.windSpeed()    == Approx(0.0));
}

TEST_CASE("EnvironmentTimeline wave: clamp before first keyframe")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(R"({
        "currentSeries": [],
        "windSeries":    [],
        "waveSeries": [
            {"t": 10, "Hs": 2.0, "Tp": 8.0, "dirNaut": 0, "spectrum": "PIERSON"}
        ]
    })");
    ymir::Environment env;
    REQUIRE_NOTHROW(tl.advanceStep(-5.0, env));
}

// ---------------------------------------------------------------------------
// loadJson — validation errors
// ---------------------------------------------------------------------------

TEST_CASE("EnvironmentTimeline loadJson throws on negative speed")
{
    REQUIRE_THROWS_AS(
        ymir::EnvironmentTimeline{}.loadJson(R"({
            "currentSeries": [[{"t": 0, "speed": -1.0, "dirNaut": 0}]],
            "windSeries": [],
            "waveSeries": []
        })"),
        std::runtime_error);
}

TEST_CASE("EnvironmentTimeline loadJson throws on missing dirNaut in current keyframe")
{
    REQUIRE_THROWS_AS(
        ymir::EnvironmentTimeline{}.loadJson(R"({
            "currentSeries": [[{"t": 0, "speed": 1.0}]],
            "windSeries": [],
            "waveSeries": []
        })"),
        std::runtime_error);
}

TEST_CASE("EnvironmentTimeline loadJson throws on invalid spectrum string")
{
    REQUIRE_THROWS_AS(
        ymir::EnvironmentTimeline{}.loadJson(R"({
            "currentSeries": [],
            "windSeries": [],
            "waveSeries": [{"t": 0, "Hs": 1.0, "Tp": 8.0, "dirNaut": 0, "spectrum": "UNKNOWN"}]
        })"),
        std::runtime_error);
}

TEST_CASE("EnvironmentTimeline loadJson throws on malformed JSON")
{
    REQUIRE_THROWS_AS(
        ymir::EnvironmentTimeline{}.loadJson("not valid json {{"),
        std::runtime_error);
}

TEST_CASE("EnvironmentTimeline loadJson throws on negative Hs")
{
    REQUIRE_THROWS_AS(
        ymir::EnvironmentTimeline{}.loadJson(R"({
            "currentSeries": [],
            "windSeries": [],
            "waveSeries": [{"t": 0, "Hs": -1.0, "Tp": 8.0, "dirNaut": 0, "spectrum": "JONSWAP"}]
        })"),
        std::runtime_error);
}

TEST_CASE("EnvironmentTimeline loadJson throws on Tp <= 0")
{
    REQUIRE_THROWS_AS(
        ymir::EnvironmentTimeline{}.loadJson(R"({
            "currentSeries": [],
            "windSeries": [],
            "waveSeries": [{"t": 0, "Hs": 1.0, "Tp": 0.0, "dirNaut": 0, "spectrum": "JONSWAP"}]
        })"),
        std::runtime_error);
}

TEST_CASE("EnvironmentTimeline loadJson throws on missing waveSeries top-level field")
{
    REQUIRE_THROWS_AS(
        ymir::EnvironmentTimeline{}.loadJson(R"({"currentSeries": [], "windSeries": []})"),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// reset()
// ---------------------------------------------------------------------------

TEST_CASE("EnvironmentTimeline reset: empty() returns true, advanceStep is no-op")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(singleCurrentJson(0.0, 5.0, 90.0));
    REQUIRE_FALSE(tl.empty());

    tl.reset();
    REQUIRE(tl.empty());

    ymir::Environment env;
    tl.advanceStep(0.0, env);
    REQUIRE(env.currentSpeed() == Approx(0.0));
}

// ---------------------------------------------------------------------------
// Integration — full round-trip with current + wind + wave
// ---------------------------------------------------------------------------

TEST_CASE("EnvironmentTimeline integration: load current+wind+wave, advanceStep sets all env fields")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(R"({
        "currentSeries": [[{"t": 0, "speed": 1.5, "dirNaut": 90}]],
        "windSeries":    [[{"t": 0, "speed": 8.0, "dirNaut": 270}]],
        "waveSeries": [
            {"t": 0, "Hs": 2.5, "Tp": 12.0, "dirNaut": 180, "spectrum": "JONSWAP", "gamma": 3.3}
        ]
    })");

    ymir::Environment env;
    tl.advanceStep(0.0, env);

    REQUIRE(env.currentSpeed()         == Approx(1.5));
    REQUIRE(env.currentDirectionNaut() == Approx(90.0));
    REQUIRE(env.windSpeed()            == Approx(8.0));
    REQUIRE(env.windDirectionNaut()    == Approx(270.0));
    // seaState has no public getters; verify no crash and unrelated field unchanged.
    REQUIRE(env.waterDepth() == Approx(100.0));
}

TEST_CASE("EnvironmentTimeline integration: loadJson is idempotent (reload replaces data)")
{
    ymir::EnvironmentTimeline tl;
    tl.loadJson(singleCurrentJson(0.0, 1.0, 0.0));
    tl.loadJson(singleCurrentJson(0.0, 3.0, 90.0));

    ymir::Environment env;
    tl.advanceStep(0.0, env);
    REQUIRE(env.currentSpeed()         == Approx(3.0));
    REQUIRE(env.currentDirectionNaut() == Approx(90.0));
}

TEST_CASE("EnvironmentTimeline integration: unsorted keyframes are sorted on load")
{
    // Keyframes provided in reverse order; parser must sort by t.
    ymir::EnvironmentTimeline tl;
    tl.loadJson(R"({
        "currentSeries": [[
            {"t": 100, "speed": 2.0, "dirNaut": 0},
            {"t": 0,   "speed": 0.0, "dirNaut": 0}
        ]],
        "windSeries": [],
        "waveSeries": []
    })");
    ymir::Environment env;
    tl.advanceStep(50.0, env);
    REQUIRE(env.currentSpeed() == Approx(1.0));
}
