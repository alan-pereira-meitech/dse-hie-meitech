#include "jetbus/measurement_utils.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("digit_to_double and double_to_digit roundtrip", "[measurement]") {
    const int decimals = 3;
    const std::int32_t digits = 12345;
    const double value = jetbus::digit_to_double(digits, decimals);
    REQUIRE(value == Catch::Approx(12.345));
    const auto restored = jetbus::double_to_digit(value, decimals);
    REQUIRE(restored == digits);
}

TEST_CASE("string_to_bool interprets zero as false", "[measurement]") {
    REQUIRE_FALSE(jetbus::string_to_bool("0"));
    REQUIRE(jetbus::string_to_bool("1"));
    REQUIRE(jetbus::string_to_bool("42"));
}
