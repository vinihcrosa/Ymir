#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/world/World.h>
#include <ymir/world/IDomain.h>
#include <ymir/world/Environment.h>
#include <ymir/world/CouplingRegistry.h>

#include <memory>
#include <string>

using Catch::Approx;

namespace {

// Minimal stub domain that records the environment current speed at step time.
class EnvCaptureDomain final : public ymir::IDomain {
public:
    void onAddedToWorld(ymir::Environment& env, ymir::CouplingRegistry&) override
    {
        env_ = &env;
    }

    void step(double) override
    {
        capturedCurrentSpeed = env_->currentSpeed();
    }

    std::vector<ymir::BodyPosition> allBodyPositions() const override { return {}; }
    ymir::BodyState bodyState(int) const override { return {}; }
    std::string name() const override { return "capture"; }

    double capturedCurrentSpeed = -1.0;

private:
    ymir::Environment* env_ = nullptr;
};

} // namespace

// ---------------------------------------------------------------------------
// Empty timeline — no-op regression
// ---------------------------------------------------------------------------

TEST_CASE("World step with empty timeline: env.currentSpeed stays 0")
{
    ymir::World world;
    REQUIRE(world.timeline().empty());

    world.step(0.1);

    REQUIRE(world.environment().currentSpeed() == Approx(0.0));
}

TEST_CASE("World step with empty timeline does not throw")
{
    ymir::World world;
    REQUIRE_NOTHROW(world.step(1.0));
}

// ---------------------------------------------------------------------------
// Loaded timeline updates env before domain step
// ---------------------------------------------------------------------------

TEST_CASE("World step with single-keyframe current timeline: currentSpeed == 1.5 after step")
{
    ymir::World world;
    world.timeline().loadJson(R"({
        "currentSeries": [[{"t": 0, "speed": 1.5, "dirNaut": 90}]],
        "windSeries": [],
        "waveSeries": []
    })");

    world.step(0.1);

    REQUIRE(world.environment().currentSpeed() == Approx(1.5));
    REQUIRE(world.environment().currentDirectionNaut() == Approx(90.0));
}

TEST_CASE("World step: timeline resolves env before domain receives it")
{
    ymir::World world;
    world.timeline().loadJson(R"({
        "currentSeries": [[{"t": 0, "speed": 2.0, "dirNaut": 0}]],
        "windSeries": [],
        "waveSeries": []
    })");

    auto domain = std::make_unique<EnvCaptureDomain>();
    auto* raw = domain.get();
    world.addDomain(std::move(domain));

    world.step(0.1);

    // Domain must see the already-resolved current speed during its step().
    REQUIRE(raw->capturedCurrentSpeed == Approx(2.0));
}

// ---------------------------------------------------------------------------
// timeline() getter
// ---------------------------------------------------------------------------

TEST_CASE("World::timeline() getter returns mutable reference usable for loadJson")
{
    ymir::World world;
    REQUIRE(world.timeline().empty());

    world.timeline().loadJson(R"({
        "currentSeries": [[{"t": 0, "speed": 0.5, "dirNaut": 0}]],
        "windSeries": [],
        "waveSeries": []
    })");

    REQUIRE_FALSE(world.timeline().empty());
}

TEST_CASE("World::timeline() const getter returns read-only reference")
{
    ymir::World world;
    const ymir::World& cw = world;
    REQUIRE(cw.timeline().empty());
}

// ---------------------------------------------------------------------------
// Multiple steps — timeline resolves correct time each tick
// ---------------------------------------------------------------------------

TEST_CASE("World step: timeline interpolates at current world time across multiple steps")
{
    // Two keyframes: t=0 speed=0, t=100 speed=2.0
    // After 50 steps of dt=1 → world.time()=50 → speed=1.0
    ymir::World world;
    world.timeline().loadJson(R"({
        "currentSeries": [[
            {"t":   0, "speed": 0.0, "dirNaut": 0},
            {"t": 100, "speed": 2.0, "dirNaut": 0}
        ]],
        "windSeries": [],
        "waveSeries": []
    })");

    for (int i = 0; i < 50; ++i)
        world.step(1.0);

    REQUIRE(world.environment().currentSpeed() == Approx(1.0));
}

// ---------------------------------------------------------------------------
// Existing world behaviour unchanged
// ---------------------------------------------------------------------------

TEST_CASE("World with timeline: time still accumulates correctly")
{
    ymir::World world;
    world.timeline().loadJson(R"({
        "currentSeries": [[{"t": 0, "speed": 1.0, "dirNaut": 0}]],
        "windSeries": [],
        "waveSeries": []
    })");

    world.step(0.5);
    world.step(0.5);

    REQUIRE(world.time() == Approx(1.0));
}
