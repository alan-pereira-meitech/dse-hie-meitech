#include "dse/dse_jet.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include <utility>

namespace dse {
namespace {

const JetCommand& command_for_stage(const std::array<JetCommand, 4>& commands, FilterStage stage) {
    const auto index = static_cast<std::size_t>(stage);
    if (index >= commands.size()) {
        throw std::out_of_range("invalid filter stage");
    }
    return commands[index];
}

}  // namespace

DSEJet::DSEJet(JetConnection connection) : connection_(std::move(connection)) {}

void DSEJet::connect() { connection_.connect(); }

void DSEJet::disconnect() { connection_.disconnect(); }

bool DSEJet::is_connected() const noexcept { return connection_.is_connected(); }

std::string DSEJet::serial_number() { return connection_.read(JetCommands::DSESerialNumber); }

std::string DSEJet::identification() { return connection_.read(JetCommands::DSEIdentification); }

std::string DSEJet::firmware_version() { return connection_.read(JetCommands::DSEFirmwareVersion); }

int DSEJet::zero_value_digits() { return connection_.read_integer(JetCommands::DSEZeroValue); }

FilterMode DSEJet::filter_mode(FilterStage stage) {
    const int value = connection_.read_integer(filter_mode_command(stage));
    return filter_from_register(value);
}

void DSEJet::set_filter_mode(FilterStage stage, FilterMode mode) {
    const JetCommand& command = filter_mode_command(stage);
    const int current_value = connection_.read_integer(command);
    if (filter_from_register(current_value) == mode) {
        return;
    }

    int register_value = 0;
    switch (mode) {
        case FilterMode::None:
            register_value = 0;
            break;
        case FilterMode::FirComb:
        case FilterMode::FirMovingAverage:
            register_value = reserve_filter_index(mode);
            break;
        default:
            throw std::invalid_argument("unsupported filter mode");
    }

    connection_.write_integer(command, register_value);
}

int DSEJet::filter_cutoff(FilterStage stage) {
    const FilterMode mode = filter_mode(stage);
    switch (mode) {
        case FilterMode::None:
            return 0;
        case FilterMode::FirComb:
            return connection_.read_integer(comb_frequency_command(stage));
        case FilterMode::FirMovingAverage:
            return connection_.read_integer(moving_average_frequency_command(stage));
        default:
            throw std::invalid_argument("unsupported filter mode");
    }
}

void DSEJet::set_filter_cutoff(FilterStage stage, int value) {
    const FilterMode mode = filter_mode(stage);
    switch (mode) {
        case FilterMode::FirComb:
            connection_.write_integer(comb_frequency_command(stage), value);
            break;
        case FilterMode::FirMovingAverage:
            connection_.write_integer(moving_average_frequency_command(stage), value);
            break;
        case FilterMode::None:
            break;
        default:
            throw std::invalid_argument("unsupported filter mode");
    }
}

FilterConfiguration DSEJet::filter_configuration(FilterStage stage) {
    FilterConfiguration configuration;
    configuration.mode = filter_mode(stage);
    configuration.cutoff_frequency = filter_cutoff(stage);
    return configuration;
}

void DSEJet::apply_filter_configuration(FilterStage stage, const FilterConfiguration& configuration) {
    set_filter_mode(stage, configuration.mode);
    if (configuration.mode != FilterMode::None) {
        set_filter_cutoff(stage, configuration.cutoff_frequency);
    }
}

LowPassFilterMode DSEJet::low_pass_filter_mode() {
    const int raw = connection_.read_integer(JetCommands::CIA461ScaleFilter);
    switch (raw) {
        case 24737:
            return LowPassFilterMode::Iir;
        case 13073:
            return LowPassFilterMode::Fir;
        default:
            return LowPassFilterMode::None;
    }
}

void DSEJet::set_low_pass_filter_mode(LowPassFilterMode mode) {
    int value = 0;
    switch (mode) {
        case LowPassFilterMode::None:
            value = 0;
            break;
        case LowPassFilterMode::Iir:
            value = 24737;
            break;
        case LowPassFilterMode::Fir:
            value = 13073;
            break;
        default:
            throw std::invalid_argument("invalid low pass filter mode");
    }
    connection_.write_integer(JetCommands::CIA461ScaleFilter, value);
}

int DSEJet::low_pass_cutoff_frequency(LowPassFilterMode mode) {
    switch (mode) {
        case LowPassFilterMode::Fir:
            return connection_.read_integer(JetCommands::DSELowPassCutOffFrequencyFIR);
        case LowPassFilterMode::Iir:
            return connection_.read_integer(JetCommands::DSELowPassCutOffFrequencyIIR);
        case LowPassFilterMode::None:
            return 0;
        default:
            throw std::invalid_argument("invalid low pass filter mode");
    }
}

void DSEJet::set_low_pass_cutoff_frequency(LowPassFilterMode mode, int value) {
    switch (mode) {
        case LowPassFilterMode::Fir:
            connection_.write_integer(JetCommands::DSELowPassCutOffFrequencyFIR, value);
            break;
        case LowPassFilterMode::Iir:
            connection_.write_integer(JetCommands::DSELowPassCutOffFrequencyIIR, value);
            break;
        case LowPassFilterMode::None:
            break;
        default:
            throw std::invalid_argument("invalid low pass filter mode");
    }
}

void DSEJet::restore_factory_defaults() {
    connection_.write_integer(JetCommands::DSERestoreAllDefaultParameters, 1);
}

const JetCommand& DSEJet::filter_mode_command(FilterStage stage) {
    static const std::array<JetCommand, 4> kCommands = {
        JetCommands::DSEFilterModeStage2,
        JetCommands::DSEFilterModeStage3,
        JetCommands::DSEFilterModeStage4,
        JetCommands::DSEFilterModeStage5,
    };
    return command_for_stage(kCommands, stage);
}

const JetCommand& DSEJet::comb_frequency_command(FilterStage stage) {
    static const std::array<JetCommand, 4> kCommands = {
        JetCommands::DSECombFilterFrequencyStage2,
        JetCommands::DSECombFilterFrequencyStage3,
        JetCommands::DSECombFilterFrequencyStage4,
        JetCommands::DSECombFilterFrequencyStage5,
    };
    return command_for_stage(kCommands, stage);
}

const JetCommand& DSEJet::moving_average_frequency_command(FilterStage stage) {
    static const std::array<JetCommand, 4> kCommands = {
        JetCommands::DSEMovAvFilterFrequencyStage2,
        JetCommands::DSEMovAvFilterFrequencyStage3,
        JetCommands::DSEMovAvFilterFrequencyStage4,
        JetCommands::DSEMovAvFilterFrequencyStage5,
    };
    return command_for_stage(kCommands, stage);
}

FilterMode DSEJet::filter_from_register(int value) const {
    if (value >= 13105) {
        return FilterMode::FirMovingAverage;
    }
    if (value >= 13089 && value <= 13092) {
        return FilterMode::FirComb;
    }
    return FilterMode::None;
}

int DSEJet::reserve_filter_index(FilterMode desired) {
    static constexpr std::array<int, 4> kCombValues = {13089, 13090, 13091, 13092};
    static constexpr std::array<int, 4> kMovingValues = {13105, 13106, 13107, 13108};

    std::array<int, 4> active{};
    active[0] = connection_.read_integer(JetCommands::DSEFilterModeStage2);
    active[1] = connection_.read_integer(JetCommands::DSEFilterModeStage3);
    active[2] = connection_.read_integer(JetCommands::DSEFilterModeStage4);
    active[3] = connection_.read_integer(JetCommands::DSEFilterModeStage5);

    const auto& candidates = (desired == FilterMode::FirComb) ? kCombValues : kMovingValues;

    for (const int candidate : candidates) {
        bool in_use = false;
        for (const int value : active) {
            if (value == candidate) {
                in_use = true;
                break;
            }
        }
        if (!in_use) {
            return candidate;
        }
    }
    // fallback: use the first index
    return candidates.front();
}

}  // namespace dse
