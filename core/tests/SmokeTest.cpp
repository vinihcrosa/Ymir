#include <catch2/catch_test_macros.hpp>
#include <ymir/version.h>

TEST_CASE("build system is wired up", "[smoke]")
{
    REQUIRE(YMIR_VERSION_MAJOR >= 0);
    REQUIRE(YMIR_VERSION_MINOR >= 0);
    REQUIRE(YMIR_VERSION_PATCH >= 0);
}

TEST_CASE("version string is non-empty", "[smoke]")
{
    std::string v = YMIR_VERSION_STRING;
    REQUIRE_FALSE(v.empty());
}
