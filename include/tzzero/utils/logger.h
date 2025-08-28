#pragma once

#include <string>
#include <memory>
#include <sstream>
#include <mutex>

namespace tzzero::utils {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

class Logger {
public:
    static Logger& instance();
    
    void set_level(LogLevel level) { level_ = level; }
    LogLevel get_level() const { return level_; }
    
    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, const char* file, int line, const std::string& message);

private:
    Logger() = default;
    LogLevel level_{LogLevel::INFO};
    std::mutex mutex_;
    
    std::string level_to_string(LogLevel level) const;
    std::string get_timestamp() const;
};

// Convenience macros
#define LOG_DEBUG(msg) \
    do { \
        if (tzzero::utils::Logger::instance().get_level() <= tzzero::utils::LogLevel::DEBUG) { \
            std::ostringstream oss; oss << msg; \
            tzzero::utils::Logger::instance().log(tzzero::utils::LogLevel::DEBUG, __FILE__, __LINE__, oss.str()); \
        } \
    } while(0)

#define LOG_INFO(msg) \
    do { \
        if (tzzero::utils::Logger::instance().get_level() <= tzzero::utils::LogLevel::INFO) { \
            std::ostringstream oss; oss << msg; \
            tzzero::utils::Logger::instance().log(tzzero::utils::LogLevel::INFO, __FILE__, __LINE__, oss.str()); \
        } \
    } while(0)

#define LOG_WARN(msg) \
    do { \
        if (tzzero::utils::Logger::instance().get_level() <= tzzero::utils::LogLevel::WARN) { \
            std::ostringstream oss; oss << msg; \
            tzzero::utils::Logger::instance().log(tzzero::utils::LogLevel::WARN, __FILE__, __LINE__, oss.str()); \
        } \
    } while(0)

#define LOG_ERROR(msg) \
    do { \
        if (tzzero::utils::Logger::instance().get_level() <= tzzero::utils::LogLevel::ERROR) { \
            std::ostringstream oss; oss << msg; \
            tzzero::utils::Logger::instance().log(tzzero::utils::LogLevel::ERROR, __FILE__, __LINE__, oss.str()); \
        } \
    } while(0)

#define LOG_FATAL(msg) \
    do { \
        std::ostringstream oss; oss << msg; \
        tzzero::utils::Logger::instance().log(tzzero::utils::LogLevel::FATAL, __FILE__, __LINE__, oss.str()); \
    } while(0)

} // namespace tzzero::utils
