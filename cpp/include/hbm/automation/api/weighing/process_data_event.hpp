#pragma once

#include <functional>

#include "hbm/automation/api/data/process_data.hpp"

namespace hbm::automation::api::weighing {

struct ProcessDataReceivedEventArgs {
    explicit ProcessDataReceivedEventArgs(const data::IProcessData& process_data)
        : process_data(process_data) {}

    const data::IProcessData& process_data;
};

using ProcessDataHandler = std::function<void(const ProcessDataReceivedEventArgs&)>;

} // namespace hbm::automation::api::weighing
