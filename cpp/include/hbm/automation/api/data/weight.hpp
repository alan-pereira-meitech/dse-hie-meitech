#pragma once

namespace hbm::automation::api::data {

struct WeightType {
    double net{0.0};
    double gross{0.0};
    double tare{0.0};

    void update(double net_value, double gross_value, double tare_value)
    {
        net = net_value;
        gross = gross_value;
        tare = tare_value;
    }
};

} // namespace hbm::automation::api::data
