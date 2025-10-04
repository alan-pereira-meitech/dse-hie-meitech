#include "logging.h"

namespace dsejet {

Logger::Logger(LogLevel level, std::ostream &stream) : level_(level), stream_(&stream) {}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::level() const {
    return level_;
}

void Logger::log(LogLevel level, const std::string &message) {
    if (static_cast<int>(level) < static_cast<int>(level_)) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    (*stream_) << Timestamp() << " [" << LevelToString(level) << "] " << message << std::endl;
}

void Logger::trace(const std::string &message) { log(LogLevel::Trace, message); }
void Logger::debug(const std::string &message) { log(LogLevel::Debug, message); }
void Logger::info(const std::string &message) { log(LogLevel::Info, message); }
void Logger::warn(const std::string &message) { log(LogLevel::Warn, message); }
void Logger::error(const std::string &message) { log(LogLevel::Error, message); }

std::string Logger::Timestamp() const {
    using clock = std::chrono::system_clock;
    auto now = clock::now();
    auto time = clock::to_time_t(now);
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << millis.count();
    return oss.str();
}

const char *Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warn: return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

} // namespace dsejet

