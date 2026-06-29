#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>

TEST_CASE("nlohmann/json smoke — parse valid JSON", "[world][json]") {
    auto j = nlohmann::json::parse(R"({"ok": true})");
    REQUIRE(j.at("ok").get<bool>() == true);
}

TEST_CASE("nlohmann/json smoke — parse integer field", "[world][json]") {
    auto j = nlohmann::json::parse(R"({"value": 42})");
    REQUIRE(j.at("value").get<int>() == 42);
}

TEST_CASE("nlohmann/json smoke — parse string field", "[world][json]") {
    auto j = nlohmann::json::parse(R"({"name": "ymir"})");
    REQUIRE(j.at("name").get<std::string>() == "ymir");
}

TEST_CASE("nlohmann/json smoke — malformed JSON throws", "[world][json]") {
    REQUIRE_THROWS_AS(
        nlohmann::json::parse("{broken"),
        nlohmann::json::parse_error
    );
}

TEST_CASE("nlohmann/json smoke — missing key throws", "[world][json]") {
    auto j = nlohmann::json::parse(R"({"ok": true})");
    REQUIRE_THROWS_AS(j.at("missing"), nlohmann::json::out_of_range);
}
