#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "hbm/automation/api/enums.hpp"

namespace hbm::automation::api::weighing::wtx::jet {

class JetBusCommand {
public:
    JetBusCommand(DataType data_type, const char* path, int bit_index, int bit_length)
        : data_type_(data_type), path_(path), bit_index_(bit_index), bit_length_(bit_length)
    {
    }

    [[nodiscard]] DataType data_type() const noexcept { return data_type_; }
    [[nodiscard]] const std::string& path() const noexcept { return path_; }
    [[nodiscard]] int bit_index() const noexcept { return bit_index_; }
    [[nodiscard]] int bit_length() const noexcept { return bit_length_; }

    [[nodiscard]] std::string to_string(const std::string& input) const;
    [[nodiscard]] int to_int(const std::string& input) const;
    [[nodiscard]] std::string format_value(int value) const;

private:
    [[nodiscard]] int extract_bit(int value) const;

    DataType data_type_;
    std::string path_;
    int bit_index_;
    int bit_length_;
};

} // namespace hbm::automation::api::weighing::wtx::jet
