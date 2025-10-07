#include "hbm/automation/api/data/jet_process_data.hpp"

#include <array>
#include <map>

namespace hbm::automation::api::data {

JetProcessData::JetProcessData() = default;

void JetProcessData::refresh(const INetConnection& connection)
{
    decimals_ = connection.read_integer_from_buffer(weighing::wtx::jet::JetBusCommands::CIA461Decimals());
    application_mode_ = static_cast<ApplicationMode>(
        connection.read_integer_from_buffer(weighing::wtx::jet::JetBusCommands::IMDApplicationMode()));
    general_scale_error_ = connection.read_integer_from_buffer(
        weighing::wtx::jet::JetBusCommands::CIA461WeightStatusGeneralWeightError()) != 0;
    const int limit_status = connection.read_integer_from_buffer(
        weighing::wtx::jet::JetBusCommands::CIA461WeightStatusLimitStatus());
    underload_ = (limit_status == 1);
    overload_ = (limit_status == 2);
    higher_safe_load_limit_ = (limit_status == 3);

    tare_mode_ = evaluate_tare_mode(
        connection.read_integer_from_buffer(weighing::wtx::jet::JetBusCommands::CIA461WeightStatusManualTare()),
        connection.read_integer_from_buffer(weighing::wtx::jet::JetBusCommands::CIA461WeightStatusWeightType()));

    weight_stable_ = connection.read_integer_from_buffer(
                          weighing::wtx::jet::JetBusCommands::CIA461WeightStatusWeightMoving()) == 0;
    legal_for_trade_ = connection.read_integer_from_buffer(
                           weighing::wtx::jet::JetBusCommands::CIA461WeightStatusScaleSealIsOpen()) == 0;
    scale_range_ = connection.read_integer_from_buffer(
        weighing::wtx::jet::JetBusCommands::CIA461WeightStatusScaleRange());
    zero_required_ = connection.read_integer_from_buffer(
        weighing::wtx::jet::JetBusCommands::CIA461WeightStatusZeroRequired()) != 0;
    center_of_zero_ = connection.read_integer_from_buffer(
        weighing::wtx::jet::JetBusCommands::CIA461WeightStatusCenterOfZero()) != 0;
    inside_zero_ = connection.read_integer_from_buffer(
        weighing::wtx::jet::JetBusCommands::CIA461WeightStatusInsideZero()) != 0;

    unit_ = unit_id_to_string(
        connection.read_integer_from_buffer(weighing::wtx::jet::JetBusCommands::CIA461Unit()));

    const int net_digits = connection.read_integer_from_buffer(
        weighing::wtx::jet::JetBusCommands::CIA461NetValue());
    const int gross_digits = connection.read_integer_from_buffer(
        weighing::wtx::jet::JetBusCommands::CIA461GrossValue());
    const int tare_digits = connection.read_integer_from_buffer(
        weighing::wtx::jet::JetBusCommands::CIA461TareValue());

    const double net = utils::digit_to_double(net_digits, decimals_);
    const double gross = utils::digit_to_double(gross_digits, decimals_);
    const double tare = utils::digit_to_double(tare_digits, decimals_);

    weight_.update(net, gross, tare);
    printable_weight_.update(net, gross, tare, decimals_);
}

ApplicationMode JetProcessData::application_mode() const
{
    return application_mode_;
}

const WeightType& JetProcessData::weight() const
{
    return weight_;
}

const PrintableWeightType& JetProcessData::printable_weight() const
{
    return printable_weight_;
}

const std::string& JetProcessData::unit() const
{
    return unit_;
}

int JetProcessData::decimals() const
{
    return decimals_;
}

TareMode JetProcessData::tare_mode() const
{
    return tare_mode_;
}

bool JetProcessData::weight_stable() const
{
    return weight_stable_;
}

bool JetProcessData::center_of_zero() const
{
    return center_of_zero_;
}

bool JetProcessData::inside_zero() const
{
    return inside_zero_;
}

bool JetProcessData::zero_required() const
{
    return zero_required_;
}

int JetProcessData::scale_range() const
{
    return scale_range_;
}

bool JetProcessData::general_scale_error() const
{
    return general_scale_error_;
}

bool JetProcessData::legal_for_trade() const
{
    return legal_for_trade_;
}

bool JetProcessData::underload() const
{
    return underload_;
}

bool JetProcessData::overload() const
{
    return overload_;
}

bool JetProcessData::higher_safe_load_limit() const
{
    return higher_safe_load_limit_;
}

TareMode JetProcessData::evaluate_tare_mode(int manual_tare, int weight_type)
{
    if (manual_tare != 0) {
        return TareMode::PresetTare;
    }

    if (weight_type != 0) {
        return TareMode::Tare;
    }

    return TareMode::None;
}

std::string JetProcessData::unit_id_to_string(int unit_id)
{
    switch (unit_id) {
    case 0x00020000:
        return "kg";
    case 0x004B0000:
        return "g";
    case 0x00A60000:
        return "lb";
    case 0x004C0000:
        return "t";
    default:
        return "";
    }
}

} // namespace hbm::automation::api::data
