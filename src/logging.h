#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

namespace dsejet {

enum class LogLevel : int {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
};

class Logger : public std::enable_shared_from_this<Logger> {
public:
    explicit Logger(LogLevel level = LogLevel::Info, std::ostream &stream = std::clog);

    void set_level(LogLevel level);

    LogLevel level() const;

    void log(LogLevel level, const std::string &message);

    void trace(const std::string &message);
    void debug(const std::string &message);
    void info(const std::string &message);
    void warn(const std::string &message);
    void error(const std::string &message);

private:
    std::string Timestamp() const;

    static const char *LevelToString(LogLevel level);

    mutable std::mutex mutex_;
    LogLevel level_;
    std::ostream *stream_;
};

} // namespace dsejet

