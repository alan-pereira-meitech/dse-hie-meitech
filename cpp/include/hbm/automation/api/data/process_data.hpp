#pragma once

#include <string>

#include "hbm/automation/api/data/printable_weight.hpp"
#include "hbm/automation/api/data/weight.hpp"
#include "hbm/automation/api/enums.hpp"

namespace hbm::automation::api::data {

class IProcessData {
public:
    virtual ~IProcessData() = default;

    virtual ApplicationMode application_mode() const = 0;
    virtual const WeightType& weight() const = 0;
    virtual const PrintableWeightType& printable_weight() const = 0;
    virtual const std::string& unit() const = 0;
    virtual int decimals() const = 0;
    virtual TareMode tare_mode() const = 0;
    virtual bool weight_stable() const = 0;
    virtual bool center_of_zero() const = 0;
    virtual bool inside_zero() const = 0;
    virtual bool zero_required() const = 0;
    virtual int scale_range() const = 0;
    virtual bool general_scale_error() const = 0;
    virtual bool legal_for_trade() const = 0;
    virtual bool underload() const = 0;
    virtual bool overload() const = 0;
    virtual bool higher_safe_load_limit() const = 0;
};

} // namespace hbm::automation::api::data
