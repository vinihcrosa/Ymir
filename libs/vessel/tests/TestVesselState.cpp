#include <catch2/catch_test_macros.hpp>

#include <ymir/vessel/VesselState.h>

using namespace ymir::vessel;

TEST_CASE("VesselState default construction", "[VesselState]") {
    VesselState s;

    SECTION("operationalState defaults to Underway") {
        REQUIRE(s.operationalState == OperationalState::Underway);
    }

    SECTION("shapes vector is empty") {
        REQUIRE(s.shapes.empty());
    }

    SECTION("all navigation lights default to Off") {
        REQUIRE(s.lights.mast      == NavigationLight::Off);
        REQUIRE(s.lights.port      == NavigationLight::Off);
        REQUIRE(s.lights.starboard == NavigationLight::Off);
        REQUIRE(s.lights.stern     == NavigationLight::Off);
        REQUIRE(s.lights.anchor    == NavigationLight::Off);
    }
}

TEST_CASE("VesselState field assignment", "[VesselState]") {
    VesselState s;

    SECTION("changing operationalState preserves lights and shapes") {
        s.operationalState = OperationalState::Anchored;
        REQUIRE(s.operationalState == OperationalState::Anchored);
        REQUIRE(s.lights.mast == NavigationLight::Off);
        REQUIRE(s.shapes.empty());
    }

    SECTION("push_back ColregsShape adds one element") {
        s.shapes.push_back(ColregsShape::Ball);
        REQUIRE(s.shapes.size() == 1);
        REQUIRE(s.shapes[0] == ColregsShape::Ball);
    }

    SECTION("multiple shapes can coexist") {
        s.shapes.push_back(ColregsShape::Ball);
        s.shapes.push_back(ColregsShape::Diamond);
        REQUIRE(s.shapes.size() == 2);
    }
}

TEST_CASE("VesselState field-by-field equality of two defaults", "[VesselState]") {
    VesselState a;
    VesselState b;

    REQUIRE(a.operationalState == b.operationalState);
    REQUIRE(a.shapes.size()    == b.shapes.size());
    REQUIRE(a.lights.mast      == b.lights.mast);
    REQUIRE(a.lights.port      == b.lights.port);
    REQUIRE(a.lights.starboard == b.lights.starboard);
    REQUIRE(a.lights.stern     == b.lights.stern);
    REQUIRE(a.lights.anchor    == b.lights.anchor);
}
