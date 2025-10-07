#include "jetbus/process_data.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <unordered_map>
#include <string>

TEST_CASE("ProcessData decodes status bits and weight values", "[process_data]") {
    jetbus::ProcessData process_data;
    std::unordered_map<std::string, std::string> cache;

    cache[jetbus::commands::cia461_decimals().path] = "1";
    cache[jetbus::commands::cia461_unit().path] = std::to_string(0x00020000);
    cache[jetbus::commands::cia461_net_value().path] = "123";
    cache[jetbus::commands::cia461_gross_value().path] = "456";
    cache[jetbus::commands::cia461_tare_value().path] = "30";
    cache[jetbus::commands::imd_application_mode().path] = "0";

    int status_value = 0;
    status_value |= (1 << 6);  // manual tare
    status_value |= (1 << 11); // center of zero
    cache[jetbus::commands::cia461_weight_status().path] = std::to_string(status_value);

    process_data.update(cache);

    REQUIRE(process_data.decimals() == 1);
    REQUIRE(process_data.unit() == "kg");
    REQUIRE(process_data.weight().net == Catch::Approx(12.3));
    REQUIRE(process_data.weight().gross == Catch::Approx(45.6));
    REQUIRE(process_data.weight().tare == Catch::Approx(3.0));
    REQUIRE(process_data.tare_mode() == dse::TareMode::Tare);
    REQUIRE(process_data.weight_stable());
    REQUIRE(process_data.center_of_zero());
    REQUIRE_FALSE(process_data.zero_required());
    REQUIRE(process_data.legal_for_trade());
    REQUIRE_FALSE(process_data.underload());
    REQUIRE_FALSE(process_data.overload());
    REQUIRE_FALSE(process_data.higher_safe_load_limit());
    REQUIRE_FALSE(process_data.general_scale_error());
    REQUIRE_FALSE(process_data.scale_alarm());
}
