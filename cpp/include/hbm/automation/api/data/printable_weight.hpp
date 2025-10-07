#pragma once

#include <iomanip>
#include <sstream>
#include <string>

namespace hbm::automation::api::data {

struct PrintableWeightType {
    std::string net;
    std::string gross;
    std::string tare;

    void update(double net_value, double gross_value, double tare_value, int decimals)
    {
        net = to_string(net_value, decimals);
        gross = to_string(gross_value, decimals);
        tare = to_string(tare_value, decimals);
    }

private:
    static std::string to_string(double value, int decimals)
    {
        std::ostringstream stream;
        stream.setf(std::ios::fixed, std::ios::floatfield);
        stream << std::setprecision(decimals) << value;
        return stream.str();
    }
};

} // namespace hbm::automation::api::data
