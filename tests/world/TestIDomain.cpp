#include <catch2/catch_test_macros.hpp>

#include <ymir/world/IDomain.h>

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace {

// Minimal stub that satisfies all IDomain pure virtuals.
class StubDomain final : public ymir::IDomain {
public:
    void onAddedToWorld(ymir::Environment&, ymir::CouplingRegistry&) override {}

    void step(double) override {}

    std::vector<ymir::BodyPosition> allBodyPositions() const override
    {
        return {{0, 1.0, 2.0, 3.0}};
    }

    ymir::BodyState bodyState(int) const override { return {}; }

    std::string name() const override { return "stub"; }
};

} // namespace

// --- Compile-time invariants -------------------------------------------------

static_assert(!std::is_default_constructible_v<ymir::IDomain>,
              "IDomain is abstract — must not be default-constructible");

static_assert(!std::is_copy_constructible_v<ymir::IDomain>,
              "IDomain copy constructor is deleted");

static_assert(!std::is_copy_assignable_v<ymir::IDomain>,
              "IDomain copy assignment is deleted");

// --- Runtime tests -----------------------------------------------------------

TEST_CASE("StubDomain implements all IDomain virtuals and compiles")
{
    StubDomain d;
    REQUIRE(d.name() == "stub");
}

TEST_CASE("BodyPosition aggregate-initializes correctly")
{
    ymir::BodyPosition bp{1, 1.0, 2.0, 3.0};
    REQUIRE(bp.id == 1);
    REQUIRE(bp.x  == 1.0);
    REQUIRE(bp.y  == 2.0);
    REQUIRE(bp.z  == 3.0);
}

TEST_CASE("StubDomain allBodyPositions returns expected position")
{
    StubDomain d;
    auto positions = d.allBodyPositions();
    REQUIRE(positions.size() == 1);
    REQUIRE(positions[0].id == 0);
    REQUIRE(positions[0].x  == 1.0);
    REQUIRE(positions[0].y  == 2.0);
    REQUIRE(positions[0].z  == 3.0);
}

TEST_CASE("unique_ptr<IDomain> calls virtual destructor on StubDomain")
{
    bool destroyed = false;

    struct TrackingDomain final : ymir::IDomain {
        bool& flag;
        explicit TrackingDomain(bool& f) : flag(f) {}
        ~TrackingDomain() override { flag = true; }
        void onAddedToWorld(ymir::Environment&, ymir::CouplingRegistry&) override {}
        void step(double) override {}
        std::vector<ymir::BodyPosition> allBodyPositions() const override { return {}; }
        ymir::BodyState bodyState(int) const override { return {}; }
        std::string name() const override { return "tracking"; }
    };

    {
        std::unique_ptr<ymir::IDomain> domain = std::make_unique<TrackingDomain>(destroyed);
        REQUIRE_FALSE(destroyed);
    } // destructor fires here
    REQUIRE(destroyed);
}

TEST_CASE("StubDomain is not copy-constructible (type trait)")
{
    REQUIRE_FALSE(std::is_copy_constructible_v<ymir::IDomain>);
}

TEST_CASE("StubDomain step and bodyState are callable")
{
    StubDomain d;
    d.step(1.0);
    auto state = d.bodyState(0);
    // BodyState default-constructs — no assertion needed beyond compile
    (void)state;
    SUCCEED();
}
