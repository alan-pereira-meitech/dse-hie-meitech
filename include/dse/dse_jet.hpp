#pragma once

#include "dse/jet_connection.hpp"
#include "dse/jet_commands.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace dse {

enum class FilterStage {
    Stage2 = 0,
    Stage3,
    Stage4,
    Stage5,
};

enum class FilterMode {
    None = 0,
    FirComb,
    FirMovingAverage,
};

enum class LowPassFilterMode {
    None = 0,
    Iir,
    Fir,
};

struct FilterConfiguration {
    FilterMode mode{FilterMode::None};
    int cutoff_frequency{0};
};

class DSEJet {
  public:
    explicit DSEJet(JetConnection connection);

    void connect();
    void disconnect();
    bool is_connected() const noexcept;

    std::string serial_number();
    std::string identification();
    std::string firmware_version();
    int zero_value_digits();

    FilterMode filter_mode(FilterStage stage);
    void set_filter_mode(FilterStage stage, FilterMode mode);

    int filter_cutoff(FilterStage stage);
    void set_filter_cutoff(FilterStage stage, int value);

    FilterConfiguration filter_configuration(FilterStage stage);
    void apply_filter_configuration(FilterStage stage, const FilterConfiguration& configuration);

    LowPassFilterMode low_pass_filter_mode();
    void set_low_pass_filter_mode(LowPassFilterMode mode);

    int low_pass_cutoff_frequency(LowPassFilterMode mode);
    void set_low_pass_cutoff_frequency(LowPassFilterMode mode, int value);

    void restore_factory_defaults();

  private:
    static const JetCommand& filter_mode_command(FilterStage stage);
    static const JetCommand& comb_frequency_command(FilterStage stage);
    static const JetCommand& moving_average_frequency_command(FilterStage stage);

    FilterMode filter_from_register(int value) const;
    int reserve_filter_index(FilterMode desired);

    JetConnection connection_;
};

}  // namespace dse
