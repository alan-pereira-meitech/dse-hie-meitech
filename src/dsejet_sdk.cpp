#include "dsejet/sdk.h"

#include <cctype>
#include <iterator>
#include <stdexcept>
#include <thread>

#include "dsejet_ws_client.h"
#include "logging.h"

namespace dsejet {

namespace {
constexpr const char *PATH_SCALE_COMMAND = "6002/01";
constexpr const char *PATH_SCALE_COMMAND_STATUS = "6002/02";
constexpr const char *PATH_GROSS_VALUE = "6144/00";
constexpr const char *PATH_NET_VALUE = "601A/01";
constexpr const char *PATH_WEIGHT_STATUS = "6012/01";

constexpr int SCALE_COMMAND_TARE = 1701994868;
constexpr int SCALE_COMMAND_ZERO = 1869768058;
constexpr int SCALE_COMMAND_GROSS = 1936683623;
constexpr int SCALE_STATUS_ONGOING = 1634168417;
constexpr int SCALE_STATUS_OK = 1801543519;

struct StagePaths {
    const char *mode;
    const char *comb_frequency;
    const char *moving_frequency;
};

const StagePaths STAGES[] = {
    {nullptr, nullptr, nullptr},
    {nullptr, nullptr, nullptr},
    {"6040/02", "3321/00", "3331/00"},
    {"6040/03", "3322/00", "3332/00"},
    {"6040/04", "3323/00", "3333/00"},
    {"6040/05", "3324/00", "3334/00"},
};

int FilterCode(const std::string &type, const StagePaths &stage, const char *&frequency_path) {
    std::string lower = type;
    for (auto &ch : lower) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    if (lower == "none" || lower == "off" || lower == "0") {
        frequency_path = nullptr;
        return 0;
    }
    if (lower == "fir_comb" || lower == "comb" || lower == "fir" || lower == "fir-comb") {
        frequency_path = stage.comb_frequency;
        return 13089;
    }
    if (lower == "fir_moving_average" || lower == "moving_average" || lower == "ma" || lower == "moving-average") {
        frequency_path = stage.moving_frequency;
        return 13105;
    }
    throw std::invalid_argument("Unsupported filter type: " + type);
}

} // namespace

DSEJetSDK::DSEJetSDK(Config config)
    : logger_(std::make_shared<Logger>(static_cast<LogLevel>(config.log_level))),
      config_(std::move(config)) {
    client_ = std::make_shared<DSEJetWSClient>(config_, logger_);
    client_->set_error_handler([this](const std::string &msg) {
        if (logger_) {
            logger_->warn(msg);
        }
    });
}

DSEJetSDK::~DSEJetSDK() {
    disconnect();
}

void DSEJetSDK::connect() {
    auto future = client_->connect();
    auto timeout = config_.connect_timeout.count() > 0 ? config_.connect_timeout : std::chrono::milliseconds(10000);
    if (future.wait_for(timeout) != std::future_status::ready) {
        disconnect();
        throw std::runtime_error("Connection timeout");
    }
    future.get();
}

void DSEJetSDK::disconnect() {
    if (client_) {
        client_->close();
    }
}

bool DSEJetSDK::is_connected() const {
    return client_ && client_->is_connected();
}

void DSEJetSDK::ensure_connected() {
    if (!client_) {
        throw std::runtime_error("Client not initialised");
    }
    if (!client_->is_connected()) {
        throw std::runtime_error("Client not connected");
    }
}

void DSEJetSDK::subscribe_stream(const std::vector<std::string> &paths, const StreamCallback &callback) {
    ensure_connected();
    client_->subscribe(paths, callback);
}

nlohmann::json DSEJetSDK::read_once(const std::string &path, std::chrono::milliseconds timeout) {
    ensure_connected();
    if (timeout.count() <= 0) {
        timeout = config_.request_timeout;
    }
    auto future = client_->fetch_once(path, timeout);
    if (future.wait_for(timeout) != std::future_status::ready) {
        throw std::runtime_error("Read timeout for path " + path);
    }
    return future.get();
}

void DSEJetSDK::write_value(const std::string &path, const nlohmann::json &value, std::chrono::milliseconds timeout) {
    ensure_connected();
    if (timeout.count() <= 0) {
        timeout = config_.request_timeout;
    }
    auto future = client_->set_value(path, value, timeout);
    if (future.wait_for(timeout) != std::future_status::ready) {
        throw std::runtime_error("Write timeout for path " + path);
    }
    future.get();
}

void DSEJetSDK::write_scale_command(int command_value, const std::string &label) {
    write_value(PATH_SCALE_COMMAND, command_value);
    wait_for_command_completion(label, config_.request_timeout);
}

void DSEJetSDK::wait_for_command_completion(const std::string &label, std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto status_json = read_once(PATH_SCALE_COMMAND_STATUS, config_.request_timeout);
        int status = 0;
        if (status_json.is_number_integer()) {
            status = status_json.get<int>();
        } else if (status_json.is_string()) {
            status = std::stoi(status_json.get<std::string>());
        }
        if (status != SCALE_STATUS_ONGOING) {
            if (status != SCALE_STATUS_OK) {
                throw std::runtime_error(label + " failed with status " + std::to_string(status));
            }
            return;
        }
        if (std::chrono::steady_clock::now() - start > timeout) {
            throw std::runtime_error(label + " timed out");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void DSEJetSDK::tare() {
    write_scale_command(SCALE_COMMAND_TARE, "Tare");
}

void DSEJetSDK::zero() {
    write_scale_command(SCALE_COMMAND_ZERO, "Zero");
}

void DSEJetSDK::set_gross() {
    write_scale_command(SCALE_COMMAND_GROSS, "Set gross");
}

void DSEJetSDK::set_filter_stage(int stage, const std::string &type, int frequency) {
    ensure_connected();
    if (stage < 2 || stage >= static_cast<int>(std::size(STAGES))) {
        throw std::out_of_range("Unsupported filter stage: " + std::to_string(stage));
    }
    const auto &paths = STAGES[stage];
    if (!paths.mode) {
        throw std::runtime_error("Stage paths not defined");
    }
    const char *frequency_path = nullptr;
    int filter = FilterCode(type, paths, frequency_path);
    write_value(paths.mode, filter);
    if (frequency_path != nullptr) {
        write_value(frequency_path, frequency);
    }
}

} // namespace dsejet

