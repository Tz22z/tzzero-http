#pragma once

#include <string>
#include <memory>
#include <sstream>
#include <mutex>
#include <fstream>

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
    
    void set_output_file(const std::string& filename);
    void set_max_file_size(size_t max_size_mb) { max_file_size_ = max_size_mb * 1024 * 1024; }
    void set_max_files(int max_files) { max_files_ = max_files; }
    
    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, const char* file, int line, const std::string& message);

private:
    Logger() = default;
    LogLevel level_{LogLevel::INFO};
    std::mutex mutex_;
    
    std::string output_file_;
    std::ofstream file_stream_;
    size_t current_file_size_{0};
    size_t max_file_size_{100 * 1024 * 1024};  // 默认 100MB
    int max_files_{10};
    
    void rotate_log_file();
    std::string level_to_string(LogLevel level) const;
    std::string get_timestamp() const;
};

// 便捷宏定义
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

}  // namespace tzzero::utils







