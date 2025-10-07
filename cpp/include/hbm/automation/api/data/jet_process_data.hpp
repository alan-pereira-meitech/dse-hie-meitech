#pragma once

#include <string>

#include "hbm/automation/api/data/process_data.hpp"
#include "hbm/automation/api/net_connection.hpp"
#include "hbm/automation/api/utils/measurement_utils.hpp"
#include "hbm/automation/api/weighing/wtx/jet/jet_bus_commands.hpp"

namespace hbm::automation::api::data {

class JetProcessData : public IProcessData {
public:
    JetProcessData();

    void refresh(const INetConnection& connection);

    ApplicationMode application_mode() const override;
    const WeightType& weight() const override;
    const PrintableWeightType& printable_weight() const override;
    const std::string& unit() const override;
    int decimals() const override;
    TareMode tare_mode() const override;
    bool weight_stable() const override;
    bool center_of_zero() const override;
    bool inside_zero() const override;
    bool zero_required() const override;
    int scale_range() const override;
    bool general_scale_error() const override;
    bool legal_for_trade() const override;
    bool underload() const override;
    bool overload() const override;
    bool higher_safe_load_limit() const override;

private:
    static TareMode evaluate_tare_mode(int manual_tare, int weight_type);
    static std::string unit_id_to_string(int unit_id);

    ApplicationMode application_mode_{ApplicationMode::Standard};
    WeightType weight_{};
    PrintableWeightType printable_weight_{};
    std::string unit_{};
    int decimals_{0};
    TareMode tare_mode_{TareMode::None};
    bool weight_stable_{false};
    bool center_of_zero_{false};
    bool inside_zero_{false};
    bool zero_required_{false};
    int scale_range_{0};
    bool general_scale_error_{false};
    bool legal_for_trade_{false};
    bool underload_{false};
    bool overload_{false};
    bool higher_safe_load_limit_{false};
};

} // namespace hbm::automation::api::data
