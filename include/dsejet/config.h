#pragma once

#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace dsejet {

struct Config {
    std::string host = "127.0.0.1";
    std::uint16_t port = 80;
    bool tls = false;
    std::string user;
    std::string password;
    std::string jet_path = "/jet/canopen";
    std::chrono::milliseconds connect_timeout{10000};
    std::chrono::milliseconds request_timeout{3000};
    std::chrono::milliseconds ping_interval{5000};
    std::chrono::milliseconds reconnect_initial{1000};
    std::chrono::milliseconds reconnect_max{30000};
    std::chrono::milliseconds poll_interval{1000};
    int log_level = 2; // 0=trace .. 4=error

    static Config FromEnv(const std::string &env_path = ".env") {
        Config cfg;
        cfg.ApplyEnvironment();
        auto file_env = LoadEnvFile(env_path);
        cfg.ApplyEnvMap(file_env);
        cfg.ApplyEnvironment();
        return cfg;
    }

    void ApplyCliArgs(int argc, char **argv) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            auto require_value = [&](const char *name) {
                if (i + 1 >= argc) {
                    throw std::invalid_argument(std::string("Missing value for ") + name);
                }
                return std::string(argv[++i]);
            };
            if (arg == "--host") {
                host = require_value("--host");
            } else if (arg == "--port") {
                port = static_cast<std::uint16_t>(std::stoi(require_value("--port")));
            } else if (arg == "--tls") {
                auto val = require_value("--tls");
                ToLower(val);
                if (val == "yes" || val == "true" || val == "1") {
                    tls = true;
                } else if (val == "no" || val == "false" || val == "0") {
                    tls = false;
                } else {
                    throw std::invalid_argument("Invalid value for --tls (expected yes/no)");
                }
            } else if (arg == "--user") {
                user = require_value("--user");
            } else if (arg == "--password") {
                password = require_value("--password");
            } else if (arg == "--interval-ms") {
                poll_interval = std::chrono::milliseconds(std::stoi(require_value("--interval-ms")));
            } else if (arg == "--timeout-ms") {
                auto value = std::chrono::milliseconds(std::stoi(require_value("--timeout-ms")));
                connect_timeout = value;
                request_timeout = value;
            } else if (arg == "--log-level") {
                log_level = std::stoi(require_value("--log-level"));
                if (log_level < 0) {
                    log_level = 0;
                } else if (log_level > 4) {
                    log_level = 4;
                }
            } else if (arg == "--ping-ms") {
                ping_interval = std::chrono::milliseconds(std::stoi(require_value("--ping-ms")));
            } else if (arg == "--reconnect-ms") {
                reconnect_initial = std::chrono::milliseconds(std::stoi(require_value("--reconnect-ms")));
            } else if (arg == "--reconnect-max-ms") {
                reconnect_max = std::chrono::milliseconds(std::stoi(require_value("--reconnect-max-ms")));
            } else if (arg == "--jet-path") {
                jet_path = require_value("--jet-path");
            }
        }
    }

    static std::unordered_map<std::string, std::string> LoadEnvFile(const std::string &path) {
        std::unordered_map<std::string, std::string> result;
        std::ifstream file(path);
        if (!file.is_open()) {
            return result;
        }
        std::string line;
        while (std::getline(file, line)) {
            Trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            auto pos = line.find('=');
            if (pos == std::string::npos) {
                continue;
            }
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            Trim(key);
            Trim(value);
            if (!key.empty()) {
                result[key] = value;
            }
        }
        return result;
    }

    void ApplyEnvironment() {
        ApplyEnvMap({
            {"DSE_HOST", GetEnv("DSE_HOST")},
            {"DSE_PORT", GetEnv("DSE_PORT")},
            {"DSE_TLS", GetEnv("DSE_TLS")},
            {"DSE_USER", GetEnv("DSE_USER")},
            {"DSE_PASSWORD", GetEnv("DSE_PASSWORD")},
            {"DSE_PING_MS", GetEnv("DSE_PING_MS")},
            {"DSE_REQUEST_TIMEOUT_MS", GetEnv("DSE_REQUEST_TIMEOUT_MS")},
            {"DSE_CONNECT_TIMEOUT_MS", GetEnv("DSE_CONNECT_TIMEOUT_MS")},
            {"DSE_RECONNECT_INITIAL_MS", GetEnv("DSE_RECONNECT_INITIAL_MS")},
            {"DSE_RECONNECT_MAX_MS", GetEnv("DSE_RECONNECT_MAX_MS")},
            {"DSE_INTERVAL_MS", GetEnv("DSE_INTERVAL_MS")},
            {"DSE_LOG_LEVEL", GetEnv("DSE_LOG_LEVEL")},
            {"DSE_JET_PATH", GetEnv("DSE_JET_PATH")},
        });
    }

    void ApplyEnvMap(const std::unordered_map<std::string, std::string> &values) {
        for (const auto &kv : values) {
            if (kv.second.empty()) {
                continue;
            }
            if (kv.first == "DSE_HOST") {
                host = kv.second;
            } else if (kv.first == "DSE_PORT") {
                port = static_cast<std::uint16_t>(std::stoi(kv.second));
            } else if (kv.first == "DSE_TLS") {
                std::string lower = kv.second;
                ToLower(lower);
                tls = (lower == "yes" || lower == "true" || lower == "1");
            } else if (kv.first == "DSE_USER") {
                user = kv.second;
            } else if (kv.first == "DSE_PASSWORD") {
                password = kv.second;
            } else if (kv.first == "DSE_PING_MS") {
                ping_interval = std::chrono::milliseconds(std::stoi(kv.second));
            } else if (kv.first == "DSE_REQUEST_TIMEOUT_MS") {
                request_timeout = std::chrono::milliseconds(std::stoi(kv.second));
            } else if (kv.first == "DSE_CONNECT_TIMEOUT_MS") {
                connect_timeout = std::chrono::milliseconds(std::stoi(kv.second));
            } else if (kv.first == "DSE_RECONNECT_INITIAL_MS") {
                reconnect_initial = std::chrono::milliseconds(std::stoi(kv.second));
            } else if (kv.first == "DSE_RECONNECT_MAX_MS") {
                reconnect_max = std::chrono::milliseconds(std::stoi(kv.second));
            } else if (kv.first == "DSE_INTERVAL_MS") {
                poll_interval = std::chrono::milliseconds(std::stoi(kv.second));
            } else if (kv.first == "DSE_LOG_LEVEL") {
                log_level = std::stoi(kv.second);
            } else if (kv.first == "DSE_JET_PATH") {
                jet_path = kv.second;
            }
        }
    }

private:
    static void Trim(std::string &value) {
        const auto is_space = [](unsigned char c) { return std::isspace(c); };
        while (!value.empty() && is_space(value.front())) {
            value.erase(value.begin());
        }
        while (!value.empty() && is_space(value.back())) {
            value.pop_back();
        }
    }

    static void ToLower(std::string &value) {
        for (auto &ch : value) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
    }

    static std::string GetEnv(const char *name) {
        const char *val = std::getenv(name);
        return val ? std::string(val) : std::string();
    }
};

} // namespace dsejet

