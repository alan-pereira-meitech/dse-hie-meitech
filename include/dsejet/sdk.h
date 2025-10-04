#pragma once

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "dsejet/config.h"

namespace dsejet {

class DSEJetWSClient;
class Logger;

typedef std::function<void(const std::string &, const nlohmann::json &)> StreamCallback;

class DSEJetSDK {
public:
    explicit DSEJetSDK(Config config);
    ~DSEJetSDK();

    void connect();
    void disconnect();
    bool is_connected() const;

    void subscribe_stream(const std::vector<std::string> &paths, const StreamCallback &callback);

    nlohmann::json read_once(const std::string &path, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    void write_value(const std::string &path, const nlohmann::json &value, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

    void tare();
    void zero();
    void set_gross();
    void write_scale_command(int command_value, const std::string &label);

    void set_filter_stage(int stage, const std::string &type, int frequency);

    const Config &config() const { return config_; }
    Config &mutable_config() { return config_; }

private:
    void ensure_connected();
    void wait_for_command_completion(const std::string &label, std::chrono::milliseconds timeout);

    std::shared_ptr<Logger> logger_;
    std::shared_ptr<DSEJetWSClient> client_;
    Config config_;
};

} // namespace dsejet

